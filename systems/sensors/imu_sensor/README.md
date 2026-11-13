# IMU Sensor

Orientation, angular velocity and linear acceleration of the link it is
attached to, as an inertial measurement unit reports them.

## Use it

```xml
<sensor name="imu_sensor" type="custom" gz:type="imu">
    <update_rate>1</update_rate>
</sensor>
```

| Parameter | Default | Meaning |
|---|---|---|
| `update_rate` | `1` | Measurements per second of simulated time |
| `is_on` | `true` | Starts the sensor on or off |

## Output

`/<world>/<vessel>/<sensor>/IMU`, `sensor_msgs/msg/Imu`:

| Field | Meaning |
|---|---|
| `orientation` | World attitude of the link (ENU) |
| `angular_velocity` | In the sensor frame, rad/s |
| `linear_acceleration` | In the sensor frame, m/s², **without gravity**: a vessel at rest reads zero |

The service `/<world>/<vessel>/<sensor>/change_state` turns it on and off.

## Limits

- `noise_sigma` and `noise_amplitude` are read but not applied.
- `update_rate` is compared to whole seconds, so the sensor publishes at most
  once per second, and the acceleration is averaged over that second.
- No covariance is filled in the message.
