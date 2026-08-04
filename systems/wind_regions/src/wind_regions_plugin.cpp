/*
 * Copyright (c) 2026 IRL CROSSING
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * https://www.eclipse.org/legal/epl-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0
 */
#include "wind_regions/wind_regions_plugin.hpp"

#include <gz/plugin/Register.hh>

namespace lotusim::gazebo {
using namespace std::placeholders;

// Hardcoded to match WIND_TOPIC / the new region topic in the ROS-side `Wind`
// agent (wind.py) exactly — like the rest of aerialWorld, this plugin only
// ever runs in that one world, so there is no per-world templating to do.
namespace {
constexpr const char* kWindTopic = "/aerialWorld/wind";
constexpr const char* kWindRegionsTopic = "/aerialWorld/wind/regions";
}  // namespace

WindRegionsPlugin::WindRegionsPlugin()
{
    if (!rclcpp::ok()) {
        rclcpp::init(0, nullptr);
    }
}

WindRegionsPlugin::~WindRegionsPlugin()
{
    if (m_ros_executor) {
        m_ros_executor->cancel();
    }
    if (m_ros_node_thread && m_ros_node_thread->joinable()) {
        m_ros_node_thread->join();
    }
    if (m_logger) {
        m_logger->info(
            "WindRegionsPlugin::~WindRegionsPlugin: successfully shutdown.");
    }
}

void WindRegionsPlugin::Configure(
    const gz::sim::Entity&,
    const std::shared_ptr<const sdf::Element>& _sdf,
    gz::sim::EntityComponentManager& _ecm,
    gz::sim::EventManager&)
{
    m_world_name = lotusim::common::getWorldName(_ecm);
    m_logger = logger::createConsoleAndFileLogger(
        "wind_regions_plugin",
        m_world_name + "_wind_regions_plugin.txt");

    m_scaling_factor = _sdf->Get<double>("scaling_factor", 1.0).first;

    m_ros_node = rclcpp::Node::make_shared("wind_regions_plugin", m_world_name);

    // kWindTopic is written by both the Wind agent (TRANSIENT_LOCAL) and,
    // while the agent is passive, directly by the Unity sliders over
    // ROS-TCP — whose publisher QoS this plugin does not control. A VOLATILE
    // (plain, depth-10) subscription is compatible with either durability;
    // requesting TRANSIENT_LOCAL here would silently drop all messages if
    // the Unity-side publisher happens to be VOLATILE (DDS QoS
    // incompatibility yields zero delivery, not a fallback). Matches the
    // QoS the old wind_ros_to_gz_bridge used for the same reason.
    m_wind_sub = m_ros_node->create_subscription<lotusim_msgs::msg::Wind>(
        kWindTopic,
        rclcpp::QoS(10),
        [this](lotusim_msgs::msg::Wind::ConstSharedPtr msg) -> void {
            std::lock_guard<std::mutex> lock(m_wind_mutex);
            m_global_wind.Set(
                msg->linear_velocity.x,
                msg->linear_velocity.y,
                msg->linear_velocity.z);
            m_global_enable_wind = msg->enable_wind;
        });

    // kWindRegionsTopic, unlike kWindTopic, is only ever written by the Wind
    // agent — both ends are ours, so TRANSIENT_LOCAL is safe here and means
    // this plugin gets the current region list immediately on startup
    // instead of waiting for the agent's next periodic publish.
    m_regions_sub = m_ros_node->create_subscription<
        lotusim_msgs::msg::WindRegionArray>(
        kWindRegionsTopic,
        rclcpp::QoS(1).transient_local(),
        [this](lotusim_msgs::msg::WindRegionArray::ConstSharedPtr msg)
            -> void {
            std::vector<RegionState> regions;
            regions.reserve(msg->regions.size());
            for (auto&& region : msg->regions) {
                RegionState state;
                state.id = region.id;
                state.shape = MakeShape(region);
                state.velocity.Set(
                    region.linear_velocity.x,
                    region.linear_velocity.y,
                    region.linear_velocity.z);
                state.enable_wind = region.enable_wind;
                regions.push_back(std::move(state));
            }
            std::lock_guard<std::mutex> lock(m_wind_mutex);
            m_regions = std::move(regions);
        });

    // Spin on a dedicated thread so both subscriptions are drained on
    // arrival rather than at most one message per Update() (see
    // PhysicsInterfacePlugin's identical m_ros_executor for why spin_some()
    // is not enough here).
    if (rclcpp::ok()) {
        m_ros_executor =
            std::make_shared<rclcpp::executors::SingleThreadedExecutor>();
        m_ros_executor->add_node(m_ros_node);
        m_ros_node_thread = std::make_shared<std::thread>(
            [this]() { m_ros_executor->spin(); });
    } else {
        m_logger->error("WindRegionsPlugin::Configure: RCLCPP context shutdown.");
    }

    m_logger->info(
        "WindRegionsPlugin ready (scaling_factor={}). Subscribed to {} and {}.",
        m_scaling_factor,
        kWindTopic,
        kWindRegionsTopic);
}

std::shared_ptr<const WindRegionsPlugin::RegionShape> WindRegionsPlugin::MakeShape(
    const lotusim_msgs::msg::WindRegion& _region)
{
    if (_region.shape_type == lotusim_msgs::msg::WindRegion::CONE_SEGMENT) {
        return std::make_shared<ConeSegmentShape>(
            gz::math::Vector2d(_region.cone.origin.x, _region.cone.origin.y),
            gz::math::Vector2d(_region.cone.axis.x, _region.cone.axis.y),
            _region.cone.length,
            _region.cone.r_start,
            _region.cone.r_end);
    }
    return std::make_shared<BoxShape>(
        _region.box.x1, _region.box.y1, _region.box.x2, _region.box.y2);
}

bool WindRegionsPlugin::ResolveWind(
    double _x,
    double _y,
    const std::vector<RegionState>& _regions,
    const gz::math::Vector3d& _globalWind,
    bool _globalEnableWind,
    gz::math::Vector3d& _wind)
{
    // Last matching region in the list wins on overlap.
    for (auto it = _regions.rbegin(); it != _regions.rend(); ++it) {
        if (it->Contains(_x, _y)) {
            if (!it->enable_wind) {
                return false;
            }
            _wind = it->velocity;
            return true;
        }
    }
    if (!_globalEnableWind) {
        return false;
    }
    _wind = _globalWind;
    return true;
}

void WindRegionsPlugin::Update(
    const gz::sim::UpdateInfo& _info,
    gz::sim::EntityComponentManager& _ecm)
{
    if (_info.paused) {
        return;
    }

    // Newly spawned wind-enabled links need velocity checks turned on once
    // before WorldLinearVelocity() will return a value (see Link::Enable
    // VelocityChecks docs) — mirrors PhysicsInterfacePlugin::loadVessel.
    _ecm.EachNew<gz::sim::components::Link, gz::sim::components::WindMode>(
        [&](const gz::sim::Entity& _entity,
            const gz::sim::components::Link*,
            const gz::sim::components::WindMode*) -> bool {
            gz::sim::Link(_entity).EnableVelocityChecks(_ecm);
            return true;
        });

    std::vector<RegionState> regions_snapshot;
    gz::math::Vector3d global_wind_snapshot;
    bool global_enable_snapshot;
    {
        std::lock_guard<std::mutex> lock(m_wind_mutex);
        regions_snapshot = m_regions;
        global_wind_snapshot = m_global_wind;
        global_enable_snapshot = m_global_enable_wind;
    }

    // Diagnostic counters for the throttled summary below — cheap, and the
    // only way to tell "no wind-enabled link found" from "found but missing
    // pose/velocity/inertial" from "force applied" without attaching a
    // debugger.
    int links_seen = 0;
    int links_skipped_no_data = 0;
    int links_force_applied = 0;
    gz::math::Vector3d last_force;
    gz::math::Vector3d last_wind;

    _ecm.Each<gz::sim::components::Link, gz::sim::components::WindMode>(
        [&](const gz::sim::Entity& _entity,
            const gz::sim::components::Link*,
            const gz::sim::components::WindMode* _windMode) -> bool {
            if (!_windMode->Data()) {
                return true;
            }
            ++links_seen;

            gz::sim::Link link(_entity);
            auto pose = link.WorldPose(_ecm);
            auto lin_vel = link.WorldLinearVelocity(_ecm);
            auto inertial = link.WorldInertial(_ecm);
            if (!pose || !lin_vel || !inertial) {
                ++links_skipped_no_data;
                return true;
            }

            gz::math::Vector3d wind;
            if (!ResolveWind(
                    pose->Pos().X(),
                    pose->Pos().Y(),
                    regions_snapshot,
                    global_wind_snapshot,
                    global_enable_snapshot,
                    wind)) {
                return true;
            }

            // Matches stock WindEffects: force = mass * scaling_factor *
            // (wind - link_velocity), so mass cancels out of the resulting
            // acceleration (a = scaling_factor * relative_velocity,
            // independent of the link's mass) — without the mass factor a
            // heavier link (e.g. x500's 2 kg base_link) would visibly
            // under-react compared to the stock plugin's behaviour.
            const double mass = inertial->MassMatrix().Mass();
            const gz::math::Vector3d force =
                mass * m_scaling_factor * (wind - lin_vel.value());
            link.AddWorldForce(_ecm, force);
            ++links_force_applied;
            last_force = force;
            last_wind = wind;
            return true;
        });

    // Roughly once a second (world runs physics at 500 Hz per aerialWorld.world).
    if (++m_update_count % 500 == 0) {
        m_logger->debug(
            "WindRegionsPlugin::Update: {} wind-enabled link(s), {} skipped "
            "(missing pose/velocity/inertial), {} with force applied (last: "
            "wind=[{:.2f},{:.2f},{:.2f}] force=[{:.2f},{:.2f},{:.2f}]N).",
            links_seen,
            links_skipped_no_data,
            links_force_applied,
            last_wind.X(),
            last_wind.Y(),
            last_wind.Z(),
            last_force.X(),
            last_force.Y(),
            last_force.Z());
    }
}

}  // namespace lotusim::gazebo
GZ_ADD_PLUGIN(
    lotusim::gazebo::WindRegionsPlugin,
    gz::sim::System,
    lotusim::gazebo::WindRegionsPlugin::ISystemConfigure,
    lotusim::gazebo::WindRegionsPlugin::ISystemUpdate)
