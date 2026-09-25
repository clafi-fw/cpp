"""Builds the control reference out of surface.json - the smoke test for the whole routine.

    python "Tools/Surface Scanner/scan.py"          writes surface.json
    python "Tools/Surface Scanner/build_docs.py"    writes Controls-Reference.html

Self-contained HTML, in the same vocabulary as the manuals under Documentation.
Nothing here is written by hand: every name, type, default and sentence on the page came out
of a declaration, so a gap on the page is a gap in the source.
"""

import html
import json
from datetime import date
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent.parent
DATA = HERE / "surface.json"
OUT = HERE / "Controls-Reference.html"
NOTES = "../../Source/RawDocs"


NOCOMMENT = '<span class="missing">no comment</span>'
DASH = '<span class="muted">&mdash;</span>'


def esc(text):
    return html.escape(text or "")


def anchor(kind, name):
    return f"{kind}-{name.replace('::', '-')}"


def described(entry):
    """The one-line comment, and the note it points at."""
    d = entry.get("description") or {}
    text = esc(d.get("text"))
    note = d.get("note")
    if not note:
        return text
    target = f"{NOTES}/{note['note']}.md"
    label = note["note"] + (f"#{note['anchor']}" if note.get("anchor") else "")
    return f'{text} <a class="note" href="{esc(target)}" title="{esc(label)}">{esc(note["note"])}</a>'


def typeLink(typeName, enums):
    bare = typeName.replace("const ", "").replace("*", "").replace("&", "").strip().split("::")[-1]
    if bare in enums:
        return f'<a href="#{anchor("type", bare)}"><code>{esc(typeName)}</code></a>'
    return f"<code>{esc(typeName)}</code>"


def access(prop):
    if prop.get("boundBy"):
        target = prop.get("target") or ""
        return f'<span class="tag construction">at construction</span><br><code class="muted">{esc(target)}</code>'
    if prop.get("setter"):
        return f'<span class="tag rw">get / set</span><br><code class="muted">{esc(prop["setter"])}()</code>'
    return '<span class="tag ro">read only</span>'


def chainOf(control, byName):
    """Every base above this control that the database knows, nearest first."""
    chain, seen = [], {control["name"]}
    queue = list(control.get("bases", []))
    while queue:
        base = queue.pop(0).split("<")[0].strip()
        if base in seen or base not in byName:
            continue
        seen.add(base)
        chain.append(byName[base])
        queue.extend(byName[base].get("bases", []))
    return chain


def propertyRows(properties, enums):
    rows = []
    for p in properties:
        accepts = p.get("accepts")
        types = ", ".join(typeLink(t, enums) for t in accepts) if accepts else typeLink(p["type"], enums)
        default = f'<code>{esc(p["default"])}</code>' if p.get("default") else '<span class="muted">from the storage</span>'
        kind = p.get("valueKind", "")
        rows.append(f"""      <tr>
        <td><code class="name">{esc(p['name'])}</code></td>
        <td>{types}<span class="kind">{esc(kind)}</span></td>
        <td>{default}</td>
        <td>{access(p)}</td>
        <td>{described(p) or NOCOMMENT}</td>
      </tr>""")
    return "\n".join(rows)


def eventRows(events):
    rows = []
    for e in events:
        payload = "".join(
            f'<div><code>{esc(m["type"])} {esc(m["name"])}</code>'
            + (f' <span class="muted">{esc(m["description"])}</span>' if m.get("description") else "")
            + "</div>"
            for m in e.get("payload", [])) or '<span class="muted">nothing</span>'
        rows.append(f"""      <tr>
        <td><code class="name">{esc(e['event'])}</code></td>
        <td><code>{esc(e['method'])}()</code><br><code class="muted">{esc(e['alias'])}</code></td>
        <td class="payload">{payload}</td>
        <td>{described(e) or NOCOMMENT}</td>
      </tr>""")
    return "\n".join(rows)


def table(headers, rows):
    head = "".join(f"<th>{h}</th>" for h in headers)
    return f'<table>\n      <thead><tr>{head}</tr></thead>\n      <tbody>\n{rows}\n      </tbody>\n    </table>'


def build():
    data = json.loads(DATA.read_text(encoding="utf-8"))
    controls = data["controls"]
    valueTypes = data["valueTypes"]
    byName = {c["name"]: c for c in controls}
    enums = {v["name"] for v in valueTypes}

    categories = {}
    for c in controls:
        categories.setdefault(c["category"], []).append(c)

    ownProps = sum(len(c["properties"]) for c in controls)
    ownEvents = sum(len(c["events"]) for c in controls)
    noComment = sum(1 for c in controls for p in c["properties"] if not p["description"]["text"])
    bare = sum(1 for c in controls if not c["properties"] and not c["events"])

    nav = []
    for category in sorted(categories):
        items = "".join(
            f'<li><a href="#{anchor("control", c["name"])}">{esc(c["name"])}</a></li>'
            for c in sorted(categories[category], key=lambda x: x["name"]))
        nav.append(f'<li class="group"><span class="group-name">{esc(category)}</span><ul>{items}</ul></li>')
    nav.append('<li class="group"><span class="group-name">Value types</span><ul>'
               + "".join(f'<li><a href="#{anchor("type", v["name"])}">{esc(v["name"])}</a></li>'
                         for v in valueTypes) + "</ul></li>")

    sections = []
    for category in sorted(categories):
        for c in sorted(categories[category], key=lambda x: x["name"]):
            chain = chainOf(c, byName)
            bases = " &rsaquo; ".join(
                f'<a href="#{anchor("control", b["name"])}">{esc(b["name"])}</a>' for b in chain) \
                or '<span class="muted">nothing this database knows</span>'

            body = [f'<h2 id="{anchor("control", c["name"])}">{esc(c["name"])}</h2>',
                    f'<p class="lede">{described(c) or NOCOMMENT}</p>',
                    '<dl class="facts">'
                    f'<dt>module</dt><dd><code>{esc(c["module"])}</code></dd>'
                    f'<dt>declared</dt><dd><code>{esc(c["file"])}:{c["line"]}</code></dd>'
                    f'<dt>built on</dt><dd>{bases}</dd></dl>']

            if c["properties"]:
                body.append("<h3>Properties</h3>")
                body.append(table(["Property", "Type", "Default", "Access", "What it is"],
                                  propertyRows(c["properties"], enums)))
            if c["events"]:
                body.append("<h3>Events</h3>")
                body.append(table(["Event", "Connect with", "Payload", "When it is raised"],
                                  eventRows(c["events"])))

            own = bool(c["properties"] or c["events"])
            if not own:
                body.append('<p class="empty">Declares no properties and no events of its own '
                            '&mdash; everything it offers comes down the chain.</p>')

            inheritedProps = [(b, b["properties"]) for b in chain if b["properties"]]
            inheritedEvents = [(b, b["events"]) for b in chain if b["events"]]
            if inheritedProps or inheritedEvents:
                inner = []
                for b, props in inheritedProps:
                    inner.append(f'<h4>Properties from <a href="#{anchor("control", b["name"])}">{esc(b["name"])}</a></h4>')
                    inner.append(table(["Property", "Type", "Default", "Access", "What it is"],
                                       propertyRows(props, enums)))
                for b, events in inheritedEvents:
                    inner.append(f'<h4>Events from <a href="#{anchor("control", b["name"])}">{esc(b["name"])}</a></h4>')
                    inner.append(table(["Event", "Connect with", "Payload", "When it is raised"], eventRows(events)))
                count = sum(len(p) for _, p in inheritedProps) + sum(len(e) for _, e in inheritedEvents)
                body.append(f'<details class="inherited"{"" if own else " open"}>'
                            f'<summary>Inherited: {count} more from the base chain</summary>'
                            + "\n".join(inner) + "</details>")

            sections.append('<section class="control">' + "\n    ".join(body) + "</section>")

    for v in valueTypes:
        members = "".join(
            f'<tr><td><code class="name">{esc(m["name"])}</code></td>'
            f'<td>{esc(m["description"]) or DASH}</td></tr>'
            for m in v["members"])
        sections.append(f"""<section class="valuetype">
    <h2 id="{anchor("type", v["name"])}">{esc(v["name"])}<span class="kind">{esc(v["kind"])}</span></h2>
    <p class="lede">{described(v) or NOCOMMENT}</p>
    <dl class="facts"><dt>module</dt><dd><code>{esc(v["module"])}</code></dd></dl>
    {table(["Value", "What it means"], members)}
  </section>""")

    OUT.write_text(PAGE.format(
        generated=date.today().isoformat(),
        controls=len(controls), properties=ownProps, events=ownEvents, types=len(valueTypes),
        members=sum(len(v["members"]) for v in valueTypes),
        noComment=noComment, bare=bare,
        nav="\n".join(nav), sections="\n\n".join(sections)), encoding="utf-8")
    print(f"{len(controls)} controls, {ownProps} properties, {ownEvents} events -> {OUT}")


PAGE = """<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1.0">
<title>ClaFi Controls - Reference</title>
<style>
:root {{
    --bg-color: #f9f9fb;
    --text-color: #333;
    --accent-color: #0066cc;
    --code-bg: #1e1e1e;
    --code-text: #d4d4d4;
    --border-color: #e1e4e8;
    --muted: #6a737d;
    --sidebar-width: 300px;
}}
html {{ scroll-behavior: smooth; }}
body {{
    font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
    line-height: 1.6; color: var(--text-color); background-color: var(--bg-color);
    margin: 0; padding: 0; display: flex;
}}
code {{ font-family: "Cascadia Mono", Consolas, "SF Mono", monospace; font-size: 0.86em; }}
.sidebar {{
    position: fixed; top: 0; left: 0; width: var(--sidebar-width); height: 100vh;
    background-color: #fff; border-right: 1px solid var(--border-color);
    overflow-y: auto; padding: 1.5rem 1rem; box-sizing: border-box; z-index: 100;
}}
.sidebar-header {{ font-size: 1.2rem; font-weight: 600; margin-bottom: 0.75rem; padding-left: 0.5rem; color: #111; }}
#filter {{
    width: 100%; box-sizing: border-box; padding: 0.45rem 0.6rem; margin-bottom: 1rem;
    border: 1px solid var(--border-color); border-radius: 6px; font: inherit; font-size: 0.9rem;
}}
.sidebar ul {{ list-style: none; padding: 0; margin: 0; }}
.sidebar ul ul {{ padding-left: 0.7rem; margin: 0.2rem 0 0.7rem; }}
.group-name {{ font-size: 0.78rem; text-transform: uppercase; letter-spacing: 0.04em; color: var(--muted); font-weight: 600; }}
.sidebar a {{ color: #111; text-decoration: none; display: block; padding: 0.12rem 0.5rem; border-radius: 5px; font-size: 0.9rem; }}
.sidebar a:hover {{ background: rgba(0,0,0,0.04); color: var(--accent-color); }}
main {{ margin-left: var(--sidebar-width); padding: 2.5rem 3rem; max-width: 1100px; }}
h1 {{ margin: 0 0 0.25rem; font-size: 1.9rem; color: #111; }}
.subtitle {{ color: var(--muted); margin: 0 0 1.5rem; }}
.counts {{ display: flex; flex-wrap: wrap; gap: 0.6rem; margin-bottom: 1rem; }}
.count {{ background: #fff; border: 1px solid var(--border-color); border-radius: 8px; padding: 0.6rem 0.9rem; }}
.count b {{ display: block; font-size: 1.5rem; color: #111; line-height: 1.1; }}
.count span {{ font-size: 0.78rem; color: var(--muted); }}
.gaps {{ background: #fffdf5; border: 1px solid #f0e0b0; border-radius: 8px; padding: 0.8rem 1rem; margin-bottom: 2.5rem; font-size: 0.9rem; }}
section {{ background: #fff; border: 1px solid var(--border-color); border-radius: 10px; padding: 1.4rem 1.6rem; margin-bottom: 1.6rem; }}
h2 {{ margin: 0 0 0.3rem; font-size: 1.35rem; color: #111; scroll-margin-top: 1rem; }}
h3 {{ font-size: 0.82rem; text-transform: uppercase; letter-spacing: 0.05em; color: var(--muted); margin: 1.6rem 0 0.5rem; }}
h4 {{ font-size: 0.85rem; color: var(--muted); margin: 1.2rem 0 0.4rem; font-weight: 600; }}
.lede {{ margin: 0 0 0.9rem; }}
.facts {{ display: grid; grid-template-columns: max-content 1fr; gap: 0.15rem 1rem; margin: 0 0 0.5rem; font-size: 0.86rem; }}
.facts dt {{ color: var(--muted); }}
.facts dd {{ margin: 0; }}
table {{ border-collapse: collapse; width: 100%; font-size: 0.88rem; margin-bottom: 0.5rem; }}
th {{ text-align: left; font-size: 0.75rem; text-transform: uppercase; letter-spacing: 0.04em; color: var(--muted); border-bottom: 1px solid var(--border-color); padding: 0.4rem 0.6rem 0.4rem 0; font-weight: 600; }}
td {{ border-bottom: 1px solid #f0f1f3; padding: 0.5rem 0.6rem 0.5rem 0; vertical-align: top; }}
td:first-child, th:first-child {{ padding-left: 0; }}
code.name {{ color: #111; font-weight: 600; }}
code.muted, .muted {{ color: var(--muted); }}
.kind {{ display: inline-block; margin-left: 0.4rem; font-size: 0.68rem; text-transform: uppercase; letter-spacing: 0.04em; color: var(--muted); }}
.tag {{ display: inline-block; font-size: 0.7rem; padding: 0.05rem 0.4rem; border-radius: 4px; text-transform: uppercase; letter-spacing: 0.03em; }}
.tag.ro {{ background: #eef1f4; color: #55606b; }}
.tag.rw {{ background: #e6f0fb; color: #14538a; }}
.tag.construction {{ background: #f3eefb; color: #5b3f8a; }}
.payload div {{ margin-bottom: 0.15rem; }}
a {{ color: var(--accent-color); }}
a.note {{ font-size: 0.78rem; text-decoration: none; border-bottom: 1px dotted var(--accent-color); }}
.missing {{ color: #b04a3a; font-size: 0.85rem; }}
.empty {{ color: var(--muted); font-size: 0.9rem; margin: 0.5rem 0 0; }}
details.inherited {{ margin-top: 1.4rem; border-top: 1px solid var(--border-color); padding-top: 0.8rem; }}
details.inherited summary {{ cursor: pointer; font-size: 0.85rem; color: var(--muted); }}
section.valuetype h2 {{ font-size: 1.1rem; }}
@media (max-width: 900px) {{ .sidebar {{ display: none; }} main {{ margin-left: 0; padding: 1.5rem; }} }}
</style>
</head>
<body>
<nav class="sidebar">
  <div class="sidebar-header">ClaFi</div>
  <input id="filter" type="search" placeholder="Filter&hellip;" autocomplete="off">
  <ul id="nav">
{nav}
  </ul>
</nav>
<main>
  <h1>Controls Reference</h1>
  <p class="subtitle">Generated from <code>surface.json</code> on {generated}. Every line below was
  read out of a declaration - nothing on this page is written by hand.</p>
  <div class="counts">
    <div class="count"><b>{controls}</b><span>controls</span></div>
    <div class="count"><b>{properties}</b><span>declared properties</span></div>
    <div class="count"><b>{events}</b><span>events</span></div>
    <div class="count"><b>{types}</b><span>value types</span></div>
    <div class="count"><b>{members}</b><span>enum members</span></div>
  </div>
  <div class="gaps"><b>What the source has not said yet.</b>
  {noComment} properties carry no comment, and {bare} controls declare neither a property nor an
  event of their own. Both are gaps in the declarations, not in this page.</div>
{sections}
</main>
<script>
const filter = document.getElementById("filter");
filter.addEventListener("input", () => {{
    const q = filter.value.trim().toLowerCase();
    document.querySelectorAll("#nav > li.group").forEach(group => {{
        let shown = 0;
        group.querySelectorAll("li").forEach(item => {{
            const hit = !q || item.textContent.toLowerCase().includes(q);
            item.style.display = hit ? "" : "none";
            if (hit) shown++;
        }});
        group.style.display = shown ? "" : "none";
    }});
}});
</script>
</body>
</html>
"""

if __name__ == "__main__":
    build()
