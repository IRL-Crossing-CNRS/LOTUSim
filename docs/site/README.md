# Documentation site

The sources of https://irl-crossing-cnrs.github.io/LOTUSim/. Every repository
keeps its own Markdown; `collect.py` copies it here at build time and Sphinx
turns it into one site.

```sh
./build.sh serve     # http://127.0.0.1:8000, rebuilds on change
./build.sh html      # _build/html
```

The core is read from this checkout. The other repositories are read from a
local clone, at a ref, so the site shows what is on that ref and nothing that
only exists in one working tree:

```sh
export LOTUSIM_DOCS_SCENARIO_REPO=~/path/to/LOTUSim-generic-scenario
export LOTUSIM_DOCS_INSTALL_REPO=~/path/to/How_To_Install_LOTUSim
export LOTUSIM_DOCS_UNITY_REPO=~/path/to/LOTUSim-Energy
# optional, default origin/main
export LOTUSIM_DOCS_SCENARIO_REF=origin/main
```

A repository whose variable is unset is skipped.

`_extra/lotusim-links.html` is the page the printed QR codes point to. It is
copied to the root of the site unchanged, so
https://irl-crossing-cnrs.github.io/LOTUSim/lotusim-links.html keeps working.
Edit it in place; do not move or rename it.
