/*
 * Copyright (c) 2025 Naval Group
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * https://www.eclipse.org/legal/epl-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0
 */
#ifndef LOTUSIM_OCEAN_CURRENT_FEED_HH_
#define LOTUSIM_OCEAN_CURRENT_FEED_HH_

#include <geometry_msgs/msg/vector3.hpp>
#include <rclcpp/rclcpp.hpp>

namespace lotusim::gazebo {

/**
 * @brief Fake, uniform ocean current feed for KinematicInterface.
 *
 * NOT a general physics feature, and deliberately kept out of
 * PhysicsInterfacePlugin/kinematic_interface.cpp's own files: it only
 * affects entities on the Kinematic connection type (a manual/demo
 * fallback with no real hydrodynamics to begin with). A vessel that wants a
 * physically-simulated current should use XDynWebSocket instead, whose own
 * hydrodynamic config already carries a real "ekman current" model — this
 * class is unrelated to that path and never touches it. Living in its own
 * translation unit means a deployment that doesn't want this fake-current
 * demo feature can simply not construct one (or drop this file from the
 * build) without touching any of the shared plugin code.
 *
 * Subscribes once to `<node namespace>/ocean_current`
 * (geometry_msgs/Vector3, world-frame ENU m/s, TRANSIENT_LOCAL so a single
 * latched publish from the scenario runner is delivered regardless of
 * subscribe/publish ordering) and forwards every message straight into
 * KinematicInterface::setCurrent().
 */
class OceanCurrentFeed {
public:
    explicit OceanCurrentFeed(const rclcpp::Node::SharedPtr& node);

private:
    rclcpp::Subscription<geometry_msgs::msg::Vector3>::SharedPtr m_sub;
};

}  // namespace lotusim::gazebo

#endif
