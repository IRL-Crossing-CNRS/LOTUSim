# Subsea Pressure Sensor

Water pressure at the sensor's depth, and optionally the depth a real
pressure sensor would infer from it.

## Use it

```xml
<sensor name="pressure_sensor" type="custom" gz:type="subsea_pressure">
    <update_rate>1</update_rate>
    <noise_amplitude>0.5</noise_amplitude>        <!-- kPa -->
    <estimate_depth_on>true</estimate_depth_on>
</sensor>
```

| Parameter | Default | Meaning |
|---|---|---|
| `update_rate` | `1` | Measurements per second of simulated time |
| `is_on` | `true` | Starts the sensor on or off |
| `noise_sigma`, `noise_amplitude` | `0`, `0` | Gaussian noise added to the pressure: `noise_amplitude × N(0, noise_sigma)`, kPa |
| `standard_pressure` | `101.325` | Pressure at the surface, kPa |
| `kPa_per_meter` | `9.80638` | Pressure gained per metre of depth |
| `estimate_depth_on` | `false` | Fill `depth` with the depth inferred from the noisy pressure |
| `m_saturation` | `3000` | Read but not applied yet |

## Output

`/<world>/<vessel>/<sensor>/pressure_sensor`, `lotusim_sensor_msgs/msg/PressureDepth`:

| Field | Meaning |
|---|---|
| `pressure` | `standard_pressure + depth × kPa_per_meter + noise`, kPa |
| `depth` | `(pressure − standard_pressure) / kPa_per_meter` when `estimate_depth_on`, else `0` |

The service `/<world>/<vessel>/<sensor>/change_state` turns it on and off.

## Limits

- No saturation, no temperature or salinity effect: pressure is linear in
  depth.
- `update_rate` is compared to whole seconds: at most one measurement per
  second.
