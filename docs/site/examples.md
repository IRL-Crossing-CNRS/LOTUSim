# Code examples

```{admonition} Core simulator alone
:class: lotusim-context lotusim-context-core

These examples run against a world started with the core's own commands (`lotusim run`, `lotusim_core sim`, `mise run sim`), without the generic scenario.
```

Small programs that talk to a running simulation, one per language. Each
runs against a world started from the core, in its `nix develop` shell,
without the scenario workspace.

```{toctree}
:maxdepth: 1

Python scripts <_collected/core/examples/python-scripts/README>
C++ <_collected/core/examples/cpp_examples/README>
Jupyter notebooks <_collected/core/examples/jupyter-python/README>
```
