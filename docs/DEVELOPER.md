# Developer guide — LOTUSim core

Entry point for someone who is going to **change** this repository. If you
only want to run scenarios, you want the scenario workspace
(`LOTUSim-generic-scenario`) and its `README.md`, not this file.

This repository is the core: the Gazebo system plugins, the physics bridge to
xdyn, the ROS 2 message definitions, the vehicle models and the worlds.
It contains no mission logic and no scenario — those live in the scenario
workspace, in Python.

## 1. What is in here

```txt
systems/        Gazebo system plugins (C++) -- the simulator's own behaviour
interfaces/     ROS 2 message, service and action definitions
assets/         models/ (SDF + xdyn YAML per vehicle) and worlds/ (.world)
physics/        prebuilt xdyn binaries (xdyn, xdyn-for-cs, xdyn-for-me, libx-dyn.so)
launch/         the `lotusim` CLI, its bash completion, install and entrypoint scripts
docs/           this guide, coordinate_frames.md, Doxygen config, architecture images
examples/       standalone examples (jupyter-python)
```

### The plugins in `systems/`

| Plugin class | Package | Role |
|---|---|---|
| `EntityManager` | `entity_manager` | Spawns and deletes models on the `mas_cmd` / `mas_cmd_array` action, and republishes every entity's pose and twist on `/<world>/poses` |
| `AerialEntityManager` | `aerial_demo_entity_manager` | The same service for the aerial world, which is a second Gazebo process |
| `PhysicsInterfacePlugin` | `physics_engine_interface` | The backend switch. Consumes `/<world>/vessel_cmd_array` and drives each vessel either through xdyn or through `KinematicInterface` |
| `RenderPlugin` | `render_interface` | Streams poses to Unity over the TCP/UDP bridge |
| `WindRegionsPlugin` | `wind_regions` | Applies a per-link aerodynamic force from `/aerialWorld/wind` and `/aerialWorld/wind/regions`. Full replacement for stock `gz-sim-wind-effects-system`, same `<enable_wind>` tag and same force approximation |
| `WaypointFollowerPlugin` | `waypoint_follower` | Gazebo-side waypoint following, driven by the `SetWaypoints` service. Independent of the Python GNC pipeline |
| `LotusimSensorPlugin` | `sensors/lotusim_sensor_plugin` | Sensor host plugin; individual sensors (AIS, battery, IMU, radar, subsea pressure) are separate packages under `systems/sensors/` |
| `LightActuatorPlugin` | `actuators/light_actuator` | Example actuator plugin |

### Inside `physics_engine_interface`

This is the package that decides what physics a vehicle actually gets, and
the one most likely to surprise you.

| File | Role |
|---|---|
| `physics_interface_plugin.cpp` | The Gazebo system: reads vessel commands, dispatches per vessel to xdyn or Kinematic, writes state back into the ECM |
| `xdyn_websocket.cpp` | One websocket per xdyn-backed vessel. Sends state, receives forces. Also where an externally-computed current is injected, by a Galilean velocity shift: subtract before sending, add back on the reply |
| `gauss_markov_current.cpp` | First-order Gauss-Markov (OU) current process, injected through that shift |
| `copernicus_current.cpp` | Replay of a measured Copernicus depth profile, same injection point, mutually exclusive with Gauss-Markov |
| `kinematic_interface.cpp` | The no-physics backend: integrates a commanded `{u, w, vz}` and overwrites the model pose each step. Process-wide singleton, so it serves every Kinematic vehicle in any domain |
| `ocean_current_feed.cpp` | Subscribes `/<world>/ocean_current` and feeds `KinematicInterface::setCurrent` — a single uniform drift vector, unrelated to the xdyn currents above |
| `ros2_interface.cpp` | The plugin's ROS 2 node and its spin thread |

**Where each current model lives** is the single most common confusion:

- **Ekman** is xdyn's own environment model. No code here implements it. It
  is enabled by the vehicle YAML that xdyn is launched with
  (`environment models: - model: ekman current`), so the current a BlueROV
  feels is chosen by *which file* is loaded — the four
  `assets/models/bluerov2_heavy/BlueROV2_current_{ekman,gauss,copernicus,none}.yml`
  variants — driven by the scenario JSON's top-level `bluerov_current` key.
- **Gauss-Markov and Copernicus** are ours, computed here, injected at
  xdyn's interface. The YAML loaded in those cases declares no current at
  all, so nothing is double-counted.
- **The Kinematic current** is a third, much simpler thing: one uniform
  vector added to the commanded velocity, no depth structure, no force.

### `interfaces/`

`lotusim_msgs` holds the simulator's own types: `VesselPositionArray` /
`VesselPosition` (the pose feedback bus), `VesselCmdArray` / `VesselCmd` (the
actuator command bus, payload a JSON string in `cmd_string`),
`GuidanceSetpoint`, `WaypointFollowerStatus`, `Wind` and the wind-region
types, the turbine/LCOE telemetry types, `MASCmd` (action) and `SetWaypoints`
(service). `lotusim_sensor_msgs` holds the sensor outputs (AIS, GPS, radar,
pressure, collisions).

These definitions are the contract with the Python side. Changing a field
here means rebuilding both workspaces.

## 2. Building

```bash
source /opt/ros/jazzy/setup.bash
lotusim clean_build
source $HOME/lotusim_ws/install/setup.bash
```

`lotusim` (in `launch/`) is the CLI wrapper: `install`, `clean`, `build`,
`clean_build`, `doc`, `run [world]`, `ui`, with `--debug`, `--gui`,
`--ws-path` and `--assets-path`. Use it rather than a bare `colcon build` —
it sets the workspace and asset paths the plugins are looked up through.

The xdyn binaries in `physics/` are prebuilt and committed; they are not
built by this workspace. Rebuilding them is a separate exercise against the
xdyn repository, and a rebuild is what a change to a physics model such as
the Ekman current requires.

## 3. Conventions that will bite you

- **Frames.** Read [coordinate_frames.md](coordinate_frames.md) before
  touching anything that moves. The invariant: every vehicle publishes its
  pose on `/<world>/poses` in world ENU with a body x-forward frame
  (REP-103), whatever the backend. xdyn works in NED/FRD, Unity in a
  left-handed frame. The conversions are in `xdyn_websocket.cpp`, and a
  conversion applied to the wrong side (world basis vs. body basis) is a bug
  that looks like a plausible trajectory.
- **The aerial world is a second Gazebo process.** `aerialWorld.world` is
  always launched alongside the scenario's own world, and its name is
  hardcoded on both sides. Aerial entities are spawned through
  `AerialEntityManager`, not the main `EntityManager`.
- **xdyn stalls on an incomplete command set.** If a vehicle's YAML declares
  actuator command keys, every step needs a complete command for them. A
  vehicle with no declared actuator drifts happily with no command at all;
  one with a declared propeller does not.
- **`external forces:` is a sibling of the model, not a child of
  `dynamics:`.** Mis-indented in a vehicle YAML, xdyn parses it as empty and
  reports no error — every degree of freedom then stays frozen at its initial
  value regardless of current. This has already happened more than once.

## 4. Verifying a change

Test through the scenario workspace's `scenario_launch.sh` or through
`lotusim run`, never through a bare `gz sim`: a bare run bypasses the
orchestration, the spawn path and the physics bridge that the change most
likely affects.

A clean start proves little. Enable `"record_csv": true` in the scenario and
read the per-vehicle CSV afterwards over a run long enough for a divergence
to appear (60–90 s). Frame errors, force-coupling errors and unstable gains
all produce a simulation that starts normally and is wrong a minute later.

## 5. Related repositories

| Repository | Contains |
|---|---|
| `LOTUSim-generic-scenario` | Scenarios, agent classes, behaviour trees, the Python GNC pipeline |
| `LOTUSim-Xdyn` | The xdyn hydrodynamic solver whose binaries are vendored in `physics/` |
| `LOTUSim-Unity-modules` | The Unity renderer that consumes `/<world>/poses` |
