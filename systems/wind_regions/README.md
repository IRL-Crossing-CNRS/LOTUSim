# Wind Regions

A Gazebo world plugin that blows wind on the aerial vehicles: one ambient
wind for the whole world, plus any number of regions, each with its own wind,
received live over ROS 2. It replaces Gazebo's own `WindEffects`, which has a
single wind vector for the whole world and cannot change it by place.

The wind and wake *models* (turbine wakes, the `Wind` and `Wake` agents, the
message definitions) are described in the scenario workspace's
**Wind and wake** page (`doc/WAKE_EFFECT.md`). This page is the plugin itself.

## Use it

In the world SDF (`assets/worlds/aerialWorld.world` declares it):

```xml
<plugin filename="wind_regions_plugin" name="lotusim::gazebo::WindRegionsPlugin">
  <scaling_factor>0.35</scaling_factor>
  <!-- all optional; these defaults leave turbulence off -->
  <ambient_turbulence_sigma>0.0</ambient_turbulence_sigma>
  <turbulence_time_constant>1.0</turbulence_time_constant>
  <turbulence_sigma_ratio_v>0.8</turbulence_sigma_ratio_v>
  <turbulence_sigma_ratio_w>0.5</turbulence_sigma_ratio_w>
  <turbulence_seed>0</turbulence_seed>
</plugin>
```

and on every link that should feel the wind, as for stock `WindEffects`:

```xml
<enable_wind>true</enable_wind>
```

| Parameter | Default | Meaning |
|---|---|---|
| `scaling_factor` | `1.0` | Gain of the drag-like force below |
| `ambient_turbulence_sigma` | `0.0` | Standard deviation of the gusts of the ambient wind, m/s. Regions carry their own |
| `turbulence_time_constant` | `1.0` | Correlation time of the gusts, s |
| `turbulence_sigma_ratio_v`, `turbulence_sigma_ratio_w` | `0.8`, `0.5` | Cross-wind and vertical gust strength, relative to the along-wind one (IEC 61400-1) |
| `turbulence_seed` | `0` | Random seed of the gusts |

## Inputs

| Topic | Message | Meaning |
|---|---|---|
| `/aerialWorld/wind` | `lotusim_msgs/Wind` | Ambient wind. Also published by Unity's wind sliders |
| `/aerialWorld/wind/regions` | `lotusim_msgs/WindRegionArray` | The whole list of regions, replaced at once |

A region is a box (a vertical column, all altitudes) or a cone segment (a
turbine wake). The **last** region of the list that contains a link wins; a
link in no region gets the ambient wind.

## How it is implemented

`src/wind_regions_plugin.cpp`. Each step, for each wind-enabled link: find the
region containing its `x, y` (and `z` for a cone), add the gust, then apply
`mass × scaling_factor × (wind − link velocity)` with `Link::AddWorldForce`.
The force is the one stock `WindEffects` uses, so the tuning carries over.
Gusts are an Ornstein-Uhlenbeck process per axis, resolved along the mean
wind, across it and vertically.

**Add a region shape:** subclass `RegionShape` (one `Contains()` method) in
`wind_regions_plugin.hpp` and add one line in `MakeShape()`, then a matching
shape message in `lotusim_msgs`.

## Limits

- Only Gazebo-simulated links feel it: the aerial world's drones. Vessels on
  xdyn or Kinematic get no wind from this plugin.
- Boxes have no altitude bound.
