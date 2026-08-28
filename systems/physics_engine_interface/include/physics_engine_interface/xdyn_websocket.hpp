/*
 * Copyright (c) 2025 Naval Group
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * https://www.eclipse.org/legal/epl-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0
 */
#ifndef LOTUSIM_XDYN_WEBSOCKET_HH_
#define LOTUSIM_XDYN_WEBSOCKET_HH_

#include <future>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <optional>
#include <string>
#include <websocketpp/client.hpp>
#include <websocketpp/common/memory.hpp>
#include <websocketpp/common/thread.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

#include "physics_engine_interface/copernicus_current.hpp"
#include "physics_engine_interface/gauss_markov_current.hpp"
#include "physics_engine_interface/physics_interface_base.hpp"

namespace lotusim::gazebo {

namespace Weblib = websocketpp::lib;
using hdl = websocketpp::connection_hdl;
using Client = websocketpp::client<websocketpp::config::asio_client>;
using json = nlohmann::json;

static const gz::math::Quaterniond
    q_ned_to_enu(0.0, 0.70710678118, 0.70710678118, 0.0);

gz::math::Quaterniond quatNedToEnu(const gz::math::Quaterniond& q_ned);
gz::math::Quaterniond quatEnuToNed(const gz::math::Quaterniond& q_enu);
gz::math::Vector3d vecNedToEnu(const gz::math::Vector3d& v_ned);
gz::math::Vector3d vecEnuToNed(const gz::math::Vector3d& v_enu);

constexpr unsigned short DEFAULT_WEBSOCKET_TIMEOUT = 5;

/// Immersion, in metres, beyond which the vessel is reported Underwater rather
/// than Surface. Overridable per vessel from the SDF (`<surface_depth>`): it is
/// a vehicle property, not a universal constant. A BlueROV working between 3 m
/// and 55 m otherwise straddles the hard-coded 10 m boundary and pays a
/// reconnection on every crossing.
///
/// There is no aerial threshold: aerial vehicles do not go through xdyn at all
/// (PhysicalEntity._lotus_blocks gives them `connection_type ROS2`), so this
/// interface only ever arbitrates between Surface and Underwater.
constexpr double DEFAULT_SURFACE_DEPTH = 10.0;

/**
 * @brief One xdyn reply, parsed but left in xdyn's own conventions.
 *
 * onMessage() only parses; every frame conversion happens in getNewState(),
 * so the NED/ENU logic lives in exactly one place and can be reasoned about
 * as a whole.
 *
 * @warning `lin_vel` and `ang_vel` are **body-frame** velocities, as xdyn
 * defines u,v,w and p,q,r. They are NOT world-frame velocities, and must be
 * rotated by the attitude before being written to Gazebo's ECM.
 */
struct XdynReply {
    double time{0.0};
    gz::math::Vector3d position;  ///< NED, world frame
    gz::math::Quaterniond attitude{1.0, 0.0, 0.0, 0.0};  ///< NED
    gz::math::Vector3d lin_vel;   ///< NED axes, BODY frame (u, v, w)
    gz::math::Vector3d ang_vel;   ///< NED axes, BODY frame (p, q, r)
    bool valid{false};
};

/**
 * @class XDynWebSocketClient
 * @brief Singleton class for connecting to XDyn through WebSocket.
 *
 * This singleton class manages communication with XDyn using WebSocket.
 * It allows reuse of the WebSocket client to send requests and receive
 * responses.
 *
 * Upon sending a request to XDyn and receiving a message, a frame conversion
 * is performed. XDyn uses the NED (North-East-Down) coordinate convention and
 * the right-hand axis convention.
 *
 * ### Request Format
 * Requests to XDyn must be made in JSON format containing the following fields:
 * ```
 * {
 *   "t":  <time>,
 *   "x":  <position_x>,
 *   "y":  <position_y>,
 *   "z":  <position_z>,
 *   "u":  <velocity_x>,
 *   "v":  <velocity_y>,
 *   "w":  <velocity_z>,
 *   "p":  <angular_rate_x>,
 *   "q":  <angular_rate_y>,
 *   "r":  <angular_rate_z>,
 *   "qr": <quaternion_r>,
 *   "qi": <quaternion_i>,
 *   "qj": <quaternion_j>,
 *   "qk": <quaternion_k>
 * }
 * ```
 *
 * ### Example Configuration
 * ```xml
 * <lotusim_param>
 *    <physics_engine_interface>
 *        <surface>
 *            <connection_type>XDynWebSocket</connection_type>
 *            <uri>ws://127.0.0.1:12345</uri>
 *            <!-- Optional. Seeds the command map with the Wageningen
 *                 B-series keys <name>(rpm)/(P/D)/(beta). Only correct for a
 *                 YAML whose force models use that convention. -->
 *            <thrusters>
 *                <thruster1>PSPropRudd</thruster1>
 *                <thruster2>SBPropRudd</thruster2>
 *            </thrusters>
 *            <!-- Optional, and mutually exclusive with <thrusters>: a raw
 *                 JSON object used verbatim as the initial command map. Use
 *                 this for any force model that is not Wageningen B-series,
 *                 e.g. `maneuvering` models taking a thrust in newtons. It
 *                 keeps the plugin agnostic of the force model, and it
 *                 guarantees the very first step already has every command
 *                 xdyn expects (a missing key fails the whole step). -->
 *            <initial_commands>
 *                {"bluerov2_heavy_prop_1(T)": 0.0}
 *            </initial_commands>
 *            <!-- Optional domain thresholds in metres, defaulting to 10.
 *                 Above <aerial_altitude> the vessel is Aerial, below
 *                 <surface_depth> it is Underwater. -->
 *            <surface_depth>60</surface_depth>
 *        </surface>
 *        <init_state>Surface</init_state>
 *    </physics_engine_interface>
 * </lotusim_param>
 * ```
 *
 * ### Example Command Map
 * The `m_vessels_cmd_map_ptr` string is formatted as:
 * ```json
 * {
 *   "PSPropRudd(P/D)": 0.79,
 *   "PSPropRudd(beta)": 0.0,
 *   "PSPropRudd(rpm)": 0.0,
 *   "SBPropRudd(P/D)": 0.79,
 *   "SBPropRudd(beta)": 0.0,
 *   "SBPropRudd(rpm)": 0.0
 * }
 * ```
 *
 * @note The NED frame convention is used throughout all communications.
 */

class XdynWebsocket : public PhysicsInterfaceBase {
public:
    XdynWebsocket();

    ~XdynWebsocket();

    void operator=(const XdynWebsocket&) = delete;

    XdynWebsocket(XdynWebsocket& other) = delete;

    static std::shared_ptr<XdynWebsocket> getInstance(
        const gz::sim::Entity& _entity,
        const std::string& _name);

    /**
     * @brief Get the New State object using given vessel state
     *
     * @param _entity
     * @param previous_state
     * @param time_dif
     * @return std::optional<json>
     */
    std::optional<std::tuple<VesselInformation, DomainType>> getNewState(
        const gz::sim::Entity& _entity,
        const VesselInformation& previous_state,
        float time_diff) override final;

    /**
     * @brief Create a Connection object
     *
     * @param _entity
     * @param uri
     * @param thrusters_name
     * @return true
     * @return false
     */
    bool createConnection(
        const gz::sim::Entity& _entity,
        const std::string& _name,
        const sdf::ElementPtr _sdf) override final;

    bool removeConnection(const gz::sim::Entity& _entity) override final;

    bool activateConnection(const gz::sim::Entity& _entity) override final;

    bool deactivateConnection(const gz::sim::Entity& _entity) override final;

    std::string getURI(const gz::sim::Entity& _entity) override final;

protected:
    static std::shared_ptr<XdynWebsocket> m_instance;

    static std::mutex m_instance_mutex;

private:
    bool send(const gz::sim::Entity& _entity, const std::string& message);

    void onMessage(
        websocketpp::connection_hdl hdl,
        websocketpp::config::asio_client::message_type::ptr msg);

    void onOpen(
        const gz::sim::Entity& _entity,
        websocketpp::connection_hdl hdl);

    void onFail(
        const gz::sim::Entity& _entity,
        websocketpp::connection_hdl hdl);

private:
    // Websocket stuff
    Client m_client;

    /**
     * @brief Thread to run client.
     *
     */
    Weblib::shared_ptr<Weblib::thread> m_thread;

    static std::mutex m_variable_mutex;

    /**
     * @brief Mapping for entity and naming
     *
     */
    static std::unordered_map<gz::sim::Entity, std::string> m_name_mapping;

    /**
     * @brief Entity mapping
     *
     */
    static std::unordered_map<std::string, gz::sim::Entity> m_entity_mapping;

    /**
     * @brief Mapping of entity to URI of the physics engine
     *
     */
    static std::unordered_map<gz::sim::Entity, std::string> m_uri;

    /**
     * @brief Connection mapping
     *
     */
    static std::unordered_map<gz::sim::Entity, Client::connection_ptr>
        m_connection_mapping;

    /**
     * @brief Connection mapping
     *
     */
    static std::unordered_map<Client::connection_ptr, gz::sim::Entity>
        m_connection_entity_mapping;

    /**
     * @brief Connection status mapping
     *
     */
    static std::unordered_map<gz::sim::Entity, std::string> m_status;

    /**
     * @brief Msg saving
     *
     */
    static std::unordered_map<gz::sim::Entity, std::mutex> m_msg_mutex;

    /**
     * @brief Thread locks
     *
     */
    static std::unordered_map<gz::sim::Entity, std::condition_variable>
        m_msg_cv;

    /**
     * @brief Last reply parsed by onMessage, per entity, in xdyn conventions.
     *
     */
    static std::unordered_map<gz::sim::Entity, XdynReply> m_saved_reply;

    /**
     * @brief Set by onMessage, cleared by send() before each request.
     *
     * Guards against the lost-wakeup race: without it, a reply arriving before
     * send() starts waiting would notify nobody and the step would burn the
     * full DEFAULT_WEBSOCKET_TIMEOUT. Distinct from XdynReply::valid, which
     * says whether the reply was usable — an error reply is received but not
     * valid.
     */
    static std::unordered_map<gz::sim::Entity, bool> m_msg_ready;

    /**
     * @brief Per-vessel Surface/Underwater threshold, in metres.
     *
     */
    static std::unordered_map<gz::sim::Entity, double> m_surface_depth;

    /**
     * @brief Per-vessel Gauss-Markov current generator, only populated for a
     * vessel whose SDF declares <gauss_markov_current>. xdyn itself has no
     * such environment model, so this current never reaches xdyn as a
     * parameter: getNewState() injects it directly into the state exchange
     * (see gauss_markov_current.hpp).
     *
     */
    static std::unordered_map<gz::sim::Entity, std::shared_ptr<GaussMarkovCurrent>>
        m_gauss_markov_current;

    /**
     * @brief Per-vessel measured-current replay, only populated for a vessel
     * whose SDF declares <copernicus_current>. Injected into the state
     * exchange by getNewState() exactly as the Gauss-Markov current is, and
     * for the same reason: xdyn has no environment model that replays a
     * measured profile (see copernicus_current.hpp).
     *
     * A vessel declares at most one of the two; they are alternative current
     * sources for the same slot, never summed.
     */
    static std::unordered_map<gz::sim::Entity, std::shared_ptr<CopernicusCurrent>>
        m_copernicus_current;

};
}  // namespace lotusim::gazebo
#endif