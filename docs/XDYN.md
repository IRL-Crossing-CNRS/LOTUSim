# xdyn, the hydrodynamics

xdyn computes how a vessel moves in the water: hydrostatics, waves, current,
damping, propellers. LOTUSim runs it as a separate server per vehicle class
and steps it from the physics plugin. The build LOTUSim uses is
[lxdyn](https://github.com/naval-group/lxdyn), Naval Group's continuation of
SIREHNA's xdyn.

## Where its documentation is

| What | Where |
|---|---|
| The model file format: every environment and force model, every key | xdyn's [user documentation](https://sirehna_naval_group.gitlab.io/sirehna/xdyn/) (in French), in particular the [environment models](https://gitlab.com/sirehna_naval_group/sirehna/xdyn/-/blob/master/doc/user_fr/modeles_environnementaux.md) and the [force models](https://gitlab.com/sirehna_naval_group/sirehna/xdyn/-/blob/master/doc/user_fr/modeles_efforts.md) |
| Step-by-step tutorials (falling ball, hydrostatics, waves, propulsion) | lxdyn's [README](https://github.com/naval-group/lxdyn#the-tutorials) |
| The same documentation as PDF files | [xdyn.pdf](xdyn.pdf) and [xdyn_tutorial.pdf](xdyn_tutorial.pdf), in this repository |
| Building xdyn, its tests | lxdyn's [README](https://github.com/naval-group/lxdyn) |

## How LOTUSim runs it

```
scenario_launch.sh ──starts──▶ xdyn-for-cs <model>.yaml --port P --dt 0.2   (one per vehicle class)
                                     ▲
                    websocket ws://127.0.0.1:P, one message per step
                                     ▼
physics_engine_interface (Gazebo plugin) ──▶ pose and velocity of the vessel in Gazebo
```

1. The scenario launcher starts one `xdyn-for-cs` server for each vehicle
   class the scenario uses, on the port listed in its `XDYN_CONFIGS` map
   (`Wamv` → `wamv.yaml`, port 12348). All vessels of a class share it.
2. A vessel declares `<interface_type>XDynWebSocket</interface_type>` and the
   server's `<uri>` in its SDF; the scenario's agent classes write that block
   for you.
3. Each Gazebo step, `xdyn_websocket.cpp` sends xdyn the vessel's last state
   (`t`, position, quaternion, body velocities, in xdyn's NED/FRD frames), the
   step `Dt` and the commands, and gets the next state back.
4. **Commands** are named `<force model name>(<command>)`, as the model file
   declares them: `wamv_prop_left(T)` for a thrust in newtons,
   `propeller(rpm)` for a propeller speed. LOTUSim speaks RPM; the plugin
   converts `(rpm)` commands to rad/s, the unit xdyn reads.
5. **Currents** come either from the model file (`ekman current`) or from the
   plugin (Gauss-Markov, Copernicus), never both. See
   [DEVELOPER.md](DEVELOPER.md), "Where each current model lives".
6. **A weather that changes during the run** reaches xdyn through two files it
   re-reads when they change: the rays file of the waves (`reload rays file:
   true`, with a fade per ray) and the wind changes of the Ekman current
   (`weather file`). With `layer thickness follows wind: true`, the Ekman top
   layer depth follows the wind (`D = 7.6 U10 / sqrt(sin|latitude|)`). The
   scenario workspace's launcher writes all of them, from the scenario's
   `weather` block.

## A model file

`assets/models/<model>/<model>.yaml`. The WAM-V's, shortened:

```yaml
rotations convention: [psi, theta', phi'']

environmental constants:
    g: {value: 9.81, unit: m/s^2}
    rho: {value: 1025, unit: kg/m^3}
    nu: {value: 1.18e-6, unit: m^2/s}

environment models:
  - model: waves            # a JONSWAP spectrum; the launcher can replace it
    ...                     # with the scenario's sea state
  - model: ekman current    # optional

bodies:                     # NED parent frame
  - name: WAMV
    mesh: wamv/meshes/wamv_pontoons.stl     # relative to XDYN_ASSETS_PATH
    position of body frame relative to mesh: ...
    initial position of body frame relative to NED: ...
    initial velocity of body frame relative to NED: ...
    dynamics:
        hydrodynamic forces calculation point in body frame: ...
        centre of inertia: ...
        rigid body inertia matrix at the center of gravity and projected in the body frame: ...
        added mass matrix at the center of gravity and projected in the body frame: ...
    external forces:
      - model: gravity
      - model: non-linear hydrostatic (exact)
      - model: non-linear Froude-Krylov
      - model: linear damping
      - model: quadratic damping
      - model: maneuvering        # one thruster: a force in the body frame
        name: wamv_prop_left      # → the command wamv_prop_left(T)
        commands: [T]
        X: T
        ...
```

`assets/models/` has one for every xdyn vehicle (`bluerov2_heavy/BlueROV2.yml`,
`lrauv/lrauv.yml`, `fremm/fremmConfig.yaml`, ...): start from the closest one.

## Writing a model that holds

| Check | Why |
|---|---|
| `external forces:` lists the forces, indented under the body | A misindented or empty list parses as no force at all: the vessel freezes or sinks, with no error |
| A `linear damping` or `quadratic damping` model is present | In xdyn only the damping models see the current. Without one, the vessel ignores the current |
| `hydrodynamic forces calculation point` equals `centre of inertia` | Any offset creates a torque; with little restoring moment the vessel slowly rolls over |
| Hydrostatics and Froude-Krylov together | Hydrostatics alone integrates the pressure from the mean sea level: the hull barely follows the waves. Froude-Krylov adds the wave pressure |
| `non-linear hydrostatic (exact)` for multi-hull or thin shapes | The `fast` variant can make such hulls spin in calm water |
| One closed, outward-facing mesh shell per hull, written as ASCII STL | Separate shells grazing the surface on their own give force spikes; xdyn rejects some binary STL files |
| Mesh facets small compared with the shortest wave | Waves are sampled at the vertices: long facets average them out |
| `omega max` of the wave spectrum suited to the hull | Wavelengths far shorter than the hull add load without realistic motion |
| Spawn slightly below `z = 0`, at the draught | Exactly on the surface, the Ekman current model returns no current |
| A small enough time step | Light hulls with no added mass need a smaller `--dt` (`xdyn_dt()` in `scenario_launch.sh`) |

The [WAM-V model](https://github.com/IRL-Crossing-CNRS/LOTUSim-generic-scenario/blob/main/doc/WAMV_MODEL.md)
is a complete example that follows all of these.

## Other pitfalls

- **Mesh paths** are relative to `XDYN_ASSETS_PATH`, which must end with a
  `/`: xdyn joins the two by plain concatenation. Without the slash, or with
  `XDYN_ASSETS_PATH` unset, xdyn refuses the vehicle at start.
- **Logs:** `XDYN_VERBOSE=1` on the launcher writes every message xdyn
  exchanges to `scenario_logs/<date>/xdyn_<class>.log`.

## Run another xdyn

The core's `nix develop` shell provides `xdyn-for-cs`. To use another build,
point `XDYN_PATH` at the directory that holds it before launching a scenario.
