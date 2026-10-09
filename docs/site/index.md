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

Three steps take you from nothing to your own mission.

::::{grid} 1 1 3 3
:gutter: 3

:::{grid-item-card} 1 · Install
:link: _collected/install/index
:link-type: doc
:class-card: lotusim-card lotusim-step lotusim-step-1

Choose an installation and follow its guide to the end.
:::

:::{grid-item-card} 2 · Run a scenario
:link: _collected/scenario/README
:link-type: doc
:class-card: lotusim-card lotusim-step lotusim-step-2

Launch one of the example scenarios and watch it in Unity.
:::

:::{grid-item-card} 3 · Write your own
:link: _collected/scenario/doc/WRITE_SCENARIO
:link-type: doc
:class-card: lotusim-card lotusim-step lotusim-step-3

Describe vehicles, missions and sensors in a scenario JSON file.
:::
::::

## Go further

::::{grid} 1 2 2 4
:gutter: 3

:::{grid-item-card} Architecture
:link: _collected/core/docs/DEVELOPER
:link-type: doc
:class-card: lotusim-card lotusim-topic

How the plugins, the xdyn bridge and the scenario runner fit together.
:::

:::{grid-item-card} Navigation & control
:link: _collected/scenario/doc/GNC_MODULAR_ARCHITECTURE
:link-type: doc
:class-card: lotusim-card lotusim-topic

The guidance, navigation and control pipeline every vehicle runs.
:::

:::{grid-item-card} Unity
:link: _collected/unity/README
:link-type: doc
:class-card: lotusim-card lotusim-topic

The 3D scene, its cameras and what it receives from the simulator.
:::

:::{grid-item-card} Messages
:link: _collected/reference/lotusim_msgs
:link-type: doc
:class-card: lotusim-card lotusim-topic

Every ROS 2 message the simulator publishes or listens to.
:::
::::

## Papers, videos & code

::::{grid} 1 1 2 2
:gutter: 3

:::{grid-item}
**LOTUSim** · IROS 2026\
[arXiv:2607.03072](https://arxiv.org/abs/2607.03072) ·
[HAL](https://hal.science/hal-05677110)

```{raw} html
<div class="lotusim-video"><iframe src="https://www.youtube-nocookie.com/embed/iXDz8ZqSpq4" title="LOTUSim video" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" allowfullscreen loading="lazy"></iframe></div>
```
:::

:::{grid-item}
**LOTUSim-Energy** · IROS 2026 workshop\
[arXiv:2609.17124](https://arxiv.org/abs/2609.17124) ·
[HAL](https://hal.science/hal-05741707)

```{raw} html
<div class="lotusim-video"><iframe src="https://www.youtube-nocookie.com/embed/NRd-LjRrrac" title="LOTUSim-Energy video" allow="accelerometer; autoplay; clipboard-write; encrypted-media; gyroscope; picture-in-picture" allowfullscreen loading="lazy"></iframe></div>
```
:::
::::

### Cite LOTUSim

If you use [LOTUSim](https://github.com/naval-group/LOTUSim) in your research,
or any of the repositories directly linked to it — the scenario workspace, the
Unity project, the xdyn physics engine, the web UI — please cite the IROS 2026
paper:

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
workshop paper:

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

How to install <_collected/install/index>
Run a scenario <_collected/scenario/README>
Write a scenario <_collected/scenario/doc/WRITE_SCENARIO>
Scenario examples <_collected/scenario/src/simulation_run/config/README>
```

```{toctree}
:caption: Guides
:hidden:

Missions and behaviour trees <_collected/scenario/doc/MISSIONS>
Faster than real time <_collected/scenario/doc/ACCELERATED_SIMULATION>
Agents on a remote machine <_collected/scenario/deployment/README>
Jupyter examples <_collected/core/examples/jupyter-python/README>
```

```{toctree}
:caption: Architecture
:hidden:

Core simulator <_collected/core/docs/DEVELOPER>
Coordinate frames <_collected/core/docs/coordinate_frames>
Scenario workspace <_collected/scenario/doc/ARCHITECTURE>
Scenario workspace: developer guide <_collected/scenario/doc/DEVELOPER>
Navigation, guidance & control <_collected/scenario/doc/GNC_MODULAR_ARCHITECTURE>
Wind and wake <_collected/scenario/doc/WAKE_EFFECT>
Diagrams <_collected/scenario/doc/DIAGRAMS>
```

```{toctree}
:caption: PX4
:hidden:

PX4 integration <_collected/core/docs/PX4_Documentation/PX4_Integration_Technical>
PX4 data flow <_collected/core/docs/PX4_Documentation/PX4_Flow_Diagram>
Autonomous navigation <_collected/core/docs/PX4_Documentation/AUTONAV_RECOMMENDATION>
PX4 path vs ROS waypoints <_collected/core/docs/PX4_Documentation/AUTONAV_PX4_VS_DIRECT_ROS_WAYPOINT_COMPARISON>
```

```{toctree}
:caption: Unity
:hidden:

Unity project <_collected/unity/README>
```

```{toctree}
:caption: Reference
:hidden:
:glob:

_collected/reference/*
```

```{toctree}
:caption: Contribute
:hidden:

How to contribute <contributing>
```
