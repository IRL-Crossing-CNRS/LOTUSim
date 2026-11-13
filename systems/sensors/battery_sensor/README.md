# Battery Sensor

A battery that the vessel's propellers drain: state of charge, voltage and
current, published like a real battery management system would.

The [power subsystem](../../power_subsystem/README.md) is the other, more
complete battery model (several providers, generators, load shedding). This
sensor is the simple one: one battery, drained by the thrusters' RPM.

## Use it

```xml
<!-- from assets/models/bluerov2_heavy/model-battery.sdf -->
<sensor name="battery_sensor" type="custom" gz:type="battery">
    <battery_name>linear_battery</battery_name>
    <voltage>48.0</voltage>
    <open_circuit_voltage_constant_coef>50.4</open_circuit_voltage_constant_coef>
    <open_circuit_voltage_linear_coef>-3.1</open_circuit_voltage_linear_coef>
    <capacity>60.0</capacity>
    <resistance>0.05</resistance>
    <smooth_current_tau>2.0</smooth_current_tau>
    <power_load>20.0</power_load>
    <start_draining>false</start_draining>
    <fix_issue_225>true</fix_issue_225>
</sensor>
```

| Parameter | Default | Meaning |
|---|---|---|
| `battery_name` | required | Name in the topics |
| `voltage` | required | Initial open-circuit voltage, V |
| `open_circuit_voltage_constant_coef`, `open_circuit_voltage_linear_coef` | | `e0`, `e1` of the open-circuit voltage `e0 + e1 × (1 − state of charge)` |
| `capacity` | required, > 0 | Ah |
| `initial_charge` | `capacity` | Ah |
| `resistance` | | Internal resistance, Ω |
| `smooth_current_tau` | | Low-pass time constant of the current, s |
| `power_load` | `0` | Draw while draining with the propellers idle, W. Replaced by the propellers' draw as soon as they turn |
| `start_draining` | `false` | Drain from the start, before any propeller command |
| `enable_recharge`, `charging_time` | `false` | Recharge in `charging_time` hours |
| `recharge_by_topic`, `power_draining_topic`, `stop_power_draining_topic` | | Gazebo topics that start and stop draining or charging |
| `fix_issue_225` | | The gz-common linear battery fix of the same name |

## Output

- `/<world>/<vessel>/battery/state`, `sensor_msgs/msg/BatteryState`,
  TRANSIENT_LOCAL so a late subscriber still gets the last state. This is
  the one remote ROS-only agents read.
- Gazebo transport: `/model/<vessel>/battery/<battery_name>/state`
  (`gz.msgs.BatteryState`) and `/model/<vessel>/battery/power`.

## Limits

- Only commands with an `(rpm)` key drain the battery. A vessel commanded in
  thrust (the WAM-V's `(T)`) or kinematically never drains it.
- The RPM polynomial is the same for every vessel.
