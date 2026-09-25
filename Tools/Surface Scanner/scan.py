"""Harvests the ClaFi design surface out of the source and checks the declaration routine.

The notes a comment refers to live in Source/RawDocs.
Reads lines. Does not parse C++, does not expand macros, does not follow templates.

    python scan.py            writes surface.json
    python scan.py --check    reports deviations, exit code 1 if any are errors
"""

import argparse
import json
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent
SOURCE = ROOT / "Source"
NOTES = ROOT / "Source" / "RawDocs"
MAX_COLUMNS = 100

RE_MODULE = re.compile(r"^\s*export\s+module\s+([\w.:]+)\s*;")
RE_NAMESPACE = re.compile(r"^\s*namespace\s+([\w:]+)")
RE_CLASS = re.compile(r"^\s*export\s+(class|struct)\s+(\w+)\s*(?::\s*(.+?))?\s*(\{)?\s*$")
RE_ONE_LINE_TYPE = re.compile(r"^\s*export\s+(?:class|struct)\s+(\w+)\s*(?::\s*(?:public\s+)?([\w:<>]+)\s*)?\{(.*)\}\s*;")
RE_EXPORT_TEMPLATE = re.compile(r"^\s*export\s+template\s*<")
RE_CLASS_LINE = re.compile(r"^\s*(class|struct)\s+(\w+)\s*(?::\s*(.+?))?\s*(\{)?\s*$")
RE_ENUM = re.compile(r"^\s*export\s+enum\s+class\s+(\w+)")
RE_BASE = re.compile(r"(?:public|protected|private)?\s*([\w:]+(?:<[^>]*>)?)")
RE_PROPERTY = re.compile(r"\bDECLARE_(PROPERTY_STORAGE|WRITABLE_PROPERTY|REF_PROPERTY|PROPERTY)\s*\((.*)\)\s*$")
RE_BIND = re.compile(r"\bBIND_PROPERTY_(MEMBER|CALL|VALUE|ACTION)\s*\((.*)\)\s*;")
RE_READ = re.compile(r"\bREAD_PROPERTY\s*\(")
RE_QUALIFIED = re.compile(r"^\s*(?:template\s*<.*>\s*)?[\w:<>&*\[\]\s]*?(\w+)::~?\w+\s*\(")
RE_MEMBER_FN = re.compile(r"\b(\w+)\s*\(")
RE_EVENT = re.compile(r"\bDECLARE_EVENT\s*\(\s*(\w+)\s*,\s*(\w+)\s*,\s*(\w+)\s*\)")
RE_INIT_PROPERTY = re.compile(r"\bINIT_PROPERTY\s*\(\s*(\w+)\s*\)")
RE_BIND_MEMBER = re.compile(r"\bBIND_(?:MEMBER|CALL|CALL_VALUE|ACTION)\s*\(\s*([\w:\s\*&<>]+?)\s*,")
RE_PROPS_GET = re.compile(r"\bProps::(?:get|find)\s*<?\(")
RE_ALIAS = re.compile(r"^\s*(?:export\s+)?using\s+(\w+)\s*=\s*([\w:<>,*& ]+?)\s*;")
RE_SEE = re.compile(r"\bSee\s+([\w.\- ]+?)(#[\w\-]+)?\s*$")
RE_DATA_MEMBER = re.compile(r"^\s*(?!using|friend|static|explicit|virtual|return)((?:const\s+)?[\w:]+(?:<[^>]*>)?\s*[\*&]{0,2})\s+(\w+)\s*(\{[^;]*\})?\s*;\s*$")

VALUE_KINDS = {
    "bool": "bool", "int": "int", "float": "float", "double": "float",
    "std::size_t": "int", "std::uint32_t": "int", "std::int32_t": "int",
    "std::uintptr_t": "int", "std::uint8_t": "int", "std::uint64_t": "int",
    "std::wstring": "text", "std::wstring_view": "text", "std::string": "text",
    "wchar_t": "text", "char": "text",
    "CustomFloatPoint": "float2", "FloatPoint": "float2", "IntPoint": "int2",
    "FloatRect": "float4", "IntRect": "int4", "Color": "color", "Rgb": "color",
}


def slug(text):
    """A markdown heading anchor, the way a renderer makes one."""
    return re.sub(r"[^a-z0-9]+", "-", text.split("::")[-1].lower()).strip("-")


def headings(path):
    """Every anchor the note offers."""
    return {slug(line.lstrip("#").strip())
            for line in path.read_text(encoding="utf-8", errors="replace").splitlines()
            if line.startswith("#")}


class Report:
    def __init__(self):
        self.entries = []

    def add(self, level, path, line, text):
        rel = path.relative_to(ROOT).as_posix() if path else ""
        self.entries.append((level, rel, line, text))

    def errors(self):
        return [e for e in self.entries if e[0] == "error"]

    def dump(self):
        for level, rel, line, text in sorted(self.entries, key=lambda e: (e[1], e[2])):
            print(f"{rel}:{line}: {level}: {text}")


def strip_code(line, in_block):
    """The line with comments and literals removed, the block-comment state after it, and the
    trailing comment, if the line carries code and a comment both."""
    out = []
    trailing = None
    i = 0
    while i < len(line):
        two = line[i:i + 2]
        if in_block:
            if two == "*/":
                in_block = False
                i += 2
                continue
            i += 1
            continue
        if two == "//":
            if "".join(out).strip():
                trailing = line[i:].lstrip("/").strip()
            break
        if two == "/*":
            in_block = True
            i += 2
            continue
        if line[i] in "\"'":
            quote = line[i]
            i += 1
            while i < len(line):
                if line[i] == "\\":
                    i += 2
                    continue
                if line[i] == quote:
                    i += 1
                    break
                i += 1
            out.append(" ")
            continue
        out.append(line[i])
        i += 1
    return "".join(out), in_block, trailing


def split_args(text):
    """Top level comma split, so a braced default carrying commas stays one argument."""
    parts, depth, current = [], 0, []
    for ch in text:
        if ch in "({[<":
            depth += 1
        elif ch in ")}]>":
            depth -= 1
        if ch == "," and depth == 0:
            parts.append("".join(current).strip())
            current = []
            continue
        current.append(ch)
    parts.append("".join(current).strip())
    return parts


def base_names(clause):
    if not clause:
        return []
    return [m.group(1) for m in RE_BASE.finditer(clause) if m.group(1) not in ("public", "protected", "private")]


class Scanner:
    def __init__(self, report):
        self.report = report
        self.missing = []
        self.constants = set()
        self.commentWidth = {}
        self.aliases = {}
        self.classes = {}
        self.enums = {}
        self.structs = {}
        self.files = []

    def scan_file(self, path):
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
        module = ""
        namespace = "ClaFi"
        comment, comment_start = [], 0
        in_block = False
        depth = 0
        stack = []            # (name, depth at which the body opened)
        pending = None        # class awaiting its Allman brace
        outer = None          # class a definition written at namespace scope belongs to
        exportedTemplate = False  # an export template line, whose class comes on the next line
        templateParams = set()    # its parameter names, which are bases no chain can be walked through
        enum = None
        binds = set()
        props_get_lines = []

        for number, raw in enumerate(lines, 1):
            code, in_block, trailing = strip_code(raw, in_block)
            line_width = len(raw.rstrip())
            stripped = raw.strip()

            if stripped.startswith("//"):
                if not comment:
                    comment_start = number
                    self.commentWidth[(path, number)] = 0
                self.commentWidth[(path, comment_start)] = max(self.commentWidth[(path, comment_start)], len(raw.rstrip()))
                body = re.sub(r"^@brief\s+", "", stripped.lstrip("/").strip())
                if not body.startswith("TODO"):
                    comment.append(body)
                continue

            if not stripped:
                comment, comment_start = [], 0

            m = RE_MODULE.match(code)
            if m:
                module = m.group(1)
            m = RE_NAMESPACE.match(code)
            if m:
                namespace = m.group(1)
            m = RE_ALIAS.match(code)
            if m:
                self.aliases[m.group(1)] = m.group(2)

            for m in RE_INIT_PROPERTY.finditer(code):
                binds.add(m.group(1))
            for m in RE_BIND_MEMBER.finditer(code):
                binds.add(m.group(1))
            if RE_PROPS_GET.search(code):
                props_get_lines.append(number)
            for m in re.finditer(r"\bs_(\w+)Default\b", code):
                self.constants.add(f"s_{m.group(1)}Default")

            m = RE_QUALIFIED.match(code)
            if m and (m.group(1) in self.classes or m.group(1) in self.structs):
                outer = m.group(1)
            owner = stack[-1][0] if stack else outer
            if owner:
                target = self.classes.get(owner) or self.structs.get(owner)
                if target is not None:
                    for m in RE_MEMBER_FN.finditer(code):
                        target["getters"].add(m.group(1))

            m = RE_PROPERTY.search(code)
            if m and owner:
                self.add_property(path, number, owner, m.group(1), m.group(2), comment, comment_start,
                                  trailing, line_width)
                comment, comment_start = [], 0

            m = RE_BIND.search(code)
            if m and owner:
                self.add_binding(path, number, owner, m.group(1).lower(), split_args(m.group(2)),
                                 comment, comment_start, trailing, line_width)

            if RE_READ.search(code) and owner:
                self.add_read(path, number, owner, code, comment, comment_start, trailing, line_width)

            m = RE_EVENT.search(code)
            if m and owner:
                self.add_event(path, number, owner, m.groups(), comment, comment_start,
                               trailing, line_width)
                comment, comment_start = [], 0

            m = RE_ENUM.match(code)
            if m:
                enum = {"name": m.group(1), "namespace": namespace, "module": module,
                        "file": path, "line": number, "members": [],
                        "description": self.describe(path, m.group(1), comment, comment_start, number,
                                                    trailing, line_width, True)}
                self.enums[m.group(1)] = enum
                # An enum whose brace is on its own line is pushed below; one that opens on the
                # declaration line has to be pushed here, or its members are never read.
                if "{" in code:
                    pending = "#enum"
                comment, comment_start = [], 0
            elif enum is not None and stack and stack[-1][0] == "#enum":
                member = re.match(r"^\s*(\w+)\s*(=[^,]*)?,?\s*$", code)
                if member:
                    enum["members"].append({
                        "name": member.group(1),
                        "description": self.describe(path, f"{enum['name']}::{member.group(1)}",
                                                     comment, comment_start, number, trailing, line_width)["text"]})
                    comment, comment_start = [], 0

            m = RE_ONE_LINE_TYPE.match(code)
            if m:
                self.structs.setdefault(m.group(1), {
                    "name": m.group(1), "kind": "struct", "namespace": namespace, "module": module,
                    "file": path, "line": number, "bases": [m.group(2)] if m.group(2) else [],
                    "properties": [], "events": [],
                    "payload": [{"type": d.group(1).strip(), "name": d.group(2), "description": ""}
                                for d in [RE_DATA_MEMBER.match("    " + part.strip() + ";")
                                          for part in (m.group(3) or "").split(";") if part.strip()] if d],
                    "getters": set(),
                    "description": {"text": "", "note": None}})

            m = RE_CLASS.match(code)
            if m is None and exportedTemplate:
                m = RE_CLASS_LINE.match(code)
            if RE_EXPORT_TEMPLATE.match(code):
                exportedTemplate = True
                templateParams = set(re.findall(r"(?:class|typename|\w+)\s+(\w+)\s*(?:,|>|$)", code))
            else:
                exportedTemplate = False
            if m and not code.rstrip().endswith(";"):
                kind, name, bases = m.group(1), m.group(2), m.group(3)
                entry = {"name": name, "kind": kind, "namespace": namespace, "module": module,
                         "file": path, "line": number, "bases": base_names(bases),
                         "properties": [], "events": [], "payload": [], "getters": set(),
                         "templateParams": templateParams if exportedTemplate or pending is None else set(),
                         "description": self.describe(path, name, comment, comment_start, number,
                                          trailing, line_width, True)}
                table = self.classes if kind == "class" else self.structs
                if name in table and table[name]["namespace"] != namespace:
                    self.report.add("warning", path, number,
                                    f"{name} is also declared in {table[name]['namespace']}, and the database keys on the name")
                table[name] = entry
                pending = name
                comment, comment_start = [], 0
            elif enum is not None and pending is None and code.strip().startswith("{") and not stack:
                pending = "#enum"

            opens = code.count("{")
            closes = code.count("}")
            if opens:
                if pending:
                    stack.append((pending, depth))
                    pending = None
                depth += opens
            if closes:
                depth -= closes
                while stack and stack[-1][1] >= depth:
                    if stack[-1][0] == "#enum":
                        enum = None
                    stack.pop()

            if stack and self.structs.get(stack[-1][0]):
                self.add_payload(stack[-1][0], code, comment, comment_start, path, number, trailing,
                                 line_width, stack[-1][0].endswith("Event"))

            # A comment belongs to the line it touches. Whatever this line was, the next
            # declaration does not inherit it.
            comment, comment_start = [], 0

        self.files.append({"path": path, "module": module, "binds": binds, "props_get": props_get_lines})

    def describe(self, path, name, comment, start, line=0, trailing=None, width=0, opensScope=False):
        if trailing:
            if comment:
                self.report.add("error", path, line,
                                f"{name} carries a comment above the declaration and one on it")
            if opensScope:
                self.report.add("error", path, line,
                                f"{name} opens a scope and its comment belongs above the declaration")
            if width > MAX_COLUMNS:
                self.report.add("error", path, line,
                                f"{name} runs to {width} columns, {MAX_COLUMNS} is the limit - the comment goes above")
            return self.reference(path, line, trailing, name)
        if not comment:
            self.missing.append((path, line, name))
            return {"text": "", "note": None}
        if len(comment) > 1:
            self.report.add("error", path, start, f"{name} has a comment of {len(comment)} lines, one is the maximum")
        elif self.commentWidth.get((path, start), 0) > MAX_COLUMNS:
            self.report.add("error", path, start,
                            f"the comment on {name} runs to {self.commentWidth[(path, start)]} columns, {MAX_COLUMNS} is the limit")
        elif not opensScope and width and width + len(" // ") + len(comment[0]) <= MAX_COLUMNS:
            self.report.add("warning", path, line,
                            f"{name} fits its comment on the declaration line, in {width + 4 + len(comment[0])} columns")
        return self.reference(path, start, comment[0], name)

    def reference(self, path, start, text, name):
        note = None
        m = RE_SEE.search(text)
        if m:
            stem = m.group(1).strip()
            note = {"note": stem, "anchor": (m.group(2) or "")[1:] or slug(name)}
            text = text[:m.start()].strip()
            target = NOTES / f"{stem}.md"
            if not target.exists():
                self.report.add("error", path, start, f"reference to a note that does not exist: {stem}")
            elif note["anchor"] not in headings(target):
                self.report.add("error", path, start,
                                f"{stem} has no section the reference can reach: #{note['anchor']}")
        return {"text": text, "note": note}

    def add_property(self, path, number, owner, form, arguments, comment, start, trailing=None, width=0):
        args = split_args(arguments)
        writable = form == "WRITABLE_PROPERTY"
        wanted = 4 if writable else 3
        if len(args) < wanted:
            self.report.add("error", path, number,
                            "property declaration takes a type, a name"
                            + (", a setter" if writable else "") + " and a default")
            return
        setter = args[2] if writable else None
        default = ", ".join(args[wanted - 1:])
        entry = {"type": args[0], "name": args[1], "default": default, "setter": setter,
                 "byReference": form == "REF_PROPERTY", "ownGetter": form == "PROPERTY_STORAGE",
                 "boundBy": "", "target": f"m_{args[1]}", "file": path, "line": number,
                 "description": self.describe(path, args[1], comment, start, number, trailing, width)}
        target = self.classes.get(owner) or self.structs.get(owner)
        if target is None:
            self.report.add("error", path, number, f"property {args[1]} declared outside a class")
            return
        target["properties"].append(entry)

    def add_binding(self, path, number, owner, form, args, comment, start, trailing, width):
        target = args[1] if len(args) > 1 else ""
        target = target.split(" = ")[0].strip()
        name = args[0].replace("const", "").replace("*", "").replace("&", "").strip()
        name = name.split("::")[-1]
        self.record(path, number, owner, {
            "type": args[0], "name": name, "default": "", "setter": None,
            "byReference": False, "ownGetter": False, "boundBy": form, "target": target,
            "file": path, "line": number,
            "description": self.describe(path, name, comment, start, number, trailing, width)})

    def add_read(self, path, number, owner, code, comment, start, trailing, width):
        """READ_PROPERTY(type, default) inside whatever expression the constructor writes."""
        at = code.index("READ_PROPERTY")
        i = code.index("(", at) + 1
        depth, current = 1, []
        while i < len(code) and depth:
            if code[i] == "(":
                depth += 1
            elif code[i] == ")":
                depth -= 1
                if not depth:
                    break
            current.append(code[i])
            i += 1
        args = split_args("".join(current))
        if len(args) < 2:
            self.report.add("error", path, number, "READ_PROPERTY takes a type and a default")
            return
        name = args[0].strip()
        self.record(path, number, owner, {
            "type": args[0], "name": name, "default": ", ".join(args[1:]), "setter": None,
            "byReference": False, "ownGetter": False, "boundBy": "read", "target": name,
            "file": path, "line": number,
            "description": self.describe(path, name, comment, start, number, trailing, width)})

    def record(self, path, number, owner, entry):
        target = self.classes.get(owner) or self.structs.get(owner)
        if target is None:
            self.report.add("error", path, number, f"property {entry['name']} declared outside a class")
            return
        target["properties"].append(entry)

    def add_event(self, path, number, owner, groups, comment, start, trailing=None, width=0):
        event_type, alias, method = groups
        entry = {"event": event_type, "alias": alias, "method": method,
                 "file": path, "line": number,
                 "description": self.describe(path, event_type, comment, start, number, trailing, width)}
        target = self.classes.get(owner) or self.structs.get(owner)
        if target is None:
            self.report.add("error", path, number, f"event {event_type} declared outside a class")
            return
        if not event_type.endswith("Event"):
            self.report.add("error", path, number, f"{event_type} is an event type and its name does not end in Event")
        if not alias.startswith("On"):
            self.report.add("error", path, number, f"handler alias is {alias} and a handler alias starts with On")
        expected_method = alias[0].lower() + alias[1:]
        if method != expected_method:
            self.report.add("error", path, number, f"connect method is {method}, {expected_method} is what the alias says")
        target["events"].append(entry)

    def add_payload(self, name, code, comment, start, path, number, trailing, width, isEvent):
        m = RE_DATA_MEMBER.match(code)
        if not m:
            return
        # A value wrapper's members are read for its shape alone - only an event's payload is
        # something a reader of the manual ever sees.
        text = ""
        if isEvent:
            text = self.describe(path, f"{name}::{m.group(2)}", comment, start or number, number, trailing, width)["text"]
        self.structs[name]["payload"].append({
            "type": m.group(1).strip(), "name": m.group(2), "description": text})

    def resolve_value_kind(self, type_name):
        bare = self.resolve(type_name.replace("const ", "").replace("&", "").replace("*", "").strip())
        # A pointer to member is the kind of what it points AT: the class it reaches into says
        # where the value lives, not what the value is.
        if "::*" in bare:
            bare = bare.split()[0]
        if bare in VALUE_KINDS:
            return VALUE_KINDS[bare]
        bare = bare.split("::")[-1]
        if bare in VALUE_KINDS:
            return VALUE_KINDS[bare]
        if bare in self.enums:
            return "enum"
        entry = self.structs.get(bare) or self.classes.get(bare)
        if entry:
            for base in entry["bases"]:
                if base in VALUE_KINDS:
                    return VALUE_KINDS[base]
                if base.startswith("NumericValueWrapper"):
                    inner = base[base.find("<") + 1:].split(",")[0].strip()
                    return VALUE_KINDS.get(inner, "number")
            if len(entry["payload"]) == 1:
                kind = VALUE_KINDS.get(entry["payload"][0]["type"])
                if kind:
                    return kind
            # A type the tree declares and nothing reduces to a primitive: the designer tool
            # shows it as an object of its own, not as a value it can type into a box.
            return "object"
        return "unknown"

    def resolve(self, name):
        """The class a name stands for, through export using aliases and template spellings."""
        seen = set()
        while name in self.aliases and name not in seen:
            seen.add(name)
            name = self.aliases[name]
        return name

    def is_control(self, entry):
        seen = set()
        queue = [entry["name"]] + list(entry["bases"])
        while queue:
            base = self.resolve(queue.pop())
            arguments = []
            if "<" in base:
                arguments = [a.strip() for a in base[base.find("<") + 1:].rstrip(">").split(",")]
                base = base[:base.find("<")]
            if base in seen:
                continue
            seen.add(base)
            if base in ("Control", "RichControl"):
                return True
            queue.extend(arguments)
            parent = self.classes.get(base)
            if parent:
                queue.extend(parent["bases"])
        return False

    def check_bindings(self):
        bound = set()
        for entry in self.files:
            bound |= entry["binds"]
        declared = set()
        for entry in self.classes.values():
            for prop in entry["properties"]:
                declared.add(prop["name"])
                if prop["boundBy"]:
                    pass
                elif prop["name"] not in bound and f"m_{prop['name']}" not in bound:
                    self.report.add("error", prop["file"], prop["line"],
                                    f"property {prop['name']} has no INIT_PROPERTY")
                if prop["ownGetter"] and prop["name"] not in entry["getters"]:
                    self.report.add("warning", prop["file"], prop["line"],
                                    f"property {prop['name']} declares its storage and the class has no {prop['name']}() getter")
                if prop["setter"] and prop["setter"] not in entry["getters"]:
                    self.report.add("warning", prop["file"], prop["line"],
                                    f"property {prop['name']} names {prop['setter']} and the class does not have it")
                kind = self.resolve_value_kind(prop["type"])
                if kind == "unknown":
                    self.report.add("warning", prop["file"], prop["line"],
                                    f"property {prop['name']} has no known value kind for {prop['type']}")
        for entry in self.files:
            for number in entry["props_get"]:
                self.report.add("warning", entry["path"], number,
                                "Props::get outside a declared property")

    def check_comments(self):
        surface = set()
        for entry in self.classes.values():
            if not self.is_control(entry):
                continue
            surface.add(entry["name"])
            for prop in entry["properties"]:
                surface.add(prop["name"])
            for event in entry["events"]:
                surface.add(event["event"])
        surface |= set(self.enums)
        for path, line, name in self.missing:
            if "::" in name:
                continue
            if name in surface:
                self.report.add("warning", path, line, f"{name} has no comment")

    def check_chains(self):
        for entry in self.classes.values():
            if self.is_control(entry) or not entry["bases"]:
                continue
            unknown = [b for b in entry["bases"]
                       if self.resolve(b).split("<")[0] not in self.classes
                       and self.resolve(b).split("<")[0] not in self.structs
                       and b not in entry.get("templateParams", set())]
            if unknown and (entry["properties"] or entry["events"]):
                self.report.add("warning", entry["file"], entry["line"],
                                f"{entry['name']} declares a surface and its base chain does not reach Control: {', '.join(unknown)}")

    def check_base_types(self):
        """A property type declared beside a base class reaches an application only through an
        import of that base. Using Button is not a reason to import ButtonBase."""
        base = (SOURCE / "Controls" / "Base")
        for entry in list(self.enums.values()) + list(self.structs.values()):
            if entry["name"].endswith("Event") or not entry.get("file"):
                continue
            if base not in entry["file"].parents:
                continue
            self.report.add("warning", entry["file"], entry["line"],
                            f"{entry['name']} is a property type declared beside a base class - "
                            f"it belongs in UiTypes, or in TextEngine.Text if it is a text")

    def check_events(self):
        for entry in self.classes.values():
            for event in entry["events"]:
                if event["event"] not in self.structs and event["event"] not in self.classes:
                    self.report.add("warning", event["file"], event["line"],
                                    f"no declaration found for event type {event['event']}")

    def merge(self, properties):
        """One property per place the value lands. Several bindings on one target are one
        property that accepts several spellings - a control's text takes Text, a view and a
        literal, and that is one row in the database, not three."""
        out, byTarget = [], {}
        for prop in properties:
            entry = {k: v for k, v in prop.items() if k != "file"}
            entry["valueKind"] = self.resolve_value_kind(prop["type"])
            key = prop.get("target") or prop["name"]
            if prop.get("boundBy") and key in byTarget:
                byTarget[key].setdefault("accepts", [byTarget[key]["type"]]).append(prop["type"])
                continue
            byTarget[key] = entry
            out.append(entry)
        return out

    def database(self):
        controls = []
        for entry in self.classes.values():
            if not self.is_control(entry):
                continue
            relative = entry["file"].relative_to(SOURCE)
            controls.append({
                "name": entry["name"], "module": entry["module"],
                "namespace": entry["namespace"], "bases": entry["bases"],
                "category": relative.parent.as_posix(),
                "file": relative.as_posix(), "line": entry["line"],
                "description": entry["description"],
                "properties": self.merge(entry["properties"]),
                "events": [{k: v for k, v in e.items() if k != "file"} | {"payload": self.structs.get(e["event"], {}).get("payload", [])}
                           for e in entry["events"]],
            })
        controls.sort(key=lambda c: c["name"])
        value_types = [{"name": e["name"], "kind": "enum", "module": e["module"],
                        "description": e["description"],
                        "members": e["members"]} for e in self.enums.values() if e["members"]]
        value_types.sort(key=lambda v: v["name"])
        return {"controls": controls, "valueTypes": value_types}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="report deviations instead of writing the database")
    parser.add_argument("--out", default=str(Path(__file__).resolve().parent / "surface.json"))
    args = parser.parse_args()

    report = Report()
    scanner = Scanner(report)
    for path in sorted(SOURCE.rglob("*.cppm")) + sorted(SOURCE.rglob("*.cpp")):
        scanner.scan_file(path)
    scanner.check_bindings()
    scanner.check_events()
    scanner.check_comments()
    scanner.check_chains()
    scanner.check_base_types()

    if args.check:
        report.dump()
        errors = len(report.errors())
        print(f"\n{len(scanner.classes)} classes, {errors} errors, {len(report.entries) - errors} warnings")
        return 1 if errors else 0

    database = scanner.database()
    Path(args.out).write_text(json.dumps(database, indent=2), encoding="utf-8")
    print(f"{len(database['controls'])} controls, {len(database['valueTypes'])} value types -> {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
