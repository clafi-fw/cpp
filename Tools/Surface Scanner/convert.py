"""Moves the tree onto the declaration routine, one mechanical step at a time.

Reads scan.py's own report, so the two never disagree about what a declaration is.

    python convert.py --dry-run    prints what it would write, and what it cannot do
    python convert.py              writes it

It only ever does what the source already says: the one-line comment is the first sentence
already there, and the whole block is moved into a note verbatim. Where the first sentence
does not fit the column limit, it stops and says so rather than inventing a shorter one.
"""

import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
SOURCE = ROOT / "Source"
NOTES = ROOT / "Source" / "RawDocs"
MAX_COLUMNS = 100

# Which note carries the prose a file's declarations shed. One per subsystem, so a reader
# looking for what a control does has one place to look rather than forty.
NOTE_OF = [
    ("Controls/Base", "Controls-Base"),
    ("Controls/Grids", "Grids"),
    ("Controls/Browser", "Browser"),
    ("Controls", "Controls"),
    ("Y-Core/Foundation", "Control-Foundation"),
    ("Y-Core/TextEngine", "TextEngine-Types"),
    ("Y-Core/Graphics", "Graphics-Types"),
    ("Y-Core/Context", "Context"),
    ("Y-Core/Transfer", "Transfer"),
    ("Y-Core/AppTheme", "AppTheme"),
    ("Y-Core/Dom", "Dom"),
    ("Y-Core/System", "UI-Types"),
    ("Y-Core", "Y-Core"),
    ("Platform", "Platform"),
    ("Icons", "Icons"),
    ("Application", "Application"),
    ("Diagnostic", "Diagnostic"),
    ("StdActions", "StdActions"),
    ("PathArt", "PathArt"),
]

RE_MULTILINE = re.compile(r"^(.+?):(\d+): error: ([\w:*]+) has a comment of (\d+) lines")
RE_FITS = re.compile(r"^(.+?):(\d+): warning: ([\w:*]+) fits its comment on the declaration line, in (\d+)")


def noteOf(path):
    rel = path.relative_to(SOURCE).as_posix()
    for prefix, note in NOTE_OF:
        if rel.startswith(prefix + "/"):
            return note
    return "Y-Core"


def slug(text):
    return re.sub(r"[^a-z0-9]+", "-", text.lower()).strip("-")


def bodyOf(line):
    """A comment line's words, with the marker and any doxygen tag taken off."""
    text = line.strip()
    text = text.lstrip("/").strip()
    return re.sub(r"^@(brief|note|class)\s*", "", text)


def noteBody(line):
    """A comment line for the note: the marker off, whatever indentation followed kept."""
    text = line.strip()
    text = re.sub(r"^/+ ?", "", text)
    return re.sub(r"^@(brief|note|class)\s*", "", text)


def firstSentence(paragraphs):
    text = paragraphs[0]
    m = re.search(r"(?<=[a-z0-9)\"'])\.(\s|$)", text)
    return (text[:m.start() + 1] if m else text).strip()


def report():
    out = subprocess.run([sys.executable, str(Path(__file__).parent / "scan.py"), "--check"],
                         capture_output=True, text=True).stdout
    return out.splitlines()


def plan():
    """Every edit, grouped by file and ordered bottom-up so line numbers stay good."""
    edits, refused = {}, []
    for line in report():
        m = RE_MULTILINE.match(line)
        if m:
            path, start, name, count = ROOT / m.group(1), int(m.group(2)), m.group(3), int(m.group(4))
            edits.setdefault(path, []).append(("block", start, name, count))
            continue
        m = RE_FITS.match(line)
        if m:
            path, at, name = ROOT / m.group(1), int(m.group(2)), m.group(3)
            edits.setdefault(path, []).append(("inline", at, name, 1))
    for path in edits:
        edits[path].sort(key=lambda e: -e[1])
    return edits, refused


SUMMARIES = Path(__file__).parent / "summaries.txt"


def summaries():
    """Hand-written one-liners for the declarations whose own first sentence will not fit.
    One per line: <path relative to the repo>:<line>|<the one line>."""
    given = {}
    if SUMMARIES.exists():
        for line in SUMMARIES.read_text(encoding="utf-8").splitlines():
            if not line.strip() or line.lstrip().startswith("#"):
                continue
            where, _, text = line.partition("|")
            path, _, at = where.rpartition(":")
            given[(ROOT / path.strip(), int(at))] = text.strip()
    return given


def convert(dryRun):
    edits, refused = plan()
    given = summaries()
    additions = {}
    written = 0
    for path, items in sorted(edits.items()):
        lines = path.read_text(encoding="utf-8").replace("\r\n", "\n").split("\n")
        note = noteOf(path)
        for kind, at, name, count in items:
            if kind == "inline":
                comment = lines[at - 2]
                indent = comment[:len(comment) - len(comment.lstrip())]
                text = bodyOf(comment)
                label = text[0].lower() + text[1:] if text[:2].islower() or text[:1].isupper() else text
                label = label.rstrip(".")
                merged = f"{lines[at - 1]} // {label}"
                if len(merged) > MAX_COLUMNS:
                    refused.append((path, at, name, "would not fit after all"))
                    continue
                lines[at - 1] = merged
                del lines[at - 2]
                written += 1
                continue

            block = lines[at - 1: at - 1 + count]
            indent = block[0][:len(block[0]) - len(block[0].lstrip())]
            paragraphs, current = [], []
            for raw in block:
                body = bodyOf(raw)
                if not body:
                    if current:
                        paragraphs.append(" ".join(current))
                        current = []
                    continue
                current.append(body)
            if current:
                paragraphs.append(" ".join(current))
            if not paragraphs:
                refused.append((path, at, name, "nothing but markers"))
                continue
            summary = given.get((path, at)) or firstSentence(paragraphs)
            if (path, at) not in given and ("{" in summary or "}" in summary):
                refused.append((path, at, name, "opens with a usage example, not a sentence"))
                continue
            whole = " ".join(paragraphs)
            needsNote = whole.strip() != summary.strip()
            reference = f" See {note}" if needsNote else ""
            oneLine = f"{indent}// {summary}{reference}"
            if len(oneLine) > MAX_COLUMNS:
                refused.append((path, at, name, f"first sentence needs {len(oneLine)} columns"))
                continue
            lines[at - 1: at - 1 + count] = [oneLine]
            written += 1
            if needsNote:
                additions.setdefault(note, []).append((name, block, path))
        if not dryRun:
            path.write_text("\n".join(lines).replace("\n", "\r\n"), encoding="utf-8", newline="")

    for note, entries in additions.items():
        target = NOTES / f"{note}.md"
        text = target.read_text(encoding="utf-8").replace("\r\n", "\n") if target.exists() else \
            f"# {note.replace('-', ' ')}\n\nThe words that no longer fit above a declaration.\n"
        seen = {slug(h.lstrip('#').strip()) for h in text.splitlines() if h.startswith("#")}
        for name, block, path in reversed(entries):
            heading = name
            if slug(heading) in seen:
                heading = f"{name} in {path.stem}"
            seen.add(slug(heading))
            body = [noteBody(raw) for raw in block]
            text += f"\n## {heading}\n\n" + "\n".join(body).replace("\n\n\n", "\n\n") + "\n"
        if not dryRun:
            target.write_text(text.replace("\n", "\r\n"), encoding="utf-8", newline="")

    print(f"{written} comments condensed, {len(additions)} notes touched, {len(refused)} refused\n")
    for path, at, name, why in refused:
        print(f"  REFUSED {path.relative_to(ROOT).as_posix()}:{at} {name} - {why}")
    return refused


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--show", type=int, default=0, help="print this many proposed one-liners")
    args = ap.parse_args()
    if args.show:
        edits, _ = plan()
        shown = 0
        for path, items in sorted(edits.items()):
            lines = path.read_text(encoding="utf-8").replace("\r\n", "\n").split("\n")
            for kind, at, name, count in sorted(items, key=lambda e: e[1]):
                if kind != "block" or shown >= args.show:
                    continue
                block = lines[at - 1: at - 1 + count]
                paragraphs, current = [], []
                for raw in block:
                    b = bodyOf(raw)
                    if not b:
                        if current: paragraphs.append(" ".join(current)); current = []
                        continue
                    current.append(b)
                if current: paragraphs.append(" ".join(current))
                if not paragraphs: continue
                s = firstSentence(paragraphs)
                indent = block[0][:len(block[0]) - len(block[0].lstrip())]
                width = len(indent) + 3 + len(s) + (len(f" See {noteOf(path)}") if " ".join(paragraphs).strip() != s else 0)
                flag = "  <-- TOO WIDE" if width > MAX_COLUMNS else ""
                print(f"{path.stem}:{at} {name}\n    {s}{flag}")
                shown += 1
        sys.exit(0)
    convert(args.dry_run)


def eventConnectors(dryRun):
    """A DECLARE_EVENT line with no comment takes the one the event type already carries.
    The connector fires when the event happens, so the two say the same thing by construction."""
    described = {}
    for path in list(SOURCE.rglob("*.cppm")) + list(SOURCE.rglob("*.cpp")):
        lines = path.read_text(encoding="utf-8", errors="replace").replace("\r\n", "\n").split("\n")
        for i, line in enumerate(lines):
            m = re.match(r"^\s*export\s+(?:class|struct)\s+(\w+Event)\b", line)
            if m and i and lines[i - 1].strip().startswith("//"):
                text = bodyOf(lines[i - 1])
                if text and not text.startswith("TODO"):
                    described.setdefault(m.group(1), text)

    out = subprocess.run([sys.executable, str(Path(__file__).parent / "scan.py"), "--check"],
                         capture_output=True, text=True).stdout
    rows = [(ROOT / m.group(1), int(m.group(2)))
            for m in (re.match(r"^(.+?):(\d+): warning: [\w:*]+ has no comment", l)
                      for l in out.splitlines()) if m]
    byFile = {}
    for path, at in rows:
        byFile.setdefault(path, []).append(at)

    done, missing = 0, []
    for path, ats in byFile.items():
        lines = path.read_text(encoding="utf-8").replace("\r\n", "\n").split("\n")
        for at in sorted(ats, reverse=True):
            decl = lines[at - 1]
            m = re.search(r"DECLARE_EVENT\s*\(\s*(\w+)", decl)
            if not m:
                continue
            text = described.get(m.group(1))
            if not text:
                missing.append((path, at, m.group(1)))
                continue
            label = (text[0].lower() + text[1:]).rstrip(".")
            trailing = f"{decl} // {label}"
            if len(trailing) <= MAX_COLUMNS:
                lines[at - 1] = trailing
            else:
                indent = decl[:len(decl) - len(decl.lstrip())]
                above = f"{indent}// {text}"
                if len(above) > MAX_COLUMNS:
                    missing.append((path, at, m.group(1) + " - too wide either way"))
                    continue
                lines.insert(at - 1, above)
            done += 1
        if not dryRun:
            path.write_text("\n".join(lines).replace("\n", "\r\n"), encoding="utf-8", newline="")
    print(f"{done} event connectors described from their event type, {len(missing)} left")
    for m in missing:
        print(f"  {m[0].relative_to(ROOT).as_posix()}:{m[1]} {m[2]}")


INSERTS = Path(__file__).parent / "inserts.txt"


def insertComments(dryRun=False):
    """Comments written by hand for declarations that carry none.
    One per line: <path>:<line>|<the one line>. A scope opener takes it above, anything else
    takes it on the line while it fits."""
    if not INSERTS.exists():
        print("nothing to insert")
        return
    byFile = {}
    for line in INSERTS.read_text(encoding="utf-8").splitlines():
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        where, _, text = line.partition("|")
        path, _, at = where.rpartition(":")
        byFile.setdefault(ROOT / path.strip(), []).append((int(at), text.strip()))

    done, refused = 0, []
    for path, items in byFile.items():
        lines = path.read_text(encoding="utf-8").replace("\r\n", "\n").split("\n")
        for at, text in sorted(items, key=lambda e: -e[0]):
            decl = lines[at - 1]
            indent = decl[:len(decl) - len(decl.lstrip())]
            opensScope = re.search(r"\b(class|struct|enum)\b", decl) is not None
            if not opensScope:
                label = (text[0].lower() + text[1:]).rstrip(".")
                trailing = f"{decl} // {label}"
                if len(trailing) <= MAX_COLUMNS:
                    lines[at - 1] = trailing
                    done += 1
                    continue
            above = f"{indent}// {text}"
            if len(above) > MAX_COLUMNS:
                refused.append((path, at, len(above)))
                continue
            lines.insert(at - 1, above)
            done += 1
        if not dryRun:
            path.write_text("\n".join(lines).replace("\n", "\r\n"), encoding="utf-8", newline="")
    print(f"{done} comments written, {len(refused)} too wide")
    for path, at, width in refused:
        print(f"  {path.relative_to(ROOT).as_posix()}:{at} needs {width} columns")
