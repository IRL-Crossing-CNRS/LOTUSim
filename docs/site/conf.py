"""Sphinx configuration for the LOTUSim documentation site.

Build with ./build.sh (see README.md in this folder). The pages under
_collected/ are copied from the repositories by collect.py at build time.
"""

import os

project = "LOTUSim"
author = "IRL Crossing and Naval Group"
copyright = "LOTUSim contributors, EPL-2.0"

extensions = [
    "myst_parser",
    "sphinx_copybutton",
    "sphinx_design",
    "sphinxcontrib.mermaid",
]

source_suffix = {".md": "markdown"}
root_doc = "index"
exclude_patterns = ["_build", ".venv", "README.md", "_collected/.sources", "_collected/.core_interfaces", "_collected_assets"]

myst_enable_extensions = ["colon_fence", "deflist", "attrs_inline", "html_image"]
myst_heading_anchors = 3
# The repositories draw their diagrams in ```mermaid fences, as GitHub renders.
myst_fence_as_directive = ["mermaid"]
mermaid_version = "11.4.1"
# Diagrams follow the page's light or dark theme, and use a system font: the
# web font loads after Mermaid measures its labels, which clips them.
mermaid_init_js = """
import mermaid from "https://cdn.jsdelivr.net/npm/mermaid@11.4.1/dist/mermaid.esm.min.mjs";
const theme = document.body.dataset.theme;
const dark = theme === "dark" ||
  (theme !== "light" && window.matchMedia("(prefers-color-scheme: dark)").matches);
const palette = dark ? {
  primaryColor: "#0B2A4A", primaryTextColor: "#EFF5FC", primaryBorderColor: "#F2A623",
  lineColor: "#93A9C2", secondaryColor: "#123A63", tertiaryColor: "#082039",
  clusterBkg: "#061F38", clusterBorder: "#3B5B7C", edgeLabelBackground: "#04182F",
  noteBkgColor: "#123A63", noteTextColor: "#EFF5FC",
} : {
  primaryColor: "#E8F1FB", primaryTextColor: "#0B2A4A", primaryBorderColor: "#0E5A9C",
  lineColor: "#5B7A99", secondaryColor: "#FFF3DC", tertiaryColor: "#F4F7FB",
  clusterBkg: "#F4F7FB", clusterBorder: "#B7CCE2", edgeLabelBackground: "#FFFFFF",
  noteBkgColor: "#FFF3DC", noteTextColor: "#0B2A4A",
};
mermaid.initialize({
  startOnLoad: false,
  theme: "base",
  themeVariables: { ...palette, fontFamily: "system-ui, -apple-system, 'Segoe UI', sans-serif", fontSize: "15px" },
  flowchart: { curve: "basis", padding: 12, nodeSpacing: 40, rankSpacing: 50, useMaxWidth: true,
               wrappingWidth: 320, subGraphTitleMargin: { top: 6, bottom: 22 } },
  sequence: { useMaxWidth: true, mirrorActors: false },
});
window.mermaid = mermaid;
"""

# Collected pages are written for GitHub, not for this site: a few of their
# links point at anchors or files Sphinx cannot resolve. They still work as
# plain links; do not fail the build on them.
suppress_warnings = ["myst.xref_missing", "myst.header", "toc.not_included", "misc.highlighting_failure"]

html_theme = "furo"
html_title = "LOTUSim"
html_logo = "_static/logo.png"
html_favicon = "_static/logo.png"
html_static_path = ["_static"]
html_css_files = [
    "https://fonts.googleapis.com/css2?family=Space+Grotesk:wght@500;700&family=JetBrains+Mono:wght@400;500&display=swap",
    "lotusim.css",
]
html_templates_path = ["_templates"]
templates_path = ["_templates"]

# lotusim-links.html is the page the printed QR codes point to. It is copied
# to the root of the site untouched, so its URL never changes.
html_extra_path = ["_extra", "_collected_assets"]

html_sidebars = {
    "**": [
        "sidebar/brand.html",
        "sidebar/search.html",
        "sidebar/scroll-start.html",
        "sidebar/navigation.html",
        "links.html",
        "sidebar/scroll-end.html",
    ]
}

html_theme_options = {
    "sidebar_hide_name": False,
    "navigation_with_keys": True,
    "light_css_variables": {
        "color-brand-primary": "#0B2A4A",
        "color-brand-content": "#0E5A9C",
    },
    "dark_css_variables": {
        "color-brand-primary": "#F2A623",
        "color-brand-content": "#F2A623",
        "color-background-primary": "#04182F",
        "color-background-secondary": "#020B18",
        "color-foreground-primary": "#EFF5FC",
        "color-foreground-secondary": "#93A9C2",
    },
}

# A local build is a preview: say so on every page, so a screenshot of it is
# never mistaken for the published site. The deployment leaves this unset.
_preview = os.environ.get("LOTUSIM_DOCS_PREVIEW_BANNER")
if _preview:
    html_theme_options["announcement"] = _preview

html_copy_source = False
html_show_sourcelink = False
