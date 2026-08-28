/*
 * Copyright (c) 2025 Naval Group
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * https://www.eclipse.org/legal/epl-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0
 */
#include "physics_engine_interface/xdyn_websocket.hpp"

namespace lotusim::gazebo {

// An attitude maps BODY vectors to WORLD vectors, so a change of convention
// takes the world basis change on the left and the body one on the right:
//
//     R_enu_flu = M_world(NED->ENU) . R_ned_frd . M_body(FRD->FLU)^-1
//
// Conjugating by q_ned_to_enu alone would apply the world map to the body
// side too, putting xdyn's body x (forward) on Gazebo body y and publishing a
// yaw 90 deg off the x-forward convention KinematicInterface uses.
// lotusim_sdk/control/frames.py inverts this and must match.
gz::math::Quaterniond quatNedToEnu(const gz::math::Quaterniond& q_ned)
{
    return q_ned_to_enu * q_ned * q_frd_to_flu.Inverse();
}

gz::math::Quaterniond quatEnuToNed(const gz::math::Quaterniond& q_enu)
{
    return q_ned_to_enu.Inverse() * q_enu * q_frd_to_flu;
}

gz::math::Vector3d vecNedToEnu(const gz::math::Vector3d& v_ned)
{
    return {v_ned.Y(), v_ned.X(), -v_ned.Z()};
}

gz::math::Vector3d vecEnuToNed(const gz::math::Vector3d& v_enu)
{
    return {v_enu.Y(), v_enu.X(), -v_enu.Z()};
}

std::shared_ptr<XdynWebsocket> XdynWebsocket::m_instance = nullptr;
std::mutex XdynWebsocket::m_instance_mutex;
std::mutex XdynWebsocket::m_variable_mutex;
std::unordered_map<gz::sim::Entity, std::string> XdynWebsocket::m_name_mapping;
std::unordered_map<std::string, gz::sim::Entity>
    XdynWebsocket::m_entity_mapping;
std::unordered_map<gz::sim::Entity, std::string> XdynWebsocket::m_uri;
std::unordered_map<gz::sim::Entity, Client::connection_ptr>
    XdynWebsocket::m_connection_mapping;
std::unordered_map<Client::connection_ptr, gz::sim::Entity>
    XdynWebsocket::m_connection_entity_mapping;
std::unordered_map<gz::sim::Entity, std::string> XdynWebsocket::m_status;
std::unordered_map<gz::sim::Entity, std::mutex> XdynWebsocket::m_msg_mutex;
std::unordered_map<gz::sim::Entity, std::condition_variable>
    XdynWebsocket::m_msg_cv;
std::unordered_map<gz::sim::Entity, XdynReply> XdynWebsocket::m_saved_reply;
std::unordered_map<gz::sim::Entity, bool> XdynWebsocket::m_msg_ready;
std::unordered_map<gz::sim::Entity, double> XdynWebsocket::m_surface_depth;
std::unordered_map<gz::sim::Entity, std::shared_ptr<GaussMarkovCurrent>>
    XdynWebsocket::m_gauss_markov_current;
std::unordered_map<gz::sim::Entity, std::shared_ptr<CopernicusCurrent>>
    XdynWebsocket::m_copernicus_current;

XdynWebsocket::XdynWebsocket() : PhysicsInterfaceBase("XdynWebsocket")
{
    m_client.clear_access_channels(websocketpp::log::alevel::all);
    m_client.clear_error_channels(websocketpp::log::elevel::all);
    m_client.init_asio();
    m_client.start_perpetual();

    m_thread = Weblib::make_shared<Weblib::thread>(&Client::run, &m_client);

    m_engine_logger->debug("t,vessel_name,X,Y,Z,U,V,W,qi,qj,qk,qr,p,q,r");
}

XdynWebsocket::~XdynWebsocket()
{
    m_client.stop_perpetual();
    websocketpp::lib::error_code ec;
    for (auto&& i : m_connection_mapping) {
        m_client
            .close(i.second, websocketpp::close::status::going_away, "", ec);
    }
    m_client.stop();
    m_thread->join();
}

std::shared_ptr<XdynWebsocket> XdynWebsocket::getInstance(
    const gz::sim::Entity& _entity,
    const std::string& _name)
{
    std::scoped_lock lock(m_instance_mutex, m_variable_mutex);
    m_name_mapping[_entity] = _name;
    m_entity_mapping[_name] = _entity;
    if (m_instance == nullptr) {
        m_instance = std::make_shared<XdynWebsocket>();
    }
    return m_instance;
}

bool XdynWebsocket::createConnection(
    const gz::sim::Entity& _entity,
    const std::string& _name,
    const sdf::ElementPtr _sdf)
{
    std::unique_lock<std::mutex> lock(m_variable_mutex);
    std::string uri;
    if (_sdf->HasElement("uri")) {
        uri = _sdf->Get<std::string>("uri");
    } else {
        m_logger->error(
            "XdynWebsocket::createConnection: No uri for {}",
            _name);
        return false;
    }
    m_uri[_entity] = uri;

    // Surface/Underwater threshold: a vehicle property, not a universal one.
    m_surface_depth[_entity] = _sdf->HasElement("surface_depth")
        ? _sdf->Get<double>("surface_depth")
        : DEFAULT_SURFACE_DEPTH;

    // Optional Gauss-Markov current, injected on the caller side (see
    // gauss_markov_current.hpp) -- absent by default, since xdyn vessels
    // normally take their current from their own YAML environment model.
    if (_sdf->HasElement("gauss_markov_current")) {
        auto gm_sdf = _sdf->GetElement("gauss_markov_current");
        const double mean_x = gm_sdf->HasElement("mean_x")
            ? gm_sdf->Get<double>("mean_x") : 0.0;
        const double mean_y = gm_sdf->HasElement("mean_y")
            ? gm_sdf->Get<double>("mean_y") : 0.0;
        const double tau = gm_sdf->HasElement("tau")
            ? gm_sdf->Get<double>("tau") : 60.0;
        // Stationary standard deviation, not the driving-noise intensity:
        // directly comparable to the mean, and its meaning does not shift
        // when tau changes. See gauss_markov_current.hpp.
        const double std_dev = gm_sdf->HasElement("std_dev")
            ? gm_sdf->Get<double>("std_dev") : 0.0;
        const unsigned int seed = gm_sdf->HasElement("seed")
            ? gm_sdf->Get<unsigned int>("seed") : 0u;
        if (gm_sdf->HasElement("sigma")) {
            m_logger->warn(
                "XdynWebsocket::createConnection: {} declares <sigma> in "
                "<gauss_markov_current>, which is not a parameter of this model. "
                "Use <std_dev>, the stationary standard deviation in m/s "
                "(std_dev = sigma * sqrt(tau/2)). Ignoring <sigma>.",
                _name);
        }
        m_gauss_markov_current[_entity] = std::make_shared<GaussMarkovCurrent>(
            mean_x, mean_y, tau, std_dev, seed);
    }

    // Optional measured-current replay from a Copernicus depth profile (see
    // copernicus_current.hpp), the same injection slot as the Gauss-Markov
    // current above and equally absent by default. Declaring both would mean
    // two currents fighting over one slot, so that is refused rather than
    // silently resolved one way or the other.
    if (_sdf->HasElement("copernicus_current")) {
        auto cop_sdf = _sdf->GetElement("copernicus_current");
        if (m_gauss_markov_current.count(_entity)) {
            m_logger->error(
                "XdynWebsocket::createConnection: {} declares both "
                "<gauss_markov_current> and <copernicus_current>. They are "
                "alternative sources for the same injected current, not "
                "additive. Ignoring <copernicus_current>.",
                _name);
        } else if (!cop_sdf->HasElement("profile")) {
            m_logger->error(
                "XdynWebsocket::createConnection: {} declares "
                "<copernicus_current> without a <profile> path. No measured "
                "current will be applied.",
                _name);
        } else {
            const std::string profile = cop_sdf->Get<std::string>("profile");
            try {
                auto cur = std::make_shared<CopernicusCurrent>(profile);
                m_logger->info(
                    "XdynWebsocket::createConnection: {} replaying measured "
                    "current from '{}' ({} depth level(s)).",
                    _name, profile, cur->levels());
                m_copernicus_current[_entity] = std::move(cur);
            } catch (const std::exception& e) {
                // Loud, and with no current installed: a run that silently
                // fell back to zero current would be indistinguishable from
                // the no-current control condition.
                m_logger->error(
                    "XdynWebsocket::createConnection: {} could not load "
                    "<copernicus_current> profile: {}. NO current will be "
                    "applied to this vessel.",
                    _name, e.what());
            }
        }
    }

    // Pre-create every per-entity slot of the maps that send() and onMessage()
    // touch, while we are still on the single-threaded load path.
    //
    // Those two run concurrently once the scenario has more than one xdyn
    // vessel: PhysicsInterfacePlugin::Update fires updateVesselState through a
    // std::async PER VESSEL, and the websocket delivers replies on its own
    // thread on top of that. Reaching a missing key through operator[] INSERTS
    // it, which can rehash the unordered_map under the other threads walking
    // it — a data race that segfaults inside send()/onMessage() rather than
    // anywhere near the real cause. Creating the entries here means the
    // concurrent paths only ever look keys up, never restructure the map.
    // They must therefore use find()/at(), never operator[].
    m_msg_mutex[_entity];
    m_msg_cv[_entity];
    m_msg_ready[_entity] = false;
    m_saved_reply[_entity] = XdynReply{};

    // Seed the command map. xdyn fails a whole step if any command declared by
    // its force models is missing, so the very first step must already carry
    // every key — an agent node cannot publish early enough to cover it.
    if (m_vessels_cmd_map_ptr) {
        if (_sdf->HasElement("initial_commands")) {
            // Force-model agnostic: whatever JSON object the model needs, used
            // verbatim. Required by anything that is not Wageningen B-series,
            // e.g. `maneuvering` models whose command is a thrust in newtons.
            const std::string raw = _sdf->Get<std::string>("initial_commands");
            try {
                auto parsed = json::parse(raw);
                if (!parsed.is_object()) {
                    throw std::runtime_error("not a JSON object");
                }
                (*m_vessels_cmd_map_ptr)[_entity] = parsed.dump();
            } catch (const std::exception& e) {
                m_logger->error(
                    "XdynWebsocket::createConnection: {} has an invalid <initial_commands>: {}\n{}",
                    _name,
                    e.what(),
                    raw);
                return false;
            }
        } else if (_sdf->HasElement("thrusters") &&
                   _sdf->GetElement("thrusters")->GetFirstElement()) {
            // Wageningen B-series convention. The emptiness check above is not
            // redundant: the SDK emits <thrusters> unconditionally, so a model
            // with no thruster (e.g. a `maneuvering` one commanded in newtons,
            // whose THRUSTERS list is empty) still has the tag, just with no
            // child. Iterating it as a do/while dereferenced that null first
            // child before testing it — a segfault on the vessel's first
            // update, inside Gazebo's own update loop.
            auto sdfPtr_thruster =
                _sdf->GetElement("thrusters")->GetFirstElement();
            auto thrusters_cmd = json::object();
            while (sdfPtr_thruster != sdf::ElementPtr(nullptr)) {
                std::string thruster_name = sdfPtr_thruster->Get<std::string>();
                thrusters_cmd[thruster_name + "(rpm)"] = 2.0;
                thrusters_cmd[thruster_name + "(P/D)"] = 0.79;
                thrusters_cmd[thruster_name + "(beta)"] = 0.0;
                sdfPtr_thruster = sdfPtr_thruster->GetNextElement();
            }
            (*m_vessels_cmd_map_ptr)[_entity] = thrusters_cmd.dump();
        } else {
            m_logger->warn(
                "XdynWebsocket::createConnection: {} declares neither <thrusters> nor "
                "<initial_commands>. Every step will fail until an agent publishes a "
                "complete command set on vessel_cmd_array.",
                _name);
        }
    }
    return true;
}

bool XdynWebsocket::removeConnection(const gz::sim::Entity& _entity)
{
    deactivateConnection(_entity);
    m_vessels_cmd_map_ptr->erase(_entity);
    m_uri.erase(_entity);
    m_saved_reply.erase(_entity);
    m_msg_ready.erase(_entity);
    m_surface_depth.erase(_entity);
    m_gauss_markov_current.erase(_entity);
    m_copernicus_current.erase(_entity);
    return true;
}

bool XdynWebsocket::activateConnection(const gz::sim::Entity& _entity)
{
    try {
        if (m_status.find(_entity) != m_status.end() &&
            (m_status[_entity] == "opened" ||
             m_status[_entity] == "configuring")) {
            m_logger->warn(
                "XdynWebsocket::activateConnection: Called for vessel entity {} even when it is active",
                _entity);
            return true;
        }

        if (m_uri.find(_entity) == m_uri.end()) {
            m_logger->error(
                "XdynWebsocket::activateConnection: Called for vessel entity {} but no uri found.",
                _entity);
            return false;
        }

        websocketpp::lib::error_code ec;
        Client::connection_ptr con =
            m_client.get_connection(m_uri[_entity], ec);
        if (ec) {
            m_logger->info(
                "XdynWebsocket::activateConnection: Connect initialization error: {}",
                ec.message());
            return false;
        }
        {
            std::unique_lock<std::mutex> lock(m_variable_mutex);
            m_connection_mapping.insert({_entity, con});
            m_connection_entity_mapping.insert({con, _entity});
            m_status.insert({_entity, "configuring"});
        }
        con->set_open_handler(bind(
            &XdynWebsocket::onOpen,
            this,
            _entity,
            websocketpp::lib::placeholders::_1));
        con->set_fail_handler(bind(
            &XdynWebsocket::onFail,
            this,
            _entity,
            websocketpp::lib::placeholders::_1));
        con->set_message_handler(bind(
            &XdynWebsocket::onMessage,
            this,
            websocketpp::lib::placeholders::_1,
            websocketpp::lib::placeholders::_2));

        // Poll for the open handler instead of sleeping a flat 3 s per try. A
        // local xdyn answers in a few milliseconds, and this call sits on
        // Gazebo's Update thread: every domain transition used to freeze the
        // whole simulation for 3 s, which is unusable for a vehicle that
        // crosses the surface/underwater boundary during a mission.
        constexpr auto kPoll = std::chrono::milliseconds(10);
        constexpr auto kAttemptTimeout = std::chrono::seconds(3);
        for (int retry = 0; retry < 3; ++retry) {
            m_logger->info(
                "XdynWebsocket::activateConnection: Starting connection: {}",
                m_name_mapping[_entity]);
            m_client.connect(con);
            const auto deadline = std::chrono::steady_clock::now() +
                kAttemptTimeout;
            while (m_status[_entity] != "opened" &&
                   m_status[_entity] != "failed" &&
                   std::chrono::steady_clock::now() < deadline) {
                std::this_thread::sleep_for(kPoll);
            }
            if (m_status[_entity] == "opened") {
                return true;
            }
        }
        m_logger->error(
            "XdynWebsocket::activateConnection: Called for vessel entity {} but unable to connect.",
            _entity);
        return false;
    } catch (const std::exception& e) {
        m_logger->error(
            "XdynWebsocket::activateConnection: Called for vessel entity {} but error\n{}.",
            _entity,
            e.what());

    } catch (...) {
        m_logger->error(
            "XdynWebsocket::activateConnection: Called for vessel entity {} but unkown error.",
            _entity);
    }
    return false;
}

bool XdynWebsocket::deactivateConnection(const gz::sim::Entity& _entity)
{
    std::unique_lock<std::mutex> lock(m_variable_mutex);
    // Same guard as send(): loadVessel calls deleteVessel (hence this) on any
    // failure path, including one where activateConnection never succeeded and
    // there is nothing to close.
    auto con_it = m_connection_mapping.find(_entity);
    if (con_it == m_connection_mapping.end() || !con_it->second) {
        m_status[_entity] = "closed";
        return true;
    }
    websocketpp::lib::error_code ec;
    m_client.close(
        con_it->second->get_handle(),
        websocketpp::close::status::going_away,
        "",
        ec);
    m_connection_entity_mapping.erase(con_it->second);
    m_connection_mapping.erase(con_it);
    m_status[_entity] = "closed";
    return true;
}

std::string XdynWebsocket::getURI(const gz::sim::Entity& _entity)
{
    std::unique_lock<std::mutex> lock(m_variable_mutex);
    if (m_uri.find(_entity) != m_uri.end()) {
        return m_uri[_entity];
    } else {
        return "";
    }
}

void XdynWebsocket::onOpen(
    const gz::sim::Entity& _entity,
    websocketpp::connection_hdl hdl)
{
    std::unique_lock<std::mutex> lock(m_variable_mutex);
    auto uri = m_client.get_con_from_hdl(hdl)->get_uri()->str();
    m_status[_entity] = "opened";
    m_logger->info("XdynWebsocket::onOpen: Opened {}", uri);
}

void XdynWebsocket::onFail(
    const gz::sim::Entity& _entity,
    websocketpp::connection_hdl hdl)
{
    std::unique_lock<std::mutex> lock(m_variable_mutex);
    auto uri = m_client.get_con_from_hdl(hdl)->get_uri()->str();
    m_status[_entity] = "failed";
    m_logger->info("XdynWebsocket::onFail: Failed {}", uri);
}

void XdynWebsocket::onMessage(
    websocketpp::connection_hdl hdl,
    websocketpp::config::asio_client::message_type::ptr msg)
{
    // Runs on the websocket thread, concurrently with one send() per vessel.
    // Every lookup here is find()-based for the same reason as in send(): an
    // operator[] miss would insert and possibly rehash under those threads.
    // The slots are pre-created in createConnection().
    auto entity_it = m_connection_entity_mapping.find(
        m_client.get_con_from_hdl(hdl));
    if (entity_it == m_connection_entity_mapping.end()) {
        m_logger->error(
            "XdynWebsocket::onMessage: reply on a connection with no entity mapped; dropping it.");
        return;
    }
    const gz::sim::Entity entity = entity_it->second;

    auto mutex_it = m_msg_mutex.find(entity);
    auto ready_it = m_msg_ready.find(entity);
    auto cv_it = m_msg_cv.find(entity);
    auto reply_it = m_saved_reply.find(entity);
    if (mutex_it == m_msg_mutex.end() || ready_it == m_msg_ready.end() ||
        cv_it == m_msg_cv.end() || reply_it == m_saved_reply.end()) {
        m_logger->error(
            "XdynWebsocket::onMessage: no message slots for entity {}; dropping the reply.",
            entity);
        return;
    }
    std::unique_lock<std::mutex> lock(mutex_it->second);

    // Parse only — no frame conversion here. All the NED/ENU logic lives in
    // getNewState(), so the conventions can be reasoned about in one place.
    XdynReply parsed;
    try {
        json reply = json::parse(msg->get_payload());

        // xdyn reports a refused step as an "error" field instead of a state.
        // Previously this threw inside the websocket callback and the vessel
        // silently kept its last pose.
        if (reply.contains("error")) {
            m_logger->error(
                "XdynWebsocket::onMessage: xdyn refused the step: {}",
                reply["error"].dump());
            reply_it->second = parsed;  // valid == false
            ready_it->second = true;
            cv_it->second.notify_one();
            return;
        }

        parsed.time = reply["t"].back().get<double>();
        parsed.position = gz::math::Vector3d(
            reply["x"].back().get<double>(),
            reply["y"].back().get<double>(),
            reply["z"].back().get<double>());
        // gz::math::Quaterniond takes (w, x, y, z); xdyn names its vector part
        // (qi, qj, qk) = (x, y, z). qj and qk were swapped here, so a pure yaw
        // returned by xdyn (qk != 0, qj == 0) reached the ECM as a pure pitch.
        parsed.attitude = gz::math::Quaterniond(
            reply["qr"].back().get<double>(),
            reply["qi"].back().get<double>(),
            reply["qj"].back().get<double>(),
            reply["qk"].back().get<double>());
        // BODY frame — see XdynReply.
        parsed.lin_vel = gz::math::Vector3d(
            reply["u"].back().get<double>(),
            reply["v"].back().get<double>(),
            reply["w"].back().get<double>());
        parsed.ang_vel = gz::math::Vector3d(
            reply["p"].back().get<double>(),
            reply["q"].back().get<double>(),
            reply["r"].back().get<double>());
        parsed.valid = true;
    } catch (const std::exception& e) {
        m_logger->error(
            "XdynWebsocket::onMessage: could not parse the reply: {}",
            e.what());
    }

    reply_it->second = std::move(parsed);
    ready_it->second = true;
    cv_it->second.notify_one();
}

std::optional<std::tuple<VesselInformation, DomainType>>
XdynWebsocket::getNewState(
    const gz::sim::Entity& _entity,
    const VesselInformation& previous_state,
    float time_diff)
{
    const double dt_s = time_diff / 1000.0;  // the caller passes milliseconds

    // --- state to send, in xdyn conventions ------------------------------
    const gz::math::Vector3d ned_position =
        vecEnuToNed(previous_state.pose.Pos());
    const gz::math::Quaterniond ned_quad =
        quatEnuToNed(previous_state.pose.Rot());

    // Plugin-side current (Gauss-Markov, or a replayed Copernicus profile --
    // see gauss_markov_current.hpp / copernicus_current.hpp), only non-zero
    // for a vessel whose SDF declared one. xdyn has an environment model for
    // neither, so it is injected here by a Galilean frame change: xdyn is sent the
    // velocity RELATIVE to this current rather than the true ground
    // velocity, computes hydrodynamic forces exactly as it would for a
    // vessel truly experiencing it, and the current is added back on the
    // reply before the result is used anywhere else. One sample per step,
    // reused identically for the outgoing and incoming conversion below, so
    // the current is treated as constant over the step.
    gz::math::Vector3d current_enu(0.0, 0.0, 0.0);
    auto gm_it = m_gauss_markov_current.find(_entity);
    if (gm_it != m_gauss_markov_current.end()) {
        const auto [vc_x, vc_y] = gm_it->second->update(dt_s);
        current_enu = vecNedToEnu(gz::math::Vector3d(vc_x, vc_y, 0.0));
    } else {
        // Measured-current replay uses the same slot (a vessel declares at
        // most one of the two, enforced in createConnection). Unlike the
        // Gauss-Markov process it is a function of DEPTH rather than of
        // elapsed time, so it is sampled at the vessel's current depth --
        // ned_position.Z(), NED and therefore positive downwards -- which is
        // what makes the replayed profile's vertical shear act on the vehicle
        // as it changes depth.
        auto cop_it = m_copernicus_current.find(_entity);
        if (cop_it != m_copernicus_current.end()) {
            const auto [vc_x, vc_y] = cop_it->second->at(ned_position.Z());
            current_enu = vecNedToEnu(gz::math::Vector3d(vc_x, vc_y, 0.0));
        }
    }

    // The ECM reports velocities in the WORLD frame; xdyn's u,v,w and p,q,r are
    // BODY frame. They used to be passed straight through, which only happens
    // to be right for a vessel heading due north — any other heading fed the
    // hydrodynamic model velocities along the wrong axes.
    //
    // A depth-resolved current (`environment models: - model: ekman current`)
    // is NOT applied here: it belongs to the vessel's own YAML, and injecting
    // a second one from the plugin would double-count it. `current_enu`
    // above is zero unless this vessel specifically requested a Gauss-Markov
    // current, which is never combined with a YAML current on the same run.
    const gz::math::Vector3d relative_lin_vel_enu =
        previous_state.lin_vel - current_enu;
    const gz::math::Vector3d body_lin_vel =
        ned_quad.RotateVectorReverse(vecEnuToNed(relative_lin_vel_enu));
    const gz::math::Vector3d body_ang_vel =
        ned_quad.RotateVectorReverse(vecEnuToNed(previous_state.ang_vel));

    json data = json::object();
    data["Dt"] = dt_s;
    data["states"] = json::array();
    json previous_state_json = {
        // Seconds, like Dt. This used to carry milliseconds, so any
        // time-driven model (irregular waves, a time-indexed set-point) saw a
        // clock running 1000x too fast.
        {"t", previous_state.time},
        {"x", ned_position.X()},
        {"y", ned_position.Y()},
        {"z", ned_position.Z()},
        {"qi", ned_quad.X()},
        {"qj", ned_quad.Y()},
        {"qk", ned_quad.Z()},
        {"qr", ned_quad.W()},
        {"u", body_lin_vel.X()},
        {"v", body_lin_vel.Y()},
        {"w", body_lin_vel.Z()},
        {"p", body_ang_vel.X()},
        {"q", body_ang_vel.Y()},
        {"r", body_ang_vel.Z()}};
    data["states"].push_back(previous_state_json);

    if (m_vessels_cmd_map_ptr->find(_entity) != m_vessels_cmd_map_ptr->end()) {
        data["commands"] = json::parse((*m_vessels_cmd_map_ptr)[_entity]);
    }

    data["requested_output"] = json::array();
    std::string msg_string = data.dump();

    if (!send(_entity, msg_string)) {
        return std::nullopt;
    }

    XdynReply reply;
    {
        std::unique_lock<std::mutex> lock(m_msg_mutex[_entity]);
        reply = m_saved_reply[_entity];
    }
    if (!reply.valid) {
        return std::nullopt;
    }

    // --- rebuild the ENU state -------------------------------------------
    const gz::math::Quaterniond gz_quad = quatNedToEnu(reply.attitude);
    // xdyn returns the velocity RELATIVE to current_enu (see above): add it
    // back here, once, so every use below (new_state.lin_vel and the
    // position integration) sees the true ground velocity. Zero for a vessel
    // with no Gauss-Markov current, so this is a no-op for the common case.
    const gz::math::Vector3d gz_lin_vel =
        vecNedToEnu(reply.attitude.RotateVector(reply.lin_vel)) + current_enu;
    const gz::math::Vector3d gz_ang_vel =
        vecNedToEnu(reply.attitude.RotateVector(reply.ang_vel));

    // Integrate the pose from the velocity rather than reading back the x,y,z
    // xdyn returns: this build advects the position by the current a second
    // time on top of the velocity state, so its x,y,z drifts at twice the
    // current speed while u,v,w stays right (measured factor 2.001 on a
    // uniform 0.5 m/s current). Trapezoidal rule, second-order over the step.
    const gz::math::Vector3d gz_position = previous_state.pose.Pos() +
        0.5 * (previous_state.lin_vel + gz_lin_vel) * dt_s;

    VesselInformation new_state;
    new_state.time = reply.time;
    new_state.entity = _entity;
    new_state.pose = gz::math::Pose3d(gz_position, gz_quad);
    new_state.lin_vel = gz_lin_vel;
    new_state.ang_vel = gz_ang_vel;

    // --- domain, from the immersion we just computed ----------------------
    // No Aerial case: aerial vehicles never reach this interface, they are
    // given connection_type ROS2 at spawn.
    const double depth = m_surface_depth.count(_entity)
        ? m_surface_depth[_entity]
        : DEFAULT_SURFACE_DEPTH;
    const DomainType domain = (gz_position.Z() <= -depth)  // ENU: positive up
        ? DomainType::Underwater
        : DomainType::Surface;
    return std::make_tuple(std::move(new_state), domain);
}

bool XdynWebsocket::send(
    const gz::sim::Entity& _entity,
    const std::string& message)
{
    // Look the per-entity slots up, never operator[]: this runs on one
    // std::async thread per vessel, and inserting here would rehash the map
    // under the others (see createConnection, where they are pre-created).
    auto mutex_it = m_msg_mutex.find(_entity);
    auto ready_it = m_msg_ready.find(_entity);
    auto cv_it = m_msg_cv.find(_entity);
    if (mutex_it == m_msg_mutex.end() || ready_it == m_msg_ready.end() ||
        cv_it == m_msg_cv.end()) {
        m_logger->error(
            "XdynWebsocket::send: no message slots for entity {} — createConnection did not run for it.",
            _entity);
        return false;
    }

    // Take the message lock BEFORE sending and clear the ready flag: a reply
    // can come back on the websocket thread faster than we reach the wait, and
    // a plain wait_for() would then miss the notification and burn the full
    // timeout. Holding the lock across the send makes onMessage queue up
    // behind us until wait_for releases it.
    std::unique_lock<std::mutex> msg_lock(mutex_it->second);
    ready_it->second = false;

    {
        std::unique_lock<std::mutex> lock(m_variable_mutex);
        // The connection is absent until activateConnection has succeeded, and
        // is erased again on deactivate; operator[] would hand back a default
        // null pointer and dereferencing it segfaults.
        auto con_it = m_connection_mapping.find(_entity);
        if (con_it == m_connection_mapping.end() || !con_it->second) {
            m_logger->error(
                "XdynWebsocket::send: no live connection for entity {}.",
                _entity);
            return false;
        }
        websocketpp::lib::error_code ec;
        m_client.send(
            con_it->second->get_handle(),
            message,
            websocketpp::frame::opcode::text,
            ec);
        if (ec) {
            m_logger->error(
                "XdynWebsocket::send: Error sending message: {}",
                ec.message());
            return false;
        }
    }

    if (!cv_it->second.wait_for(
            msg_lock,
            std::chrono::seconds(DEFAULT_WEBSOCKET_TIMEOUT),
            [&] { return ready_it->second; })) {
        m_logger->warn("XdynWebsocket::send: websocket timed out.");
        return false;
    }
    return true;
}
}  // namespace lotusim::gazebo