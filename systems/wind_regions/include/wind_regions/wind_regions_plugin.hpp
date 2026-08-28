/*
 * Copyright (c) 2026 IRL CROSSING
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * https://www.eclipse.org/legal/epl-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0
 */
#ifndef LOTUSIM_GAZEBO_WIND_REGIONS_PLUGIN_HPP_
#define LOTUSIM_GAZEBO_WIND_REGIONS_PLUGIN_HPP_

#include <gz/math/Vector2.hh>
#include <gz/math/Vector3.hh>
#include <gz/sim/EntityComponentManager.hh>
#include <gz/sim/Link.hh>
#include <gz/sim/System.hh>
#include <gz/sim/components/Link.hh>
#include <gz/sim/components/WindMode.hh>

#include <cmath>
#include <memory>
#include <mutex>
#include <rclcpp/rclcpp.hpp>
#include <string>
#include <thread>
#include <vector>

#include "lotusim_common/common.hpp"
#include "lotusim_common/logger.hpp"
#include "lotusim_msgs/msg/wind.hpp"
#include "lotusim_msgs/msg/wind_region.hpp"
#include "lotusim_msgs/msg/wind_region_array.hpp"

namespace lotusim::gazebo {

/**
 * @brief Replacement for gz-sim-wind-effects-system that supports multiple
 * 2D wind regions on top of a single ambient (global) wind vector.
 *
 * Every link tagged wind-enabled (`<enable_wind>true</enable_wind>`, same
 * convention as stock WindEffects — see components::WindMode) is, each
 * Update(), tested against the configured regions by its world X/Y position
 * (regions have no altitude bound, matching the ambient wind which has none
 * either). Each region owns a polymorphic RegionShape (BoxShape or
 * ConeSegmentShape today — see MakeShape) — the last region in the list
 * whose shape contains the link wins; a link outside every region falls
 * back to the ambient/global vector. The resolved vector produces a force
 * `mass * scaling_factor * (wind - link_velocity)`, applied via
 * Link::AddWorldForce.
 *
 * State of the art / prior art: the force formula above (including the
 * mass multiplication, which keeps the resulting acceleration independent of
 * a link's mass) is not original — it is gz-sim's own stock WindEffects
 * system's approximation (`force_approximation_scaling_factor`, see
 * gz-sim's `src/systems/wind_effects/WindEffects.cc`, upstream
 * https://github.com/gazebosim/gz-sim), reused as-is for physical/tuning
 * consistency with the plugin it replaces. What is original here: stock
 * WindEffects only ever resolves ONE wind vector for the whole world (its
 * own region concept is a *static*, SDF-only piecewise scalar multiplier on
 * that single vector — it cannot change wind *direction* by location, and
 * cannot be reconfigured without reloading the world). This plugin instead
 * resolves an arbitrary, runtime-defined list of 2D regions — each with its
 * own independent vector — received live over ROS, plus embeds its own ROS
 * node and subscribes directly to ROS topics instead of going through a
 * gz-transport bridge:
 *  - `/aerialWorld/wind` (lotusim_msgs/Wind) — ambient/global vector.
 *  - `/aerialWorld/wind/regions` (lotusim_msgs/WindRegionArray) — region list.
 *
 * Plugin format:
 *
 * <plugin filename="wind_regions_plugin"
 * name="lotusim::gazebo::WindRegionsPlugin">
 *   <scaling_factor>1.0</scaling_factor>
 * </plugin>
 */
class WindRegionsPlugin : public gz::sim::System,
                          public gz::sim::ISystemConfigure,
                          public gz::sim::ISystemUpdate {
public:
    WindRegionsPlugin();
    ~WindRegionsPlugin() override;

    void Configure(
        const gz::sim::Entity& _entity,
        const std::shared_ptr<const sdf::Element>& _sdf,
        gz::sim::EntityComponentManager& _ecm,
        gz::sim::EventManager& _eventMgr) override;

    void Update(
        const gz::sim::UpdateInfo& _info,
        gz::sim::EntityComponentManager& _ecm) override;

private:
    /**
     * @brief One region's geometry, abstracted behind a single virtual
     * Contains() so callers (RegionState, ResolveWind) never branch on which
     * shape a region actually is — adding a new shape later means adding a
     * new subclass and one line in MakeShape(), not touching every call site
     * that used to if/else over a shape-type enum.
     */
    class RegionShape {
    public:
        virtual ~RegionShape() = default;
        virtual bool Contains(double _x, double _y, double _z) const = 0;
    };

    class BoxShape : public RegionShape {
    public:
        BoxShape(double _x1, double _y1, double _x2, double _y2)
            : m_x1(_x1), m_y1(_y1), m_x2(_x2), m_y2(_y2)
        {
        }

        /// Boxes are altitude-independent: a vertical column spanning all
        /// heights, which is the convention WindRegionBox documents and what
        /// ground-level gust patches rely on. Only cone segments have
        /// vertical extent.
        bool Contains(double _x, double _y, double) const override
        {
            return _x >= m_x1 && _x <= m_x2 && _y >= m_y1 && _y <= m_y2;
        }

    private:
        double m_x1, m_y1, m_x2, m_y2;
    };

    /**
     * @brief A tapered frustum: point-in-shape is an axis projection (is it
     * within [0, length] downstream of origin?) plus a radial bound that
     * grows linearly from r_start to r_end over that span.
     *
     * The radial bound is measured in the full plane perpendicular to the
     * axis, vertical offset included. The segment is therefore a horizontal
     * tube centred on the rotor hub, extending roughly one rotor radius above
     * and below the hub near the disk and widening downstream.
     *
     * Measuring the radius laterally only, with origin.z unused, made each
     * region unbounded in altitude: a link far above the blade tips or below
     * the turbine tested as inside the wake and received the full
     * rotor-height deficit.
     *
     * The axis remains horizontal (wind direction); the radius carries the
     * vertical extent.
     */
    class ConeSegmentShape : public RegionShape {
    public:
        ConeSegmentShape(
            const gz::math::Vector3d& _origin,
            const gz::math::Vector2d& _axis,
            double _length,
            double _rStart,
            double _rEnd)
            : m_origin(_origin), m_axis(_axis), m_length(_length),
              m_rStart(_rStart), m_rEnd(_rEnd)
        {
        }

        bool Contains(double _x, double _y, double _z) const override
        {
            double dx = _x - m_origin.X();
            double dy = _y - m_origin.Y();
            double dz = _z - m_origin.Z();
            double d = dx * m_axis.X() + dy * m_axis.Y();
            if (d < 0.0 || d > m_length) {
                return false;
            }
            double lateral = dx * -m_axis.Y() + dy * m_axis.X();
            double r = m_rStart + (m_rEnd - m_rStart) * (d / m_length);
            // Radial distance from the (horizontal) axis line, so the bound
            // applies equally to sideways and vertical offset.
            return (lateral * lateral + dz * dz) <= r * r;
        }

    private:
        gz::math::Vector3d m_origin;
        gz::math::Vector2d m_axis;
        double m_length, m_rStart, m_rEnd;
    };

    /**
     * @brief Build the RegionShape a WindRegion message describes. The only
     * place in the plugin that knows about `shape_type` — everything else
     * only ever calls RegionShape::Contains().
     */
    static std::shared_ptr<const RegionShape> MakeShape(
        const lotusim_msgs::msg::WindRegion& _region);

    struct RegionState {
        std::string id;
        // Shared (not unique) because RegionState is copied wholesale every
        // Update() tick to snapshot m_regions outside the lock (see
        // ResolveWind's doc comment below) — shapes are immutable once
        // built, so sharing them across that copy is safe and avoids
        // reconstructing geometry 500 times a second.
        std::shared_ptr<const RegionShape> shape;
        gz::math::Vector3d velocity = gz::math::Vector3d::Zero;
        bool enable_wind = false;

        bool Contains(double _x, double _y, double _z) const
        {
            return shape && shape->Contains(_x, _y, _z);
        }
    };

    /**
     * @brief Resolve the effective wind vector for a world position from a
     * snapshot of the latest ROS state: last matching region in the list
     * wins, otherwise the ambient/global vector. Returns false if the
     * resolved source has wind disabled (no force to apply). Takes the
     * snapshot by parameter (rather than reading members directly) so it can
     * be called per-link, per-tick without re-locking m_wind_mutex each time.
     */
    static bool ResolveWind(
        double _x,
        double _y,
        double _z,
        const std::vector<RegionState>& _regions,
        const gz::math::Vector3d& _globalWind,
        bool _globalEnableWind,
        gz::math::Vector3d& _wind);

    std::shared_ptr<spdlog::logger> m_logger;
    std::string m_world_name;
    double m_scaling_factor{1.0};

    rclcpp::Node::SharedPtr m_ros_node;
    rclcpp::Subscription<lotusim_msgs::msg::Wind>::SharedPtr m_wind_sub;
    rclcpp::Subscription<lotusim_msgs::msg::WindRegionArray>::SharedPtr
        m_regions_sub;
    rclcpp::executors::SingleThreadedExecutor::SharedPtr m_ros_executor;
    std::shared_ptr<std::thread> m_ros_node_thread;

    // Latest wind state received by the spin thread; read (copied) once per
    // Update() on the main sim thread. Mirrors the staging pattern used by
    // PhysicsInterfacePlugin for the same reason: the ROS callbacks run on a
    // dedicated thread, the ECM is only touched from the sim thread.
    mutable std::mutex m_wind_mutex;
    gz::math::Vector3d m_global_wind{gz::math::Vector3d::Zero};
    bool m_global_enable_wind{false};
    std::vector<RegionState> m_regions;

    // Throttles the diagnostic summary logged in Update() (see .cpp) so it
    // fires roughly once a second instead of every physics step.
    uint64_t m_update_count{0};
};

}  // namespace lotusim::gazebo
#endif
