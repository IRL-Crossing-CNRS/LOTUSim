# Light Actuator

Turns a status light of a model on and off from ROS 2. It is the example of
an actuator plugin: one SDF declaration, one topic in, one effect in the
world.

## Use it

Inside the `<model>`, next to the visual that should glow:

```xml
<plugin filename="light_actuator" name="lotusim::actuator::LightActuatorPlugin">
  <visual_name>led_body_visual</visual_name>
</plugin>
```

`assets/models/bluerov2_heavy/model-battery.sdf` has a complete example.

| Parameter | Default | Meaning |
|---|---|---|
| `visual_name` | `led_body_visual` | The `<visual>` whose colour changes |

Command it:

```bash
ros2 topic pub --once /<world>/<model>/light/cmd std_msgs/msg/Bool '{data: true}'
```

`true` makes the visual glow red, `false` turns it black.

## How it is implemented

`src/light_actuator.cpp`. A ROS 2 thread stores the latest command; on the
next `PreUpdate` the plugin sets the visual's material emissive colour
through a `VisualCmd` component. An emissive colour is used rather than a
`<light>` because it shows in the GUI render and glows regardless of the
scene lighting.

## Write another actuator

Copy this package: a `gz::sim::System` with `ISystemConfigure` (read the SDF,
create the ROS 2 node and subscription) and `ISystemPreUpdate` (apply the
latest command to the ECM). Keep the ROS callback away from the ECM: store
the command under a mutex and apply it in `PreUpdate`, as here.

## Limits

- The colour is fixed (red on, black off).
- Gazebo's own render only: Unity does not show the light.
