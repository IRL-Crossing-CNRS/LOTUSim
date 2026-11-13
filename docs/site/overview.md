# What LOTUSim is made of

LOTUSim is not one program but four parts, each in its own repository. A
scenario uses all of them; you change only the one your work is about.

```{figure} _static/figures/lotusim_architecture_iros2026.png
:class: lotusim-figure
:alt: Architecture of LOTUSim: the Core (Gazebo world, rendering plugin, sensors, physics plugin) connected to the Unity rendering, the LOTUSim-Xdyn physics server, the UI, the multi-user support and the external agents
:width: 80%

System architecture of LOTUSim, from the IROS 2026 paper
([Buche et al., 2026](https://hal.science/hal-05677110)).
```

How the boxes of the figure map to the repositories:

- **Core**, the Gazebo world and its plugins, is the core simulator.
- **External Entities**, the agents, are what the generic scenario workspace
  runs: each agent spawns its vehicle and commands it over ROS 2.
- **Physics Server** is xdyn, one server per vehicle class.
- **Rendering** and **MultiUser Support** are the Unity project and its Photon
  multi-user layer.
- **UI** is Naval Group's web interface, used for UI scenarios only.

| Part | What it is | What you do in it | Repository | Language |
|---|---|---|---|---|
| **Core simulator** | The Gazebo world and LOTUSim's plugins: they spawn vehicles, step their physics through xdyn, run their sensors, send their poses to the renderer. Also every vehicle model and world (`assets/`). | Change how a vehicle is simulated: a plugin, a sensor, a model, a world, a ROS 2 message | `LOTUSim` (open source: `LOTUSim`) | C++ |
| **Scenario workspace**, also called *generic scenario* | What happens in the world: the vehicles a scenario spawns, their missions (behaviour trees), their navigation, guidance and control. Holds the scenario files and `scenario_launch.sh`, which starts everything. | Write a scenario, a mission task, a controller, an agent class | `LOTUSim-generic-scenario` (open source: `LOTUSim-generic-scenario`) | Python, JSON |
| **Unity project** | The 3D rendering of the scene. Scenarios run a built *player* of it, downloaded by the launcher; the project itself is only opened to change the visuals. | Change what the scene looks like, ship a new player | `LOTUSim-Energy` (open source: `LOTUSim-Energy`) | C# |
| **xdyn** | The hydrodynamics engine: waves, hydrostatics, damping, propellers. Provided by the core's environment; LOTUSim only writes its model files (`assets/models/*/*.yaml`). | Usually nothing; a vehicle's xdyn model lives in the core | [lxdyn](https://github.com/naval-group/lxdyn) | C++ |

The core runs on its own (a world, with nothing moving until something sends
it commands); the scenario workspace needs the core. Both are installed and
built together by the installer, the core first.

## Three ways to run it

| | Started with | Scenario | What runs |
|---|---|---|---|
| **Generic scenario** | `scenario_launch.sh --config <file>.json`, in the scenario workspace | JSON | Everything: worlds, xdyn, agents and missions, Unity |
| **Core simulator alone** | `lotusim_core sim <world>` (or `lotusim run`, `mise run sim`) | none | A world and its plugins; vehicles appear when a script or an example sends them a command |
| **UI scenarios** (Naval Group) | the LOTUSim web UI (`lotusim-ui`) | YAML | A world and the vehicles of a scenario edited in the UI. The IRL Crossing installs do not use it |

Each page says, under its title, which of these it is about. Pages of the
core's plugins, models and sensors apply to all three.

## Where to go next

| You want to | Read |
|---|---|
| Run what exists | [Run a scenario](_collected/scenario/README.md), then [Write a scenario](_collected/scenario/doc/scenario/README.md) |
| Rebuild after a change | [Build and rebuild](rebuild.md) |
| Change the simulation itself | [Architecture of the core](_collected/core/docs/DEVELOPER.md) |
| Change what vehicles do | [Architecture of the scenario workspace](_collected/scenario/doc/ARCHITECTURE.md) |
| Change the 3D view | [Unity project](_collected/unity/README.md) |
