#!/usr/bin/env python3
"""Gather the Markdown documentation of the LOTUSim repositories into one tree.

Every repository keeps its own documentation; this script copies it into
``_collected/<source>/`` so that one Sphinx build can publish all of it. Links
between collected pages stay relative, links to anything else (source files,
configuration folders) are turned into links to the repository on GitHub.

The core is read from this checkout. The other repositories are read from a
local clone at a given ref (``git archive``), never from their working tree,
so the site shows what is published on that ref and nothing that only exists
on one machine. Each one is configured through the environment:

    LOTUSIM_DOCS_<NAME>_REPO   path to a clone        (source skipped if unset)
    LOTUSIM_DOCS_<NAME>_REF    ref to read            (default: origin/main)
    LOTUSIM_DOCS_<NAME>_URL    GitHub URL of the repo (default: see SOURCES)

with <NAME> one of CORE, SCENARIO, INSTALL, INSTALL_EXTRA, UNITY. The core is
read from this checkout unless LOTUSIM_DOCS_CORE_REPO names another clone.

INSTALL_EXTRA is an optional additional installation guide, listed first on the
"How to install" page when its variables are set and absent otherwise.
LOTUSIM_DOCS_INSTALL_EXTRA_LABEL and LOTUSIM_DOCS_INSTALL_EXTRA_TEXT give its
card title and text.
"""

import os
import re
import shutil
import subprocess
import sys
import tarfile
from io import BytesIO
from pathlib import Path, PurePosixPath

HERE = Path(__file__).resolve().parent
CORE_ROOT = HERE.parent.parent
OUT = HERE / "_collected"

IMAGE_SUFFIXES = {".png", ".jpg", ".jpeg", ".gif", ".svg", ".webp"}

# name -> (default GitHub URL, the paths to take from the repository, where
# the collected pages go under _collected/)
SOURCES = {
    "core": (
        "https://github.com/IRL-Crossing-CNRS/LOTUSim",
        ["README.md", "CONTRIBUTING.md", "docs", "examples", "systems"],
        "core",
    ),
    "scenario": (
        "https://github.com/IRL-Crossing-CNRS/LOTUSim-generic-scenario",
        ["README.md", "CONTRIBUTING.md", "doc", "deployment", "src/simulation_run/config"],
        "scenario",
    ),
    "install": (
        "https://github.com/IRL-Crossing-CNRS/How_To_Install_LOTUSim",
        ["README.md"],
        "install/open-source",
    ),
    "install_extra": (
        "",
        ["README.md"],
        "install/extra",
    ),
    "unity": (
        "https://github.com/IRL-Crossing-CNRS/LOTUSim-Energy",
        ["README.md", "CONTRIBUTING.md", "doc", "Assets/Turbine_wake.md", "Assets/Submarine_Cables.md"],
        "unity",
    ),
}

UPSTREAM_INSTALL_URL = "https://github.com/naval-group/LOTUSim#quickstart"

# Pages written for this site, not collected from a repository.
SKIP_DIRS = {"site", "doxygen"}

LINK_RE = re.compile(r"(!?\[[^\]]*\])\(([^)\s]+)(\s+\"[^\"]*\")?\)")


def env(name, key, default=None):
    return os.environ.get(f"LOTUSIM_DOCS_{name.upper()}_{key}", default)


def wanted(rel: PurePosixPath) -> bool:
    if any(part in SKIP_DIRS for part in rel.parts):
        return False
    return rel.suffix.lower() == ".md" or rel.suffix.lower() in IMAGE_SUFFIXES


def extract_core(paths, dest: Path):
    for p in paths:
        src = CORE_ROOT / p
        files = [src] if src.is_file() else (src.rglob("*") if src.is_dir() else [])
        for f in files:
            rel = PurePosixPath(f.relative_to(CORE_ROOT).as_posix())
            if f.is_file() and wanted(rel):
                target = dest / rel
                target.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(f, target)


def extract_git(repo: str, ref: str, paths, dest: Path):
    listed = subprocess.run(
        ["git", "-C", repo, "ls-tree", "-r", "--name-only", ref, "--", *paths],
        check=True, capture_output=True, text=True,
    ).stdout.split()
    keep = [p for p in listed if wanted(PurePosixPath(p))]
    if not keep:
        return
    archive = subprocess.run(
        ["git", "-C", repo, "archive", "--format=tar", ref, "--", *keep],
        check=True, capture_output=True,
    ).stdout
    with tarfile.open(fileobj=BytesIO(archive)) as tar:
        tar.extractall(dest, filter="data")


def extract_python_api(repo: str, ref: str):
    """The scenario workspace's SDK sources, for the Python API reference."""
    archive = subprocess.run(
        ["git", "-C", repo, "archive", "--format=tar", ref, "--", "src/lotusim_sdk/lotusim_sdk"],
        check=True, capture_output=True,
    ).stdout
    dest = OUT / ".api" / "python"
    with tarfile.open(fileobj=BytesIO(archive)) as tar:
        members = [m for m in tar.getmembers() if m.name.endswith(".py")]
        for m in members:
            m.name = m.name.removeprefix("src/lotusim_sdk/")
        tar.extractall(dest, members=members, filter="data")


IMG_RE = re.compile(r"""!\[[^\]]*\]\(([^)\s]+)|<img[^>]+src=["']([^"']+)["']""")


def fetch_referenced_images(repo, ref, dest: Path):
    """Collect the images a page shows from outside the collected folders,
    e.g. a README's <img src="Media/...">."""
    wanted_files = set()
    for md in dest.rglob("*.md"):
        here = PurePosixPath(md.relative_to(dest).as_posix()).parent
        for m in IMG_RE.finditer(md.read_text(encoding="utf-8")):
            target = (m.group(1) or m.group(2)).split("#")[0]
            if re.match(r"^[a-z]+:", target) or target.startswith("/"):
                continue
            rel = PurePosixPath(os.path.normpath((here / target).as_posix()))
            if not str(rel).startswith("..") and not (dest / rel).exists():
                wanted_files.add(rel.as_posix())
    if not wanted_files:
        return
    if repo is None:
        for f in wanted_files:
            src = CORE_ROOT / f
            if src.is_file():
                (dest / f).parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(src, dest / f)
        return
    present = subprocess.run(
        ["git", "-C", repo, "ls-tree", "-r", "--name-only", ref, "--", *sorted(wanted_files)],
        check=True, capture_output=True, text=True,
    ).stdout.split()
    if present:
        archive = subprocess.run(
            ["git", "-C", repo, "archive", "--format=tar", ref, "--", *present],
            check=True, capture_output=True,
        ).stdout
        with tarfile.open(fileobj=BytesIO(archive)) as tar:
            tar.extractall(dest, filter="data")


def branch_name(ref: str) -> str:
    return ref.split("/", 1)[1] if ref.startswith("origin/") else ref


def rewrite_links(dest: Path, url: str, ref: str):
    """Keep links between collected files, send every other one to GitHub."""
    collected = {PurePosixPath(p.relative_to(dest).as_posix()) for p in dest.rglob("*") if p.is_file()}
    blob = f"{url}/blob/{branch_name(ref)}"
    tree = f"{url}/tree/{branch_name(ref)}"

    for md in dest.rglob("*.md"):
        here = PurePosixPath(md.relative_to(dest).as_posix()).parent

        def fix(m):
            label, target, title = m.group(1), m.group(2), m.group(3) or ""
            if re.match(r"^[a-z]+:", target) or target.startswith("#"):
                return m.group(0)
            path, _, anchor = target.partition("#")
            anchor = f"#{anchor}" if anchor else ""
            resolved = PurePosixPath(os.path.normpath((here / path).as_posix()))
            if str(resolved).startswith(".."):
                return m.group(0)
            if resolved in collected:
                return m.group(0)
            readme = resolved / "README.md"
            if readme in collected:
                rel = os.path.relpath(readme.as_posix(), here.as_posix() or ".")
                return f"{label}({rel}{anchor}{title})"
            base = tree if path.endswith("/") else blob
            return f"{label}({base}/{resolved.as_posix()}{anchor}{title})"

        text = md.read_text(encoding="utf-8")
        md.write_text(LINK_RE.sub(fix, text), encoding="utf-8")


def write_message_reference(core_root: Path):
    """One page per interface package, listing every message definition: the
    "ROS 2 custom messages" section of the site."""
    out = OUT / "reference"
    out.mkdir(parents=True, exist_ok=True)
    pages = []
    for pkg in sorted((core_root / "interfaces").iterdir()):
        if not pkg.is_dir():
            continue
        lines = [f"# {pkg.name}", "",
                 f"The ROS 2 messages, services and actions of the `{pkg.name}` package, "
                 "as their definition files declare them.", ""]
        for kind in ("msg", "srv", "action"):
            files = sorted((pkg / kind).glob(f"*.{kind}")) if (pkg / kind).is_dir() else []
            if not files:
                continue
            lines += [f"## {kind}", ""]
            for f in files:
                lines += [f"### {f.stem}", "", "```text", f.read_text(encoding="utf-8").rstrip(), "```", ""]
        (out / f"{pkg.name}.md").write_text("\n".join(lines), encoding="utf-8")
        pages.append(pkg.name)
    return pages


# Sections the repositories repeat in their README for GitHub readers: the
# videos and citations are on the home page, the list of documentation pages
# is the site's own menu, and a hand-written table of contents is replaced by
# the one add_page_contents() builds.
SHARED_SECTIONS = re.compile(r"^(videos?|relevant publications|publications|citation|cite|citing|documentation|table of contents)\b", re.I)


def drop_shared_sections(dest: Path):
    for md in dest.rglob("README.md"):
        lines = md.read_text(encoding="utf-8").split("\n")
        out, skipping, fence = [], False, False
        for line in lines:
            if line.lstrip().startswith("```"):
                fence = not fence
            if not fence and line.startswith("## "):
                skipping = bool(SHARED_SECTIONS.match(line[3:].strip()))
            if not skipping:
                out.append(line)
        # A dropped last section can leave its separator as the page's end.
        while out and out[-1].strip() in ("", "---", "***"):
            out.pop()
        md.write_text("\n".join(out) + "\n", encoding="utf-8")


# A page with many sections (four, or six headings counting subsections)
# opens with its own table of contents, numbered with its sections, so that a
# reader sees the whole page and its order before the first section.
# Pages whose headings already carry numbers keep them and get the table only:
# numbering them again would print "3.1 1. Install" and break the "§5.1"
# references the text makes to its own numbers.
MANY_SECTIONS = 4
MANY_HEADINGS = 6
NUMBERED_HEADING = re.compile(r"^#{2,3}\s+\d+(\.\d+)*[.)]?\s")


def add_page_contents(dest: Path):
    for md in dest.rglob("*.md"):
        lines = md.read_text(encoding="utf-8").split("\n")
        headings, fence, first = [], False, None
        for i, line in enumerate(lines):
            if line.lstrip().startswith("```"):
                fence = not fence
                continue
            if fence:
                continue
            if line.startswith("## ") or line.startswith("### "):
                headings.append(line)
                if first is None and line.startswith("## "):
                    first = i
        sections = sum(h.startswith("## ") for h in headings)
        if first is None or (sections < MANY_SECTIONS and len(headings) < MANY_HEADINGS):
            continue
        # After the page's introduction, before its first section and the
        # rule that usually separates them.
        at = first
        while at > 0 and lines[at - 1].strip() in ("", "---", "***"):
            at -= 1
        # Furo flags a contents directive unless told it is wanted.
        block = ["", "```{contents} On this page", ":local:", ":depth: 2", ":backlinks: none",
                 ":class: this-will-duplicate-information-and-it-is-still-useful-here", "```", ""]
        if not any(NUMBERED_HEADING.match(h) for h in headings):
            # Numbered by the stylesheet (lotusim.css): the heading text, and so
            # every anchor and link to it, stays as the repository wrote it.
            block = ["", '<div class="lotusim-numbered"></div>'] + block
        md.write_text("\n".join(lines[:at] + block + lines[at:]), encoding="utf-8")


def nest_subpages(dest: Path):
    """A README is the parent page of the READMEs in its sub-folders (the
    sensors page of the radar page): a hidden toctree puts them under it in
    the menu."""
    # The repository's own README is not a parent: its sub-pages have their
    # place in the site's menu.
    readmes = sorted(md for md in dest.rglob("README.md") if md.parent != dest)
    folders = {md.parent for md in readmes}

    def parent_page(md: Path):
        return next((d for d in md.parent.parents if d in folders), None)

    for md in readmes:
        children = [c for c in readmes if parent_page(c) == md.parent]
        if children:
            toc = [c.relative_to(md.parent).with_suffix("").as_posix() for c in children]
            with md.open("a", encoding="utf-8") as f:
                f.write("\n".join(["", "```{toctree}", ":hidden:", "", *toc, "```", ""]))


# Which way of running LOTUSim a page is about, shown in a banner under its
# title (see "What LOTUSim is made of"). The first matching prefix wins; a
# page that matches none gets no banner.
CONTEXTS = {
    "scenario": ("Generic scenario",
                 "Scenarios of the generic scenario workspace, described in a JSON file and run "
                 "with `scenario_launch.sh`."),
    "core": ("Core simulator alone",
             "The core's own commands (`lotusim run`, `lotusim_core sim`, `mise run sim`): a world "
             "without the generic scenario."),
    "both": ("Core simulator, in every way of running it",
             "Part of the core: it works the same in a generic scenario, under the core's own "
             "commands and in UI scenarios."),
}
PAGE_CONTEXT = [
    ("scenario/", "scenario"),
    ("core/README.md", "core"),
    ("core/docs/DEVELOPER.md", "core"),
    ("core/examples/", "core"),
    ("core/docs/", "both"),
    ("core/systems/", "both"),
]


def add_context_banners():
    for md in sorted(OUT.rglob("*.md")):
        rel = md.relative_to(OUT).as_posix()
        context = next((c for prefix, c in PAGE_CONTEXT if rel.startswith(prefix)), None)
        if context is None:
            continue
        title, text = CONTEXTS[context]
        lines = md.read_text(encoding="utf-8").split("\n")
        at = next((i + 1 for i, line in enumerate(lines) if line.startswith("# ")), 0)
        banner = ["", f"```{{admonition}} {title}", f":class: lotusim-context lotusim-context-{context}", "",
                  text, "```", ""]
        md.write_text("\n".join(lines[:at] + banner + lines[at:]), encoding="utf-8")


# Folders whose pages form a tree of their own (the scenario reference). The
# README is the root; `<name>_<part>.md` sits under `<name>.md` when that page
# exists (tasks_gnc under tasks), every other page under the README. Children
# keep the order in which their parent links to them.
PAGE_TREES = ["scenario/doc/scenario"]
MD_LINK = re.compile(r"\]\(([A-Za-z0-9_.-]+\.md)(?:#[^)]*)?\)")


def nest_page_trees():
    for folder in PAGE_TREES:
        root = OUT / folder
        if not (root / "README.md").exists():
            continue
        pages = {md.name for md in root.glob("*.md")}
        parents = {}
        for name in pages - {"README.md"}:
            stem = name[:-3].split("_")[0] + ".md"
            parents[name] = stem if stem != name and stem in pages else "README.md"
        for parent in sorted(set(parents.values())):
            text = (root / parent).read_text(encoding="utf-8")
            mine = [n for n in parents if parents[n] == parent]
            linked = [n for n in dict.fromkeys(MD_LINK.findall(text)) if n in mine]
            children = linked + sorted(set(mine) - set(linked))
            with (root / parent).open("a", encoding="utf-8") as f:
                f.write("\n".join(["", "```{toctree}", ":hidden:", "", *[c[:-3] for c in children], "```", ""]))


def first_heading(md: Path) -> str:
    for line in md.read_text(encoding="utf-8").splitlines():
        if line.startswith("# "):
            return line[2:].strip()
    return md.parent.name


def write_install_page(found):
    """The "How to install" page: one card per installation path available."""
    cards = []
    toc = []
    if "install_extra" in found:
        doc = "extra/README"
        label = env("install_extra", "LABEL") or first_heading(OUT / "install" / "extra" / "README.md")
        text = env("install_extra", "TEXT") or ""
        cards.append((label, doc, "doc", "lotusim-card-extra", text))
        toc.append(f"{label} <{doc}>")
    if "install" in found:
        doc = "open-source/README"
        label = "IRL Crossing open source"
        cards.append((label, doc, "doc", "lotusim-card-public",
                      "The public LOTUSim with the IRL Crossing scenario workspace and Unity player: "
                      "one script installs the core, the scenarios and their dependencies."))
        toc.append(f"{label} <{doc}>")
    cards.append(("Naval Group upstream", UPSTREAM_INSTALL_URL, "url", "lotusim-card-upstream",
                  "The core simulator alone, as Naval Group publishes it: their Nix quickstart, "
                  "then the Getting Started guide of their wiki."))

    lines = [
        "# How to install",
        "",
        "Pick the installation that matches what you want to do. Each one is a",
        "complete guide; you only need one.",
        "",
        "::::{grid} 1 1 1 1",
        ":gutter: 3",
        "",
    ]
    for title, link, link_type, css, text in cards:
        lines += [
            f":::{{grid-item-card}} {title}",
            f":link: {link}",
            f":link-type: {link_type}",
            f":class-card: lotusim-card {css}",
            "",
            text,
            ":::",
            "",
        ]
    lines += ["::::", "", "```{toctree}", ":hidden:", "", *toc, "```", ""]
    (OUT / "install").mkdir(parents=True, exist_ok=True)
    (OUT / "install" / "index.md").write_text("\n".join(lines), encoding="utf-8")


def main():
    if OUT.exists():
        shutil.rmtree(OUT)
    OUT.mkdir()
    found = []
    for name, (default_url, paths, subdir) in SOURCES.items():
        dest = OUT / subdir
        url = env(name, "URL", default_url)
        if name == "core" and not env(name, "REPO"):
            ref = env(name, "REF", "main")
            extract_core(paths, dest)
        else:
            repo = env(name, "REPO")
            ref = env(name, "REF", "origin/main")
            if name == "core":
                core_root = OUT / ".core_interfaces"
                core_root.mkdir()
                archive = subprocess.run(
                    ["git", "-C", repo, "archive", "--format=tar", ref, "--", "interfaces"],
                    check=True, capture_output=True,
                ).stdout
                with tarfile.open(fileobj=BytesIO(archive)) as tar:
                    tar.extractall(core_root, filter="data")
            if not repo:
                print(f"collect: {name}: LOTUSIM_DOCS_{name.upper()}_REPO unset, skipped", file=sys.stderr)
                continue
            extract_git(repo, ref, paths, dest)
        if name == "scenario":
            extract_python_api(repo, ref)
        if dest.exists():
            fetch_referenced_images(None if (name == "core" and not env(name, "REPO")) else repo, ref, dest)
            drop_shared_sections(dest)
            add_page_contents(dest)
            rewrite_links(dest, url, ref)
            nest_subpages(dest)
            found.append(name)
            print(f"collect: {name}: {sum(1 for _ in dest.rglob('*.md'))} pages from {ref}")
    # Images that pages show through raw HTML (<img> inside a <p>) are not
    # copied by Sphinx; publish every collected image at its own path too.
    assets = HERE / "_collected_assets"
    if assets.exists():
        shutil.rmtree(assets)
    for img in OUT.rglob("*"):
        if img.is_file() and img.suffix.lower() in IMAGE_SUFFIXES and ".core_interfaces" not in img.parts:
            target = assets / "_collected" / img.relative_to(OUT)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(img, target)

    add_context_banners()
    nest_page_trees()
    interfaces_root = OUT / ".core_interfaces" if (OUT / ".core_interfaces").exists() else CORE_ROOT
    write_message_reference(interfaces_root)
    write_install_page(found)
    (OUT / ".sources").write_text("\n".join(found), encoding="utf-8")


if __name__ == "__main__":
    main()
