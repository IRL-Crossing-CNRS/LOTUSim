uint8 BOX=0
uint8 CONE_SEGMENT=1

string id
uint8 shape_type

WindRegionBox box
WindRegionConeSegment cone

geometry_msgs/Vector3 linear_velocity
bool enable_wind

# Standard deviation [m/s] of the turbulent fluctuation about linear_velocity
# inside this region. 0.0 (the default, and what every pre-existing publisher
# produces) means a steady region, so this field is additive: nothing changes
# for a producer that does not set it.
#
# Sigma in m/s rather than a dimensionless turbulence intensity, deliberately:
# an intensity is a ratio to *some* reference speed, and in a wake the local
# (deficit) speed and the freestream differ substantially. Crespo-type
# wake-added turbulence intensity is defined against the freestream, but
# linear_velocity above carries the deficit speed — so a consumer handed a
# ratio and this message would have no way to recover the right reference and
# would silently normalise by the wrong one. Putting sigma on the wire keeps
# the convention where the knowledge is, in the producer.
float64 turbulence_sigma 0.0
