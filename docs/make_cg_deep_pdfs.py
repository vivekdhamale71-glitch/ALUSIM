#!/usr/bin/env python3
"""Writes the in-depth explanation PDF of every CG course outcome into its folder."""
import html
import os
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from cg_deep_content import CG
from make_co_pdfs import CSS, find_chrome

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
EXTRA = """
.steps { margin: 4px 0 6px 0; padding-left: 22px; }
.steps li { margin: 2px 0; }
pre { background: #f3f6fb; border: 1px solid #d6deee; border-radius: 5px; padding: 7px 10px; font-size: 8.3pt;
      font-family: 'DejaVu Sans Mono', monospace; white-space: pre-wrap; page-break-inside: avoid; }
td { font-size: 8.6pt; }
"""


def block(b):
    kind = b[0]
    if kind == "p":
        return "<p>%s</p>" % html.escape(b[1])
    if kind == "steps":
        return "<ol class='steps'>%s</ol>" % "".join("<li>%s</li>" % html.escape(s) for s in b[1])
    if kind == "code":
        return "<pre>%s</pre>" % html.escape(b[1])
    head = "".join("<th>%s</th>" % html.escape(h) for h in b[1])
    rows = "".join("<tr>%s</tr>" % "".join("<td>%s</td>" % html.escape(c) for c in row) for row in b[2])
    return "<table><tr>%s</tr>%s</table>" % (head, rows)


def page(number, folder, statement, sections):
    out = ["<html><head><meta charset='utf-8'><style>%s%s</style></head><body>" % (CSS, EXTRA)]
    out.append("<div class='band'><div class='sub'>Computer Graphics &middot; 2310218L &middot; %s</div>" % html.escape(folder))
    out.append("<h1>CO%d &ndash; how it works</h1><div class='statement'>%s</div></div>" % (number, html.escape(statement)))
    for heading, blocks in sections:
        out.append("<h2>%s</h2>" % html.escape(heading))
        out.extend(block(b) for b in blocks)
    out.append("</body></html>")
    return "".join(out)


def main():
    chrome = find_chrome()
    tmp = tempfile.mkdtemp()
    for number, folder, statement, sections in CG:
        out_dir = os.path.join(ROOT, "src", "CG_2310218L", folder)
        os.makedirs(out_dir, exist_ok=True)
        pdf = os.path.join(out_dir, "CO%d_Explanation.pdf" % number)
        source = os.path.join(tmp, "CG_CO%d.html" % number)
        with open(source, "w", encoding="utf-8") as f:
            f.write(page(number, folder, statement, sections))
        subprocess.run([chrome, "--headless=new", "--disable-gpu", "--no-pdf-header-footer", "--user-data-dir=" + tmp + "/profile",
                        "--print-to-pdf=" + pdf, "file://" + source], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        print("wrote", os.path.relpath(pdf, ROOT))
    shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    main()
