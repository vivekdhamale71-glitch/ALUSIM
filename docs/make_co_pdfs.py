#!/usr/bin/env python3
"""Makes one PDF per course outcome: how it was implemented, which files, and what every function does.

    python3 docs/make_co_pdfs.py            all subjects
    python3 docs/make_co_pdfs.py PL         only one subject (PL, COA, CG or PSOOP)

The text comes from docs/co_files.py and docs/co_details.py; the function tables are read from the
code files themselves (every function with the comment written above it), so they cannot drift.
The PDF is written into the CO's own folder. Needs Google Chrome or Chromium.
"""
import html
import os
import re
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from co_details import HOW
from co_files import COS, SUBJECTS

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

CSS = """
@page { size: A4; margin: 16mm 15mm; }
body { font-family: 'DejaVu Sans', Arial, sans-serif; font-size: 10pt; color: #1b2330; line-height: 1.42; }
.band { background: #1f3a68; color: #fff; padding: 13px 18px; border-radius: 6px; }
.band .sub { font-size: 8.5pt; letter-spacing: .05em; text-transform: uppercase; opacity: .85; }
.band h1 { margin: 4px 0 3px; font-size: 18pt; }
.band .statement { font-size: 10pt; font-style: italic; opacity: .95; }
h2 { font-size: 11.5pt; color: #1f3a68; border-bottom: 1px solid #c9d4e6; padding-bottom: 3px; margin: 16px 0 7px; }
p { margin: 3px 0; }
.file { border: 1px solid #c9d4e6; border-radius: 6px; padding: 9px 12px; margin: 10px 0; page-break-inside: avoid; }
.filehead { margin-bottom: 4px; }
.fname { font-family: 'DejaVu Sans Mono', monospace; font-weight: bold; font-size: 10.5pt; }
.tag { float: right; font-size: 8pt; font-weight: bold; padding: 2px 8px; border-radius: 10px; }
.in { background: #dff3e4; color: #1d6b34; }
.idea { background: #fdf0d2; color: #8a5a00; }
.only { background: #e6e8ee; color: #4b5568; }
.use { margin: 5px 0 3px; }
.where { font-size: 8.5pt; color: #44506a; }
.note { font-size: 9pt; color: #44506a; font-style: italic; margin-top: 2px; }
table { border-collapse: collapse; width: 100%; margin-top: 7px; font-size: 8.8pt; }
th { background: #eef2fa; text-align: left; }
th, td { border: 1px solid #d6deee; padding: 3px 7px; vertical-align: top; }
td.fn { font-family: 'DejaVu Sans Mono', monospace; font-size: 8.3pt; white-space: nowrap; font-weight: bold; }
.foot { margin-top: 14px; font-size: 8pt; color: #7a8698; }
"""

CONTROL = {"if", "for", "while", "switch", "else", "catch", "return", "sizeof"}


def capitalise(text):
    return text[0].upper() + text[1:]


def comment_above(lines, index, marker):
    """the comment lines directly above lines[index], joined into one sentence"""
    parts = []
    title = ""
    i = index - 1
    while i >= 0 and lines[i].strip().startswith(marker):
        text = lines[i].strip()[len(marker):].strip()
        divider = re.match(r"^[-=]{3,}\s*(.*?)\s*[-=]*$", text)
        if divider:
            title = divider.group(1)                    # a section divider such as  ---- bubble sort
        elif text:
            parts.insert(0, text)
        i -= 1
    description = ""
    for part in parts:                                  # lines of one comment become separate sentences
        if description and description[-1] not in ".;:!?),":
            if part[:1].isupper():
                description += "."          # a new sentence starts here
        description = (description + " " + part).strip()
    if title and description:
        return title[0].upper() + title[1:] + ": " + description
    return description


def functions_of(path):
    """[(name, description)] for every function or class that has a comment above it"""
    if not os.path.exists(path):
        return []
    lines = open(path).read().split("\n")
    asm = path.endswith(".asm")
    found = []
    for index, raw in enumerate(lines):
        line = raw.strip()
        if index == 0:
            continue                                    # the one-line comment on top of the file
        if asm:
            m = re.match(r"^([A-Za-z_][A-Za-z0-9_]*):\s*$", raw)
            if m and not raw.startswith("."):
                text = comment_above(lines, index, ";")
                # the comment starts with  name(arguments) -> result : what it does   keep only what it does
                signature = re.match(r"^[A-Za-z_]\w*\(.*?\)\s*(?:->\s*[^:]+?)?\s:\s*", text)
                if signature:
                    text = text[signature.end():]
                if text:
                    found.append((m.group(1), text))
            continue
        name = None
        m = re.match(r"^(?:class|struct)\s+([A-Za-z_]\w*)\b[^;]*\{\s*$", line)
        if m:
            name = m.group(1)
        else:
            m = re.match(r"^[A-Za-z_][\w:<>,\*&\s~]*?\b([A-Za-z_~]\w*)\s*\([^;]*\)\s*(?:const)?\s*(?:override)?\s*(?:noexcept)?\s*(?::[^{]*)?\{\s*$", line)
            if m and m.group(1) not in CONTROL and not line.startswith(("if", "for", "while", "switch", "else")):
                name = m.group(1)
        if name:
            text = comment_above(lines, index, "//")
            if text:
                found.append((name, text))
    return found


def file_block(subject_folder, folder, name, sentence):
    path = os.path.join(ROOT, "src", subject_folder, folder, name)
    rows = ""
    for function, description in functions_of(path):
        if len(description) > 300:
            cut = description[:300]
            end = max(cut.rfind(". "), cut.rfind("; "))
            if end < 120:
                end = cut.rfind(" ")
            description = cut[:end].rstrip(".;, ") + " ..."
        rows += "<tr><td class='fn'>%s</td><td>%s</td></tr>" % (html.escape(function), html.escape(capitalise(description)))
    table = ""
    if rows:
        table = "<table><tr><th>Function</th><th>What it does</th></tr>%s</table>" % rows
    return ("<div class='file'><div class='filehead'><span class='fname'>%s</span></div>"
            "<p class='use'>%s</p>%s</div>" % (html.escape(name), html.escape(capitalise(sentence)), table))


def page(key, subject_folder, subject_name, code, number, folder, statement, concept, files):
    blocks = "".join(file_block(subject_folder, folder, n, s) for n, s in files)
    how = HOW.get((key, number), "")
    how_html = "<h2>How we implemented it</h2><p>%s</p>" % html.escape(how) if how else ""
    return """<!doctype html><html><head><meta charset="utf-8"><style>%s</style></head><body>
<div class="band"><div class="sub">%s &middot; %s &middot; %s</div>
<h1>%s.CO%d</h1><div class="statement">%s</div></div>
<h2>The idea in one line</h2><p>%s</p>
%s
<h2>Where it is implemented</h2>%s
<div class="foot">src/%s/%s/ &middot; every file starts with the same one-line sentence as a comment</div>
</body></html>""" % (CSS, key, html.escape(subject_name), code, code, number, html.escape(statement),
                     html.escape(concept), how_html, blocks, subject_folder, folder)


def find_chrome():
    for name in ("google-chrome", "chromium", "chromium-browser", "google-chrome-stable"):
        found = shutil.which(name)
        if found:
            return found
    sys.exit("Chrome or Chromium was not found")


def main():
    wanted = sys.argv[1:] or [key for key, _, _, _ in SUBJECTS]
    subjects = {key: (folder, name, code) for key, folder, name, code in SUBJECTS}
    chrome = find_chrome()
    tmp = tempfile.mkdtemp()
    for subject, number, folder, statement, concept, files in COS:
        if subject not in wanted:
            continue
        subject_folder, subject_name, code = subjects[subject]
        out_dir = os.path.join(ROOT, "src", subject_folder, folder)
        os.makedirs(out_dir, exist_ok=True)
        pdf = os.path.join(out_dir, "CO%d_Implementation.pdf" % number)
        source = os.path.join(tmp, "%s_CO%d.html" % (subject, number))
        with open(source, "w", encoding="utf-8") as f:
            f.write(page(subject, subject_folder, subject_name, code, number, folder, statement, concept, files))
        subprocess.run([chrome, "--headless=new", "--disable-gpu", "--no-pdf-header-footer", "--user-data-dir=" + tmp + "/profile",
                        "--print-to-pdf=" + pdf, "file://" + source], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        print("wrote", os.path.relpath(pdf, ROOT))
    shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    main()
