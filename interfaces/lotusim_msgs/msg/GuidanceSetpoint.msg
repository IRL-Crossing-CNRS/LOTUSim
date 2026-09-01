std_msgs/Header header

# True: Control must drive directly toward (target_x, target_y) in the
# horizontal plane. False: Control tracks desired_heading and desired_speed
# instead.
bool use_position_hold

float64 target_x              # m, valid only if use_position_hold
float64 target_y              # m, valid only if use_position_hold

float64 desired_heading       # rad, positive clockwise from above
float64 desired_depth         # m, positive down
float64 desired_speed         # m/s, surge -- valid only if not use_position_hold

float64 cross_track_error     # m, signed; 0 if use_position_hold
float64 along_track_distance  # m, from the start of the current leg; 0 if use_position_hold
bool arrived
