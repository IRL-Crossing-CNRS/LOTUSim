# AIS Sensor

Reports what an AIS transponder broadcasts: the vessel's position, its true
heading, and its speed and course over ground.

## Use it

Inside a `<link>` of the vessel's SDF:

```xml
<sensor name="ais_sensor" type="custom" gz:type="ais">
    <update_rate>1</update_rate>
</sensor>
```

| Parameter | Default | Meaning |
|---|---|---|
| `update_rate` | `1` | Measurements per second of simulated time |
| `is_on` | `true` | Starts the sensor on or off |

## Output

`/<world>/<vessel>/<sensor>/ais`, `lotusim_sensor_msgs/msg/AIS`:

| Field | Meaning |
|---|---|
| `name` | Vessel name |
| `latitude`, `longitude` | Position, degrees |
| `true_heading` | Where the bow points, degrees clockwise from North |
| `sog` | Speed over ground, horizontal only, m/s |
| `cog` | Course over ground: where the vessel travels, degrees clockwise from North. 360, the AIS "not available" value, below 0.05 m/s, where the course is noise |
| `navigation_status`, `rot`, `positional_accuracy` | Not filled yet |

`true_heading` and `cog` differ by the crab angle when a current pushes the
vessel off its heading.

The service `/<world>/<vessel>/<sensor>/change_state`
(`lotusim_sensor_msgs/srv/ActivateSensor`) turns it on and off at runtime.

## Limits

- `noise_sigma` and `noise_amplitude` are read but not applied: the readings
  are exact.
- `update_rate` is compared to whole seconds (`CustomSensor::EnableMeasurement`),
  so the sensor publishes at most once per second whatever the rate.
