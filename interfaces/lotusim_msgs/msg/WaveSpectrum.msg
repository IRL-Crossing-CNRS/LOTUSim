# Wave components of the sea state, the same list every xdyn vehicle model
# reads, so a renderer can draw the surface the physics computes.
#
# Surface elevation in the world NED frame (z down), t the simulation time:
#   zeta(x, y, t) = - sum_i g_i(t) a[i] * sin(k[i] * (x cos(psi[i]) + y sin(psi[i])) - omega[i] * t + phase[i])
# psi is the direction a component travels to, from north (x) towards east (y).
#
# When the sea state changes during the run, the old components fade out while
# the new ones fade in, linearly in simulation time:
#   g_i(t) = clamp((t - fade_in_start[i]) / fade_in_duration[i], 0, 1)
#          * (1 - clamp((t - fade_out_start[i]) / fade_out_duration[i], 0, 1))
# A duration of 0 is a step at its start time; a start of 1e12 never comes.

# Sea state the components were drawn from, for display only.
float64 significant_height_m
float64 peak_period_s
float64 direction_deg

float64[] a       # amplitude (m)
float64[] psi     # propagation direction (rad)
float64[] omega   # angular frequency (rad/s)
float64[] k       # wave number (rad/m)
float64[] phase   # phase (rad)
float64[] fade_in_start       # s
float64[] fade_in_duration    # s
float64[] fade_out_start      # s
float64[] fade_out_duration   # s
