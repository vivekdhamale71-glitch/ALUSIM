#!/usr/bin/env python3
"""Puts the one-line comment from docs/co_files.py on top of every CO file (replacing any comment block there)."""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from co_files import COS, SUBJECTS

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
folders = {key: folder for key, folder, _, _ in SUBJECTS}


def sentence_comment(sentence, asm):
    marker = "; " if asm else "// "
    return marker + sentence + "\n"


def main():
    for subject, number, folder, _, _, files in COS:
        for name, sentence in files:
            path = os.path.join(ROOT, "src", folders[subject], folder, name)
            if not os.path.exists(path):
                continue
            asm = name.endswith(".asm")
            lines = open(path).read().split("\n")
            i = 0
            while i < len(lines) and (lines[i].startswith("//") or lines[i].startswith(";") or lines[i].strip() == ""):
                i += 1
            body = "\n".join(lines[i:])
            open(path, "w").write(sentence_comment(sentence, asm) + "\n" + body)


if __name__ == "__main__":
    main()
