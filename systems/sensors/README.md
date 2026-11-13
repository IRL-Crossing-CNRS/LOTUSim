# Sensors

The sensors a vehicle can carry: what each one measures, the SDF that puts
it on a vehicle, and the topic it publishes. They are declared in the
vehicle's SDF model, inside a link, as `<sensor type="custom" gz:type="...">`.

The world must load the sensor plugin once, which the worlds of
`assets/worlds/` already do:

```xml
<plugin filename="lotusim_sensor_plugin" name="lotusim::sensor::LotusimSensorPlugin">
</plugin>
```

Every sensor can be turned off and on at runtime with the service
`/<world>/<vessel>/<sensor>/change_state` (`lotusim_sensor_msgs/srv/ActivateSensor`).

How the sensors are implemented, and how to add one:
[Sensors: implementation](DEVELOPER.md).

## The sensors

Each sensor has its own page: what it measures, every parameter, its output and its limits.

| Sensor | `gz:type` | Publishes | Notes |
|---|---|---|---|
| [AIS](ais_sensor/README.md) | `ais` | `<vessel>/<sensor>/ais` (`lotusim_sensor_msgs/msg/AIS`) | Speed-over-ground and heading, derived from the `base_link`'s world velocity |
| [IMU](imu_sensor/README.md) | `imu` | `<vessel>/<sensor>/IMU` (`sensor_msgs/msg/Imu`) | Orientation, angular velocity, linear acceleration |
| [Radar](radar_sensor/README.md) | `radar` | see [Radar Sensor README](radar_sensor/README.md) | Needs a co-located `gpu_lidar` sensor on the same link |
| [Battery](battery_sensor/README.md) | `battery` | `<vessel>/battery/state` (`sensor_msgs/msg/BatteryState`), also on gz `/model/<vessel>/battery/<battery_name>/state` | Linear battery model drained by the RPM commands of `vessel_cmd_array` |
| [Sector scanner](sector_scanner_sensor/README.md) | `sector_scanner` | `<vessel>/<sensor>/scan` (`lotusim_sensor_msgs/msg/SectorScan`) | Sweeps fixed azimuth sectors and reports the models in range; purely geometric, no occlusion. `scripts/sector_scanner_viewer.py` plots it |
| [Subsea pressure](subsea_pressure_sensor/README.md) | `subsea_pressure` | `<vessel>/<sensor>/pressure_sensor` (`lotusim_sensor_msgs/msg/PressureDepth`) | Pressure derived from depth; can optionally back-estimate depth from pressure |

Minimal SDF for AIS and IMU:

```xml
<sensor name="ais_sensor" type="custom" gz:type="ais">
    <update_rate>1</update_rate>
    <noise_sigma>0.01</noise_sigma>
    <noise_amplitude>0.01</noise_amplitude>
</sensor>

<sensor name="imu_sensor" type="custom" gz:type="imu">
    <update_rate>1</update_rate>
    <noise_sigma>0.01</noise_sigma>
    <noise_amplitude>0.01</noise_amplitude>
</sensor>
```

Subsea pressure adds its own parameters:

```xml
<sensor name="pressure_sensor" type="custom" gz:type="subsea_pressure">
    <update_rate>1</update_rate>
    <m_saturation>3000</m_saturation>          <!-- kPa, output cap -->
    <estimate_depth_on>true</estimate_depth_on>
    <standard_pressure>101.325</standard_pressure>  <!-- kPa at surface -->
    <kPa_per_meter>9.80638</kPa_per_meter>
</sensor>
```

Battery and sector scanner (defaults shown for the scanner; the battery's full
parameter list is in `battery_sensor/include/battery_sensor/battery_sensor.hpp`):

```xml
<!-- from assets/models/bluerov2_heavy/model-battery.sdf -->
<sensor name="battery_sensor" type="custom" gz:type="battery">
    <battery_name>linear_battery</battery_name>  <!-- required -->
    <voltage>48.0</voltage>                      <!-- V, required -->
    <open_circuit_voltage_constant_coef>50.4</open_circuit_voltage_constant_coef>
    <open_circuit_voltage_linear_coef>-3.1</open_circuit_voltage_linear_coef>
    <capacity>60.0</capacity>                    <!-- Ah, required, > 0 -->
    <resistance>0.05</resistance>                <!-- Ohm -->
    <smooth_current_tau>2.0</smooth_current_tau> <!-- s -->
    <power_load>20.0</power_load>                <!-- W, constant baseline draw -->
    <start_draining>false</start_draining>
    <fix_issue_225>true</fix_issue_225>
</sensor>

<sensor name="sector_scanner" type="custom" gz:type="sector_scanner">
    <update_rate>10</update_rate>
    <sector_width_deg>45</sector_width_deg>           <!-- rounded so it divides 360 -->
    <dwell_time_s>1.0</dwell_time_s>                  <!-- time spent on each sector -->
    <max_range>500</max_range>                        <!-- m -->
    <vertical_beamwidth_deg>30</vertical_beamwidth_deg>
</sensor>
```

For radar's setup (it needs a paired LiDAR sensor and has several tuning parameters), see the [Radar Sensor README](radar_sensor/README.md).

Every sensor also accepts an optional `<lotusim_power>` block to draw from the vessel's battery - see the [Power Subsystem README](../../power_subsystem/README.md#add-a-sensor).
