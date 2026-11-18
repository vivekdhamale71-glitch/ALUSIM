#!/usr/bin/env python3
"""Writes the in-depth explanation PDF of every PL course outcome into its folder."""
import html
import os
import shutil
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from make_cg_deep_pdfs import EXTRA, block
from make_co_pdfs import CSS, find_chrome
from co_files import COS
from pl_deep_content import PL

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def with_folder_files(number, sections):
    """Replaces the folder rows of the last table by one row per file, with the sentence written on top of that file."""
    files = []
    for subject, n, folder, _, _, items in COS:
        if subject == "PL" and n == number:
            files = items
    heading, blocks = sections[-1]
    kind, head, rows = blocks[0]
    rows = [r for r in rows if not r[0].startswith("CO%d_" % number)]
    for name, sentence in files:
        rows.append(["%s/%s" % ("CO%d" % number, name), sentence])
    return sections[:-1] + [(heading, [(kind, head, rows)])]


def page(number, folder, statement, sections):
    sections = with_folder_files(number, sections)
    out = ["<html><head><meta charset='utf-8'><style>%s%s</style></head><body>" % (CSS, EXTRA)]
    out.append("<div class='band'><div class='sub'>Programming Languages &middot; 2310214L &middot; %s</div>" % html.escape(folder))
    out.append("<h1>CO%d &ndash; how it works</h1><div class='statement'>%s</div></div>" % (number, html.escape(statement)))
    for heading, blocks in sections:
        out.append("<h2>%s</h2>" % html.escape(heading))
        out.extend(block(b) for b in blocks)
    out.append("</body></html>")
    return "".join(out)


def main():
    chrome = find_chrome()
    tmp = tempfile.mkdtemp()
    for number, folder, statement, sections in PL:
        out_dir = os.path.join(ROOT, "src", "PL_2310214L", folder)
        pdf = os.path.join(out_dir, "CO%d_Explanation.pdf" % number)
        source = os.path.join(tmp, "PL_CO%d.html" % number)
        with open(source, "w", encoding="utf-8") as f:
            f.write(page(number, folder, statement, sections))
        subprocess.run([chrome, "--headless=new", "--disable-gpu", "--no-pdf-header-footer", "--user-data-dir=" + tmp + "/profile",
                        "--print-to-pdf=" + pdf, "file://" + source], check=True, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        print("wrote", os.path.relpath(pdf, ROOT))
    shutil.rmtree(tmp, ignore_errors=True)


if __name__ == "__main__":
    main()
