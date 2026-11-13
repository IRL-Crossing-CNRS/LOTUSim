# Build and rebuild

```{admonition} Core simulator and generic scenario
:class: lotusim-context lotusim-context-both

Building the core serves every way of running LOTUSim; building the scenario workspace serves generic scenarios.
```

After a `git pull`, a branch change or an edit, some parts of LOTUSim must be
rebuilt before the change takes effect, and some are read straight from the
sources. This page says which, and with what command.

Two of the four parts of LOTUSim are built here (see
[What LOTUSim is made of](overview.md)): the **core simulator** (C++, the
Gazebo plugins) and the **scenario workspace** or *generic scenario* (Python,
the agents and missions). Both are ROS 2 workspaces built with `colcon`. The
Unity player is built in Unity; xdyn comes ready-built.

## The commands

The installer adds two commands to `~/.bashrc`. Each opens the right
environment by itself: nothing needs to be sourced first.

| | Command |
|---|---|
| Build the core | `lotusim_core build` |
| Rebuild the core from scratch | `lotusim_core rebuild` |
| Open the scenario workspace's shell | `lotusim_shell` |

- `lotusim_core build` runs `mise run build` in the core's `nix develop`
  shell. `lotusim_core build --debug` builds with debug logs and a symlinked
  install. `rebuild` deletes `build/`, `install/` and `log/` first.
- `lotusim_shell` opens the scenario workspace's shell, built against
  **your** core clone, with both workspaces sourced. Build the scenario
  workspace from there:

  ```bash
  lotusim_shell
  colcon build --symlink-install
  ```

  To rebuild it from scratch: `rm -rf build install log` before
  `colcon build`.

Always build the core first, then the scenario workspace: the scenario
workspace is built against the core's install.

## Sourcing

A built ROS 2 workspace is only visible to a shell once its
`install/setup.bash` has been **sourced**: that is what tells `ros2`,
Python and Gazebo where its packages, messages and plugins are.

- **With the commands above you never source by hand.** `lotusim_core`
  runs inside the core's environment, and `lotusim_shell` sources both
  workspaces, the core first, when it opens.
- **After a rebuild, in a shell that was already open,** source again, or
  the shell keeps the packages it saw at opening (a new message or a new
  task is then "not found"):

  ```bash
  source $LOTUSIM_PATH/install/setup.bash   # after rebuilding the core
  source install/setup.bash                       # after rebuilding the scenario workspace
  ```

  Opening a new `lotusim_shell` does the same.
- **For any other terminal** (a `ros2 topic echo`, a script talking to a
  running scenario), open `lotusim_shell` in it: a plain terminal has
  neither ROS 2 nor the workspaces.

## What to rebuild after a change

| You changed, or pulled a change to | Rebuild |
|---|---|
| A Gazebo plugin, a sensor, anything in the core's `systems/` | the core |
| A ROS 2 message in the core's `interfaces/` | the core, then the scenario workspace |
| A model (`assets/models/`), a world (`assets/worlds/`), an xdyn YAML | nothing: they are read from the core's sources |
| Python in the scenario workspace (`lotusim_sdk`, tasks, agents) | the scenario workspace. Its Python runs from `build/`, not from `src/`: an unbuilt change does not run, even though the scenario JSON next to it is up to date |
| A scenario JSON (`src/simulation_run/config/`) | nothing: the launcher reads it from the sources |
| The Unity project | a new player, see below |

When in doubt after a pull of both repositories: `lotusim_core build`, then
`lotusim_shell` and `colcon build --symlink-install`.

## Unity players

The launcher never opens the Unity project: it starts a built player from the
scenario workspace,

| Player | Folder | Used when |
|---|---|---|
| Linux | `lotusim_unity_executables/lotusimenergy_linux/` (`LOTUSimEnergy.x86_64`) | on Linux |
| Windows | `lotusim_unity_executables/lotusimenergy_windows/` (`LOTUSimEnergy.exe`) | under WSL |

These folders are managed by the launcher: it downloads the player pinned in
`lotusim_unity_executables/players.json` and **replaces any player whose
version does not match**, a build you copied there included. So, for a player
you built yourself:

- **To try it,** keep it outside the workspace and point the launcher at it:

  ```bash
  UNITY_EXE_DIR=~/builds/lotusimenergy_linux \
    ./src/simulation_run/executable/scenario_launch.sh --config basic_examples/waypoint_solo.json
  ```

- **To give it to everyone,** ship it as a release asset and update
  `players.json`: see [Build and ship the player](_collected/unity/doc/BUILD_PLAYER.md).
