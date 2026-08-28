string vessel_name

geometry_msgs/Pose pose
# Orientation uses front-left-up
geographic_msgs/GeoPoint geo_point

# World-frame (ENU) linear and angular velocity of the vessel's base_link.
#
# Added because a control loop closing on `poses` had no other way to get a
# velocity: it had to differentiate the position, which is noisy and lags by a
# sample. The physics plugin already holds these values
# (Link::WorldLinearVelocity / WorldAngularVelocity), so publishing them is free.
#
# `has_twist` is false when the vessel's base link has no velocity checks
# enabled (only vessels driven by the physics interface get them). Check it
# rather than reading an all-zero twist as "stationary".
geometry_msgs/Twist twist
bool has_twist
