#include "core/Pipeline.h"

#include <algorithm>
#include <set>

#include "COA_2310221L/coa_asm.h"

namespace vas::core {

const char* stageName(int s) {
    static const char* n[STAGE_COUNT] = {"IF", "ID", "EX", "MEM", "WB"};
    return (s >= 0 && s < STAGE_COUNT) ? n[s] : "?";
}

namespace {
bool isLoadLike(const Instruction& ins) {
    // the value arrives only after the MEM stage
    return ins.signals().MemToReg || ins.mnemonic() == "pop" || ins.mnemonic() == "ret";
}
bool writes(const Instruction& ins, int r) {
    for (int w : ins.writeRegs()) if (w == r) return true;
    return false;
}
} // namespace

PipelineResult PipelineSimulator::run(const Program& prog, const std::vector<ExecRecord>& trace,
                                      const PipelineConfig& cfg) {
    PipelineResult res;
    struct Committed {
        int row;            // index into res.rows (-1 if not stored)
        int start[STAGE_COUNT];
        int instr;
    };
    std::vector<Committed> done;
    std::set<int> memBusy;
    int lastWB = 0;

    for (const ExecRecord& rec : trace) {
        if (rec.index < 0 || rec.index >= int(prog.code.size())) continue;
        const Instruction& ins = *prog.code[rec.index];
        const Committed* p = done.empty() ? nullptr : &done.back();
        const ExecRecord* pRec = done.empty() ? nullptr : &trace[done.size() - 1];

        PipeRow row;
        row.text = rec.text;
        row.instrIndex = rec.index;
        row.branchTaken = rec.branchTaken;

        int F = p ? p->start[S_ID] : 1;
        if (p && pRec && pRec->branchTaken) F = std::max(F, p->start[S_EX] + 1);   // after the branch resolves
        if (cfg.singleMemoryPort) {
            while (memBusy.count(F)) { ++F; ++res.structural; row.hazard = "Structural: memory port busy with a load/store"; }
        }
        // stage timing and operand readiness come from control_unit.asm
        int st[STAGE_COUNT];
        vas_schedule(p ? p->start : nullptr, F, 0, st);   // without data hazards
        int X = st[S_EX];
        int needX = X;

        // RAW data hazards against earlier committed instructions
        struct Candidate { int k; int reg; bool load; };
        std::vector<Candidate> cands;
        for (int r : ins.readRegs()) {
            for (int k = int(done.size()) - 1; k >= 0 && k >= int(done.size()) - 4; --k) {
                const Instruction& q = *prog.code[done[k].instr];
                if (!writes(q, r)) continue;
                int qWB = done[k].start[S_WB];
                bool load = isLoadLike(q);
                // with forwarding: ALU results are usable after EX, load data after MEM
                // without: the register file is written in WB and read in the last ID cycle
                int req = vas_operand_ready(done[k].start[S_MEM], qWB, cfg.forwarding, load);
                if (req > needX) {
                    needX = req;
                    row.hazard = std::string(cfg.forwarding ? "Load-use hazard on " : "RAW hazard on ") +
                                 regName(r) + " - waits for \"" + trace[k].text + "\"";
                }
                if (cfg.forwarding) cands.push_back({k, r, load});
                break;   // only the most recent producer matters
            }
        }
        row.stallCycles = vas_schedule(p ? p->start : nullptr, F, needX, st);   // stalls are spent in ID
        int D = st[S_ID];
        X = st[S_EX];
        int M = st[S_MEM], W = st[S_WB];

        row.start[S_IF] = F; row.start[S_ID] = D; row.start[S_EX] = X; row.start[S_MEM] = M; row.start[S_WB] = W;
        row.end[S_IF] = D - 1; row.end[S_ID] = X - 1; row.end[S_EX] = M - 1; row.end[S_MEM] = W - 1; row.end[S_WB] = W;
        res.stalls += row.stallCycles;

        bool memOp = ins.readsMemory() || ins.writesMemory();
        if (memOp) memBusy.insert(M);

        int rowIdx = -1;
        if (int(res.rows.size()) < cfg.maxRows) {
            res.rows.push_back(row);
            rowIdx = int(res.rows.size()) - 1;
        }
        // a forward is needed when the producer has not written back by the
        // time this instruction leaves ID (its WB cycle is X or later)
        for (const auto& c : cands) {
            if (done[c.k].start[S_WB] < X) continue;
            ++res.forwardsUsed;
            Forward fw;
            fw.from = done[c.k].row;
            fw.to = rowIdx;
            fw.fromStage = c.load ? S_MEM : S_EX;
            fw.cycle = X;
            fw.reg = regName(c.reg);
            if (fw.from >= 0 && fw.to >= 0) res.forwards.push_back(fw);
        }

        Committed c;
        c.row = rowIdx;
        c.instr = rec.index;
        for (int s = 0; s < STAGE_COUNT; ++s) c.start[s] = row.start[s];
        done.push_back(c);
        lastWB = std::max(lastWB, W);

        // control hazard: the two sequential instructions fetched behind a
        // taken branch are flushed when the branch resolves in EX
        if (rec.branchTaken) {
            for (int g = 1; g <= 2; ++g) {
                ++res.flushes;
                if (int(res.rows.size()) >= cfg.maxRows) continue;
                PipeRow ghost;
                int gi = rec.index + g;
                ghost.text = gi < int(prog.code.size()) ? prog.code[gi]->text() : "(next fetch)";
                ghost.instrIndex = gi;
                ghost.flushed = true;
                ghost.flushCycle = M - 1;   // end of the branch's EX stage
                for (int s = 0; s < STAGE_COUNT; ++s) ghost.start[s] = ghost.end[s] = -1;
                if (g == 1) {
                    ghost.start[S_IF] = D; ghost.end[S_IF] = X - 1;
                    ghost.start[S_ID] = X; ghost.end[S_ID] = M - 1;
                } else {
                    ghost.start[S_IF] = X; ghost.end[S_IF] = M - 1;
                }
                ghost.hazard = "Flushed - fetched on the wrong path behind a taken branch";
                res.rows.push_back(ghost);
            }
        }
    }
    res.instructions = int(done.size());
    res.pipelinedCycles = lastWB;
    res.nonPipelinedCycles = 5 * res.instructions;
    return res;
}

} // namespace vas::core
