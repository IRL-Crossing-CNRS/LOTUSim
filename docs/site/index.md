# LOTUSim

```{raw} html
<div class="lotusim-hero">
  <p class="lotusim-hero-kicker">Multi-domain simulator for marine robotics</p>
  <p class="lotusim-hero-text">Surface vessels, underwater vehicles and drones in one Gazebo world,
  with xdyn computing the hydrodynamics and Unity rendering the scene.</p>
  <p class="lotusim-hero-actions">
    <a class="lotusim-button lotusim-button-primary" href="_collected/install/index.html">Install</a>
    <a class="lotusim-button" href="_collected/scenario/README.html">Run a scenario</a>
    <a class="lotusim-button" href="lotusim-links.html">Papers, videos &amp; code</a>
  </p>
</div>
```

## Start here

Three steps take you from nothing to your own mission. First time? Read
[what LOTUSim is made of](overview.md): the core simulator, the scenario
workspace, Unity and xdyn, and what each one does.

::::{grid} 1 1 3 3
:gutter: 3

:::{grid-item-card} 1 · Install
:link: _collected/install/index
:link-type: doc
:class-card: lotusim-card lotusim-step lotusim-step-1

Choose an installation and follow its guide to the end.
:::

:::{grid-item-card} 2 · Run a generic scenario
:link: _collected/scenario/README
:link-type: doc
:class-card: lotusim-card lotusim-step lotusim-step-2

Launch one of the example scenarios and watch it in Unity.
:::

:::{grid-item-card} 3 · Write your own
:link: _collected/scenario/doc/scenario/README
:link-type: doc
:class-card: lotusim-card lotusim-step lotusim-step-3

Describe vehicles, missions and sensors in a scenario JSON file.
:::
::::

## Use it

How to run the simulator and describe what it simulates. No code.

::::{grid} 1 2 4 4
:gutter: 3

:::{grid-item-card} Generic scenario: launch options
:link: _collected/scenario/doc/LAUNCH_OPTIONS
:link-type: doc
:class-card: lotusim-card lotusim-topic

Debug, full-screen Unity, another xdyn or clone, logs.
:::

:::{grid-item-card} Sensors
:link: _collected/core/systems/sensors/README
:link-type: doc
:class-card: lotusim-card lotusim-topic

What each sensor measures and publishes, and how to put one on a vehicle.
:::

:::{grid-item-card} Generic scenario examples
:link: _collected/scenario/src/simulation_run/config/README
:link-type: doc
:class-card: lotusim-card lotusim-topic

Ready-to-run scenarios: currents, waves, wind farms, fleets.
:::

:::{grid-item-card} Faster than real time
:link: _collected/scenario/doc/ACCELERATED_SIMULATION
:link-type: doc
:class-card: lotusim-card lotusim-topic

Run a world faster than the clock.
:::
::::

## Develop it

How it is built and how to extend it, with the code.

::::{grid} 1 2 4 4
:gutter: 3

:::{grid-item-card} How LOTUSim is built
:link: dev/architecture
:link-type: doc
:class-card: lotusim-card lotusim-topic

How the plugins, the xdyn bridge and the scenario runner fit together.
:::

:::{grid-item-card} Add a vehicle
:link: _collected/scenario/doc/ADD_A_VEHICLE
:link-type: doc
:class-card: lotusim-card lotusim-topic

Gazebo model, xdyn model, agent class and Unity prefab, step by step.
:::

:::{grid-item-card} Navigation & control
:link: _collected/scenario/doc/GNC_MODULAR_ARCHITECTURE
:link-type: doc
:class-card: lotusim-card lotusim-topic

The guidance, navigation and control pipeline, and how to plug in your own block.
:::

:::{grid-item-card} Unity project
:link: _collected/unity/README
:link-type: doc
:class-card: lotusim-card lotusim-topic

The 3D scene, its scripts, and how to ship a new player.
:::
::::

## Look it up

Generated from the code: every message, class and function.

::::{grid} 1 2 3 3
:gutter: 3

:::{grid-item-card} ROS 2 messages
:link: _collected/reference/lotusim_msgs
:link-type: doc
:class-card: lotusim-card lotusim-topic

Every message, service and action the simulator uses.
:::

:::{grid-item-card} Generic scenario: Python API
:link: api/python
:link-type: doc
:class-card: lotusim-card lotusim-topic

The scenario SDK: agents, tasks, guidance and control, from its docstrings.
:::

:::{grid-item-card} Core simulator: C++ API
:link: api/cpp
:link-type: doc
:class-card: lotusim-card lotusim-topic

The core's Gazebo plugins, from their header comments.
:::
::::

## Papers, videos & code

::::{grid} 1 1 2 2
:gutter: 3

:::{grid-item}
**LOTUSim** · IROS 2026\
[arXiv:2607.03072](https://arxiv.org/abs/2607.03072) ·
[HAL](https://hal.science/hal-05677110) ·
[Paper page](https://irl-crossing-cnrs.github.io/resources/lotusim-iros2026)

```{raw} html
<div class="lotusim-video"><iframe src="https://www.youtube-nocookie.com/embed/iXDz8ZqSpq4" title="LOTUSim video" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" allowfullscreen loading="lazy"></iframe></div>
```
:::

:::{grid-item}
**LOTUSim-Energy** · IROS 2026 workshop\
[arXiv:2609.17124](https://arxiv.org/abs/2609.17124) ·
[HAL](https://hal.science/hal-05741707) ·
[Paper page](https://irl-crossing-cnrs.github.io/resources/lotusim-energy-iros2026)

```{raw} html
<div class="lotusim-video"><iframe src="https://www.youtube-nocookie.com/embed/NRd-LjRrrac" title="LOTUSim-Energy video" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" allowfullscreen loading="lazy"></iframe></div>
```
:::
::::

### Cite LOTUSim

If you use [LOTUSim](https://github.com/naval-group/LOTUSim) in your research,
or any of the repositories directly linked to it — the scenario workspace, the
Unity project, the xdyn physics engine, the web UI — please cite the IROS 2026
paper. Its [paper page](https://irl-crossing-cnrs.github.io/resources/lotusim-iros2026)
presents the work: abstract, figures and video.

```bibtex
@inproceedings{buche2026lotusim,
  title     = {{LOTUSim: Multi-Domain Simulator for Marine Robotics}},
  author    = {Buche, C{\'e}dric and Grosset, Juliette and Lech{\^e}ne, H{\'e}l{\`e}ne and Dubromel, Marie and Havez-Bodivit, Pierig and Neo, Malcom and Prodhon, Julien},
  booktitle = {2026 IEEE/RSJ International Conference on Intelligent Robots and Systems (IROS)},
  address   = {Pittsburgh, PA, USA},
  year      = {2026},
  month     = sep,
  url       = {https://hal.science/hal-05677110},
  note      = {HAL: hal-05677110}
}
```

If you use LOTUSim-Energy — the offshore wind farm scenarios, the wind and
wake models or the Unity offshore scene — please also cite the IROS 2026
workshop paper. Its [paper page](https://irl-crossing-cnrs.github.io/resources/lotusim-energy-iros2026)
presents the work: abstract, figures and video.

```bibtex
@inproceedings{grosset2026lotusimenergy,
  title         = {{LOTUSim-Energy: A Maritime Simulator for Human-Drone Interaction in Autonomous Offshore Operation \& Maintenance}},
  author        = {Grosset, Juliette and Dubromel, Marie and Lech{\^e}ne, H{\'e}l{\`e}ne and Arzel, Quentin and Buche, C{\'e}dric},
  booktitle     = {AQ$^2$UASIM-V2: Advancing Quantitative and QUAlitative SIMulators for marine applications Workshop, IEEE/RSJ International Conference on Intelligent Robots and Systems (IROS)},
  address       = {Pittsburgh, PA, USA},
  year          = {2026},
  month         = oct,
  url           = {https://hal.science/hal-05741707},
  eprint        = {2609.17124},
  archivePrefix = {arXiv}
}
```

### Code

[Core simulator](https://github.com/IRL-Crossing-CNRS/LOTUSim) ·
[Scenario workspace](https://github.com/IRL-Crossing-CNRS/LOTUSim-generic-scenario) ·
[Unity project](https://github.com/IRL-Crossing-CNRS/LOTUSim-Energy) ·
[Installer](https://github.com/IRL-Crossing-CNRS/How_To_Install_LOTUSim) ·
[Naval Group upstream](https://github.com/naval-group/LOTUSim)

```{toctree}
:caption: Get started
:hidden:

What LOTUSim is made of <overview>
Install <_collected/install/index>
Run a generic scenario <_collected/scenario/README>
Generic scenario: launch options <_collected/scenario/doc/LAUNCH_OPTIONS>
Run from Windows (WSL) <_collected/scenario/doc/WINDOWS_WSL>
Build, rebuild and source <rebuild>
Unity player problems <unity-troubleshooting>
```

```{toctree}
:caption: User guide
:hidden:

Generic scenario reference <_collected/scenario/doc/scenario/README>
Generic scenario examples <_collected/scenario/src/simulation_run/config/README>
Sensors on a vehicle <_collected/core/systems/sensors/README>
PX4 drones <_collected/scenario/doc/PX4_DRONES>
Run faster than real time <_collected/scenario/doc/ACCELERATED_SIMULATION>
Run agents on another machine <_collected/scenario/deployment/README>
Drive the LRAUV propeller by hand <_collected/scenario/doc/LRAUV_PROPELLER_DEMO>
```

```{toctree}
:caption: Developer guide
:hidden:

How LOTUSim is built <dev/architecture>
Missions, navigation and control <dev/missions-gnc>
Physics and environment <dev/physics>
Extend LOTUSim <dev/extend>
Core plugins <dev/core-plugins>
Unity project <dev/unity>
```

```{toctree}
:caption: API reference
:hidden:

Generic scenario: Python API <api/python>
Core simulator: C++ API <api/cpp>
ROS 2 messages <messages>
```

```{toctree}
:caption: Contribute
:hidden:

How to contribute <contributing>
```
