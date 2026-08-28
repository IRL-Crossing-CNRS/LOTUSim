# LOTUSim

![Different drones in LOTUSim.](docs/lotusim_environment.png)

LOTUSim is a real-time maritime simulation platform for human-vehicle teaming. It simulates surface, underwater, and aerial physics in a multi-agent setting; an agent can be an aerial drone, a surface ship, or an underwater vehicle. It provides an operator interface for human-autonomous agent scenarios, and can be used to train AI algorithms against its physics models.

For more information, please read our [wiki here](https://github.com/naval-group/LOTUSim/wiki)

For issues or questions about the simulation, please create an issue on the issue board.

If you are interested in partnership or have questions regarding contributing to LOTUSim, please send an email to [LOTUSim support email](mailto:lotusim_support@naval-group.com) `lotusim_support@naval-group.com`.

Upcoming open-source publication under [EPL-2.0](LICENSE).

## Where to start

This repository is the **core**: the Gazebo system plugins, the bridge to the
xdyn hydrodynamic solver, the ROS 2 message definitions, the vehicle models
and the worlds. It holds no mission logic and no scenario.

| You want to | Go to |
|---|---|
| Run a simulation, write a scenario, fly or pilot a vehicle | The scenario workspace `LOTUSim-generic-scenario` and its `README.md` |
| Change the simulator itself — a plugin, a message, a vehicle model, the physics bridge | [docs/DEVELOPER.md](docs/DEVELOPER.md) |
| Know which frame is used where, and why a trajectory looks mirrored | [docs/coordinate_frames.md](docs/coordinate_frames.md) |

## Video
A demonstrative video of LOTUSim is available on YouTube:

[![LOTUSim Video - IROS2026](https://img.youtube.com/vi/iXDz8ZqSpq4/0.jpg)](https://www.youtube.com/watch?v=iXDz8ZqSpq4)

## Relevant Publications

If you use [LOTUSim](https://github.com/naval-group/LOTUSim) in your research, or any of the repositories directly linked to LOTUSim
- [LOTUSim-Xdyn](https://github.com/naval-group/LOTUSim-Xdyn),
- [LOTUSim-generic-scenario](https://github.com/naval-group/LOTUSim-generic-scenario),
- [LOTUSim-Unity-modules](https://github.com/naval-group/LOTUSim-Unity-modules),
- [LOTUSim-UI-frontend](https://github.com/naval-group/LOTUSim-UI-frontend),
- [LOTUSim-UI-backend](https://github.com/naval-group/LOTUSim-UI-backend),

Please cite:

```bibtex
@inproceedings{LOTUSim26iros,
  title     = {{LOTUSim}: Multi-Domain Simulator for Marine Robotics},
  author    = {Buche, Cedric and Grosset, Juliette and Lechene, Helene and Dubromel, Marie and Havez-Bodivit, Pierig and Neo, Malcom and Prodhon, Julien},
  booktitle = {2026 IEEE/RSJ International Conference on Intelligent Robots and Systems (IROS)},
  year      = {2026},
  publisher = {IEEE}
}
```