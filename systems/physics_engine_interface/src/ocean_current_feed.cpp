/*
 * Copyright (c) 2025 Naval Group
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * https://www.eclipse.org/legal/epl-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0
 */
#include "physics_engine_interface/ocean_current_feed.hpp"

#include "physics_engine_interface/kinematic_interface.hpp"

namespace lotusim::gazebo {

OceanCurrentFeed::OceanCurrentFeed(const rclcpp::Node::SharedPtr& node)
{
    m_sub = node->create_subscription<geometry_msgs::msg::Vector3>(
        "ocean_current",
        rclcpp::QoS(1).transient_local(),
        [](geometry_msgs::msg::Vector3::ConstSharedPtr msg) -> void {
            KinematicInterface::getInstance()->setCurrent(msg->x, msg->y);
        });
}

}  // namespace lotusim::gazebo
