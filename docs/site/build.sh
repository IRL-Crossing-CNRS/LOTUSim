#!/usr/bin/env bash
# Build or preview the LOTUSim documentation site.
#
#   ./build.sh html      build into _build/html
#   ./build.sh serve     build, serve on http://127.0.0.1:8000 and rebuild on change
#
# The repositories other than the core are read from local clones, at a ref:
# see collect.py for the LOTUSIM_DOCS_* variables. A local build carries a
# "preview" banner; the deployment sets LOTUSIM_DOCS_PREVIEW_BANNER="".
set -euo pipefail

cd "$(dirname "$0")"

# A sourced ROS environment puts its own packages on PYTHONPATH; keep them out.
unset PYTHONPATH

if [ ! -x .venv/bin/sphinx-build ]; then
  # Debian and Ubuntu ship python3 without ensurepip unless python3-venv is
  # installed; the system pip can still install into a pip-less venv.
  if python3 -m venv .venv >/dev/null 2>&1; then
    .venv/bin/pip install --quiet -r requirements.txt
  else
    rm -rf .venv
    python3 -m venv --without-pip .venv
    python3 -m pip --python .venv/bin/python install --quiet -r requirements.txt
  fi
fi

export LOTUSIM_DOCS_PREVIEW_BANNER="${LOTUSIM_DOCS_PREVIEW_BANNER-Local preview, not the published site}"

.venv/bin/python collect.py

case "${1:-html}" in
  html)
    .venv/bin/sphinx-build -b html -q . _build/html
    echo "Built: $(pwd)/_build/html/index.html"
    ;;
  serve)
    .venv/bin/sphinx-autobuild . _build/html \
      --host 127.0.0.1 --port "${PORT:-8000}" \
      --ignore "_collected/*" --ignore ".venv/*"
    ;;
  *)
    echo "usage: $0 [html|serve]" >&2
    exit 1
    ;;
esac
