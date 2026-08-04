# One tapered frustum segment of a turbine's wake, chained with other
# segments (this one's r_end == the next one's r_start) to approximate a
# continuous cone. origin/axis are 2D (world ENU x/y) — altitude is unused,
# matching WindRegion's existing "all altitudes" convention.
geometry_msgs/Point origin
geometry_msgs/Vector3 axis
float64 length
float64 r_start
float64 r_end
