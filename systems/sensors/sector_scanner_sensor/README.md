# Sector Scanner Sensor

A sonar or radar that sweeps the horizon one sector at a time and reports the
vessels it sees in the current sector: their name, range and bearing.

## Use it

```xml
<sensor name="sector_scanner" type="custom" gz:type="sector_scanner">
    <sector_width_deg>45</sector_width_deg>
    <dwell_time_s>1.0</dwell_time_s>
    <max_range>500</max_range>
    <vertical_beamwidth_deg>30</vertical_beamwidth_deg>
</sensor>
```

| Parameter | Default | Meaning |
|---|---|---|
| `sector_width_deg` | `45` | Width of a sector. Rounded so a whole number of sectors covers 360° |
| `dwell_time_s` | `1.0` | Time spent on each sector before moving to the next, simulated time |
| `max_range` | `500` | m |
| `vertical_beamwidth_deg` | `30` | Full vertical width, centred on the sensor's horizontal plane |
| `lotusim_power` | | Optional: the sensor only scans while the power subsystem powers it |

## Output

`/<world>/<vessel>/<sensor>/scan`, `lotusim_sensor_msgs/msg/SectorScan`:

| Field | Meaning |
|---|---|
| `current_sector`, `sector_center_deg`, `sector_width_deg`, `max_range` | The sector being scanned |
| `contacts[]` | One `SectorScanContact` per target in it: `target_name`, `range` (m), `bearing_deg` (clockwise from the bow) |

`scripts/sector_scanner_viewer.py` draws the scan live:

```bash
python3 scripts/sector_scanner_viewer.py --topic /<world>/<vessel>/sector_scanner/scan
```

## Limits

- Purely geometric: no occlusion (a target behind another hull or terrain is
  still reported) and no target strength.
- It publishes every simulation step; `update_rate` is not used.
