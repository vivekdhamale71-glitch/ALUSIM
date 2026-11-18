#!/usr/bin/env python3
"""Writes the COA CO4 lab deliverable PDF from a real run of the simulator core.

It builds docs/coa_co4_dump.cpp against src/, runs three programs, and turns the traces into one landscape PDF:
the three functionalities, the ALU and control unit designs, the control signal table, the fetch/decode/execute
sequences (hardwired and microprogrammed) and the five-stage pipeline timing diagrams.
"""
import glob
import html
import json
import os
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from make_co_pdfs import find_chrome

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OUT = os.path.join(ROOT, "src", "COA_2310221L", "CO4_ControlUnitPipeline", "CO4_Lab_Deliverables.pdf")
SIGS = ["PCWrite", "IRWrite", "MemRead", "MemWrite", "RegWrite", "ALUSrc", "MemToReg", "Branch", "Jump", "FlagWrite", "StackOp"]
STAGES = ["IF", "ID", "EX", "MEM", "WB"]
E = html.escape

CSS = """
@page { size: A4 landscape; margin: 12mm 12mm; }
body { font-family: 'DejaVu Sans', Arial, sans-serif; font-size: 9.2pt; color: #1b2330; line-height: 1.4; }
.band { background: #1f3a68; color: #fff; padding: 11px 16px; border-radius: 6px; }
.band .sub { font-size: 8pt; letter-spacing: .05em; text-transform: uppercase; opacity: .85; }
.band h1 { margin: 3px 0 2px; font-size: 17pt; }
h2 { font-size: 12pt; color: #1f3a68; border-bottom: 1px solid #c9d4e6; padding-bottom: 3px; margin: 14px 0 6px; page-break-after: avoid; }
h3 { font-size: 10pt; color: #1f3a68; margin: 10px 0 4px; page-break-after: avoid; }
p { margin: 3px 0; }
table { border-collapse: collapse; margin: 5px 0; font-size: 8pt; }
th { background: #eef2fa; text-align: left; }
th, td { border: 1px solid #d6deee; padding: 2px 6px; vertical-align: top; }
td.c, th.c { text-align: center; }
td.on { background: #1f3a68; color: #fff; text-align: center; font-weight: bold; }
td.off { color: #b4bccb; text-align: center; }
pre { background: #f3f6fb; border: 1px solid #d6deee; border-radius: 5px; padding: 6px 9px; font-size: 8pt; margin: 4px 0;
      font-family: 'DejaVu Sans Mono', monospace; white-space: pre-wrap; }
.mono { font-family: 'DejaVu Sans Mono', monospace; }
.g td { width: 27px; text-align: center; font-size: 7.6pt; padding: 2px 0; }
.g td.t { width: auto; text-align: left; padding: 2px 6px; white-space: nowrap; font-family: 'DejaVu Sans Mono', monospace; }
.IF { background: #cfe3ff; } .ID { background: #d9f2d9; } .EX { background: #ffe9b8; } .MEM { background: #f6d3ee; } .WB { background: #d6d6f5; }
.stall { background: #ffd0d0; color: #a32; } .flush { background: #e4e4e4; color: #888; text-decoration: line-through; }
.keep { page-break-inside: avoid; }
.pb { page-break-before: always; }
.note { font-size: 8.2pt; color: #44506a; }
"""


def build_json(tmp):
    exe = os.path.join(tmp, "dump")
    asm = sorted(glob.glob(os.path.join(ROOT, "build", "obj", "*engine_asm*.o")))
    if not asm:
        sys.exit("Run ./build.sh first: the NASM objects are needed")
    srcs = []
    for d in ("src/core", "src/engine/ds", "src/engine/lang"):
        for dirpath, _, names in os.walk(os.path.join(ROOT, d)):
            srcs += [os.path.join(dirpath, n) for n in names if n.endswith(".cpp")]
    subprocess.run(["g++", "-std=c++17", "-O1", "-I" + os.path.join(ROOT, "src"), "-I" + os.path.join(ROOT, "third_party"),
                    "-Wno-missing-field-initializers", "-o", exe, os.path.join(ROOT, "docs", "coa_co4_dump.cpp")] + srcs + asm, check=True)
    path = os.path.join(tmp, "co4.json")
    subprocess.run([exe], check=True, env=dict(os.environ, CO4_JSON=path), stdout=subprocess.DEVNULL)
    return json.load(open(path))


def sig_cells(sig):
    cells = "".join("<td class='on'>1</td>" if sig[s] else "<td class='off'>0</td>" for s in SIGS)
    return cells + "<td class='c mono'>%s</td>" % E(sig["ALUOp"])


def sig_head():
    return "".join("<th class='c'>%s</th>" % s for s in SIGS) + "<th class='c'>ALUOp</th>"


def active(sig):
    names = [s for s in SIGS if sig[s]]
    if sig["ALUOp"] != "---":
        names.append("ALUOp=" + sig["ALUOp"])
    return ", ".join(names) if names else "none"


def timing(rows, limit):
    shown = rows[:limit]
    last = max(max(r["end"]) for r in shown)
    out = ["<table class='g'><tr><th>Instruction</th>%s</tr>" % "".join("<th class='c'>%d</th>" % c for c in range(1, last + 1))]
    for r in shown:
        cells = {}
        for k in range(5):
            for c in range(r["start"][k], r["end"][k] + 1):
                cells[c] = (STAGES[k], c > r["start"][k])
        tds = []
        for c in range(1, last + 1):
            if c in cells:
                name, extra = cells[c]
                cls = "flush" if r["flushed"] else ("stall" if extra else name)
                tds.append("<td class='%s'>%s</td>" % (cls, name if not extra else "&middot;"))
            else:
                tds.append("<td></td>")
        out.append("<tr><td class='t'>%s</td>%s</tr>" % (E(r["text"]), "".join(tds)))
    out.append("</table>")
    return "".join(out)


def pipe_summary(p):
    cpi = p["cycles"] / p["n"]
    return ("%d instructions in %d cycles (CPI %.2f); without pipelining %d cycles, speed-up %.2f&times;; %d stall cycles, %d flushes, %d forwardings"
            % (p["n"], p["cycles"], cpi, p["plain"], p["plain"] / p["cycles"], p["stalls"], p["flushes"], p["fwd"]))


def page(d):
    o = ["<html><head><meta charset='utf-8'><style>%s</style></head><body>" % CSS]
    o.append("<div class='band'><div class='sub'>Computer Organization and Architecture &middot; 2310221L &middot; CO4 lab</div>"
             "<h1>Control unit, ALU and pipelining: three functionalities run on the simulator</h1></div>")
    o.append("<p class='note'>Every number in this document comes from running the three programs on the simulator core (the same code the "
             "application uses); nothing was typed in by hand. Programs of the same kind are in the application's example list (Parse tree: variables and operators, Pipeline: while loop sum, Datapath: loop + CALL / RET).</p>")

    # 1 ---------------------------------------------------------------------------------
    o.append("<h2>1. The three selected functionalities and their instructions</h2><table><tr><th>#</th><th>Functionality</th>"
             "<th>Source</th><th>Result</th><th>Instructions executed</th><th>What it exercises</th></tr>")
    what = ["ALU: multiply and add, memory load and store", "Compare, conditional jump, loop: branch (control) hazards",
            "Stack and procedure call: PUSH, POP, CALL, RET"]
    for i, j in enumerate(d["jobs"]):
        res = ", ".join("%s = %s" % (k, v) for k, v in j["data"].items())
        o.append("<tr><td>%d</td><td>%s</td><td><pre>%s</pre></td><td class='mono'>%s</td><td class='c'>%d</td><td>%s</td></tr>"
                 % (i + 1, E(j["name"]), E(j["source"]), E(res), len(j["trace"]), E(what[i])))
    o.append("</table>")
    o.append("<h3>Assembly of the three programs, as the processor executes it (1 and 2 are generated by the compiler, 3 is written by hand)</h3><table class='keep'><tr>")
    for i, j in enumerate(d["jobs"]):
        o.append("<th>%d. %s</th>" % (i + 1, E(j["name"])))
    o.append("</tr><tr>")
    for j in d["jobs"]:
        o.append("<td><pre>%s</pre></td>" % E(j["asm"].replace("section .data", ".data").replace("section .text", ".text").replace("global _start\n", "")))
    o.append("</tr></table>")

    # 2 ---------------------------------------------------------------------------------
    o.append("<h2 class='pb'>2. ALU design</h2>")
    o.append("<p>The ALU is one block with two operand inputs, an operation input from the control unit and two outputs: the result and the four "
             "flags. Operand A comes from the register file. Operand B comes from a multiplexer that the <b>ALUSrc</b> signal switches between "
             "register B (0) and the immediate value (1). The <b>ALUOp</b> value from the control unit selects the operation. The result goes to the "
             "write-back multiplexer or to data memory (as an address or a value). When <b>FlagWrite</b> is 1 the flags CF, ZF, SF and OF are stored.</p>"
             "<p>The operation itself is the NASM routine <span class='mono'>vas_alu</span> in <span class='mono'>src/engine/asm/alu.asm</span>; it uses the "
             "real x86-64 arithmetic instructions and returns the real RFLAGS, which the C++ model reads. Operations: PASS, ADD, SUB, MUL, DIV, AND, OR, "
             "XOR, NOT, NEG, SHL, SHR, INC, DEC.</p>")
    o.append("<h3>ALU operations observed in the three runs (operand A, operand B, result, flags after)</h3><table><tr><th>Program</th><th>Instruction</th>"
             "<th class='c'>ALUOp</th><th class='c'>A</th><th class='c'>B</th><th class='c'>Result</th><th class='c'>CF</th><th class='c'>ZF</th><th class='c'>SF</th><th class='c'>OF</th></tr>")
    for j in d["jobs"]:
        seen = set()
        for t in j["trace"]:
            key = (t["alu"], t["text"])
            if t["alu"] in ("---", "PASS") or key in seen:
                continue
            seen.add(key)
            f = t["flags"]
            o.append("<tr><td>%s</td><td class='mono'>%s</td><td class='c mono'>%s</td><td class='c'>%d</td><td class='c'>%d</td><td class='c'>%d</td>%s</tr>"
                     % (E(j["name"].split(":")[0]), E(t["text"]), t["alu"], t["aluA"], t["aluB"], t["aluOut"], "".join("<td class='c'>%d</td>" % x for x in f)))
    o.append("</table>")

    # 3 ---------------------------------------------------------------------------------
    o.append("<h2>3. Control unit design</h2>")
    o.append("<p>The control unit turns the decoded instruction into the control signals of each cycle. The program contains both organisations "
             "behind the same interface, and the Hardwired / Microprogrammed buttons switch between them. Both give the same signals; they differ in how "
             "they are produced.</p>"
             "<h3>Hardwired control</h3><p>A combinational decoder: the instruction form selects one row of the decoder table (the table is the ROM "
             "<span class='mono'>vas_decode</span> in <span class='mono'>src/engine/asm/control_unit.asm</span>, like the AND-OR gates of a decoder), and the "
             "row is the signal vector. An instruction goes through the phases it needs (FETCH, DECODE, EXECUTE, MEMORY, WRITE-BACK); there is no stored micro-program.</p>"
             "<h3>Microprogrammed control</h3><p>A control store (ROM) of micro-instructions, a micro-program counter (uPC) and a dispatch table. "
             "Words 0 to 2 are the common fetch: MAR &larr; RIP, MDR &larr; IMEM[MAR] with RIP &larr; RIP + 4, IR &larr; MDR. In word 2 the next address is "
             "<b>DISPATCH</b>: the opcode is looked up in the dispatch table to find the first word of that instruction's routine; the last word of a "
             "routine jumps back to FETCH. Each word holds the micro-operation and its signal vector.</p>")
    o.append("<h3>Control store (%d words)</h3><table><tr><th>uPC</th><th>Label</th><th>Phase</th><th>Micro-operation</th>%s<th>Next</th></tr>"
             % (len(d["store"]), sig_head()))
    for w in d["store"]:
        o.append("<tr class='keep'><td class='c'>%d</td><td class='mono'>%s</td><td>%s</td><td class='mono'>%s</td>%s<td>%s</td></tr>"
                 % (w["addr"], E(w["label"]), E(w["phase"]), E(w["op"]), sig_cells(w["sig"]), E(w["next"])))
    o.append("</table>")

    # 4 ---------------------------------------------------------------------------------
    o.append("<h2 class='pb'>4. Control signal table</h2>")
    o.append("<p>One row per instruction form. A 1 means the signal is active in that instruction.</p>"
             "<table><tr><th>Instruction form</th>%s</tr>" % sig_head())
    for r in d["decoder"]:
        o.append("<tr class='keep'><td class='mono'>%s</td>%s</tr>" % (E(r["form"]), sig_cells(r["sig"])))
    o.append("</table><table class='keep'><tr><th>Signal</th><th>Meaning when it is 1</th></tr>"
             "<tr><td>PCWrite</td><td>the program counter (RIP) is updated</td></tr><tr><td>IRWrite</td><td>the fetched instruction is stored in the IR</td></tr>"
             "<tr><td>MemRead</td><td>data memory is read</td></tr><tr><td>MemWrite</td><td>data memory is written</td></tr>"
             "<tr><td>RegWrite</td><td>a register is written in write-back</td></tr><tr><td>ALUSrc</td><td>the ALU's second operand is the immediate, not a register</td></tr>"
             "<tr><td>MemToReg</td><td>the register gets the memory data, not the ALU result</td></tr><tr><td>Branch</td><td>a conditional jump: the flags decide whether RIP changes</td></tr>"
             "<tr><td>Jump</td><td>an unconditional change of RIP (JMP, CALL, RET)</td></tr><tr><td>FlagWrite</td><td>the ALU's flags are stored</td></tr>"
             "<tr><td>StackOp</td><td>RSP is changed and the stack in memory is used (PUSH, POP, CALL, RET)</td></tr>"
             "<tr><td>ALUOp</td><td>the operation the ALU performs</td></tr></table>")

    # 5 ---------------------------------------------------------------------------------
    o.append("<h2 class='pb'>5. Fetch, decode and execute</h2>")
    o.append("<p>One instruction from each functionality, shown cycle by cycle under both control units. The signals listed are the ones that are 1 in that cycle.</p>")
    picks = [(0, 3), (1, 4), (2, 2)]
    for ji, ti in picks:
        j = d["jobs"][ji]
        t = j["trace"][ti]
        o.append("<h3>%s &ndash; <span class='mono'>%s</span></h3><div class='keep'>" % (E(j["name"]), E(t["text"])))
        o.append("<table><tr><th colspan='3'>Hardwired</th></tr><tr><th>Phase</th><th>Register transfer</th><th>Active signals</th></tr>")
        for s in t["hw"]:
            o.append("<tr><td>%s</td><td class='mono'>%s</td><td>%s</td></tr>" % (E(s["phase"]), E(s["op"]), E(active(s["sig"]))))
        o.append("</table><table><tr><th colspan='4'>Microprogrammed</th></tr><tr><th>uPC</th><th>Phase</th><th>Micro-operation</th><th>Active signals</th></tr>")
        for s in t["mp"]:
            o.append("<tr><td class='c'>%d &rarr; %d</td><td>%s</td><td class='mono'>%s</td><td>%s</td></tr>" % (s["addr"], s["next"], E(s["phase"]), E(s["op"]), E(active(s["sig"]))))
        o.append("</table></div>")
        if ji == 1:
            pass
    o.append("<h3>Signals of every instruction of the three programs</h3><table><tr><th>Program</th><th>#</th><th>Instruction</th>%s</tr>" % sig_head())
    for j in d["jobs"]:
        for t in j["trace"][:14]:
            o.append("<tr class='keep'><td>%s</td><td class='c'>%d</td><td class='mono'>%s</td>%s</tr>" % (E(j["name"].split(":")[0]), t["i"] + 1, E(t["text"]), sig_cells(t["sig"])))
    o.append("</table><p class='note'>The loop program executes %d instructions in total; its first 14 rows (one full pass of the loop and the start of the next) are listed.</p>" % len(d["jobs"][1]["trace"]))

    # 6 ---------------------------------------------------------------------------------
    o.append("<h2 class='pb'>6. Five-stage pipeline timing diagrams</h2>")
    o.append("<p>Stages: <span class='IF'>&nbsp;IF&nbsp;</span> fetch, <span class='ID'>&nbsp;ID&nbsp;</span> decode and register read, <span class='EX'>&nbsp;EX&nbsp;</span> ALU, "
             "<span class='MEM'>&nbsp;MEM&nbsp;</span> data memory, <span class='WB'>&nbsp;WB&nbsp;</span> write-back. A red dot <span class='stall'>&nbsp;&middot;&nbsp;</span> is a stall cycle; a "
             "struck-through grey row is a wrongly fetched instruction removed after a taken branch (flushed); a row named (next fetch) is the fetch of whatever follows the last instruction. Data hazards (RAW) stall the consumer until the value is written back, "
             "or with forwarding until it is available from EX or MEM; a taken branch is resolved in EX and flushes the two instructions fetched after it.</p>")
    limits = [8, 14, 9]   # the loop: 12 instructions (one pass) + the 2 wrong-path fetches after its taken jump
    for ji, j in enumerate(d["jobs"]):
        for key, title in (("noFwd", "without forwarding"), ("fwd", "with forwarding")):
            p = j["pipe"][key]
            o.append("<div class='keep'><h3>%s &ndash; %s</h3><p class='note'>%s</p>%s" % (E(j["name"]), title, pipe_summary(p), timing(p["rows"], limits[ji])))
            hz = [(r["text"], r["hazard"]) for r in p["rows"] if r["hazard"]]
            if hz:
                o.append("<p class='note'>%s</p>" % "<br>".join("<b>%s</b>: %s" % (E(a), E(b)) for a, b in hz[:5]))
            o.append("</div>")
        if ji == 1:
            o.append("<p class='note'>The diagram shows one pass of the loop (12 instructions) and the two wrongly fetched instructions after its taken jump; the totals above are for the whole run of %d instructions.</p>" % len(j["trace"]))
    o.append("<h3>Summary</h3><table><tr><th>Functionality</th><th class='c'>Instructions</th><th class='c'>Not pipelined</th><th class='c'>Pipelined, no forwarding</th>"
             "<th class='c'>Pipelined, with forwarding</th><th class='c'>CPI (forwarding)</th><th class='c'>Speed-up (forwarding)</th></tr>")
    for j in d["jobs"]:
        a, b = j["pipe"]["noFwd"], j["pipe"]["fwd"]
        o.append("<tr><td>%s</td><td class='c'>%d</td><td class='c'>%d</td><td class='c'>%d</td><td class='c'>%d</td><td class='c'>%.2f</td><td class='c'>%.2f&times;</td></tr>"
                 % (E(j["name"]), b["n"], b["plain"], a["cycles"], b["cycles"], b["cycles"] / b["n"], b["plain"] / b["cycles"]))
    o.append("</table><p class='note'>Where to see these in the application: Datapath tab (ALU, control unit, CONTROL SIGNALS, HARDWIRED DECODER TABLE, CONTROL STORE, "
             "phase timeline, waveform) and Pipeline tab (HAZARD HANDLING switches, PIPELINE AT CYCLE, SPACE-TIME DIAGRAM).</p></body></html>")
    return "".join(o)


def main():
    chrome = find_chrome()
    with tempfile.TemporaryDirectory() as tmp:
        data = build_json(tmp)
        source = os.path.join(tmp, "co4.html")
        with open(source, "w", encoding="utf-8") as f:
            f.write(page(data))
        subprocess.run([chrome, "--headless=new", "--disable-gpu", "--no-pdf-header-footer", "--user-data-dir=" + tmp + "/profile",
                        "--print-to-pdf=" + OUT, "file://" + source], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    print("wrote", os.path.relpath(OUT, ROOT))


if __name__ == "__main__":
    main()
