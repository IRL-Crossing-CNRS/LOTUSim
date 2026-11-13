# LOTUSim PX4 Full Flow Corrected Diagram

This diagram shows the current PX4 integration flow using the separated-world LOTUSim architecture.

Important correction: there is no separate Python `x500.py` pose bridge in the current checkout. The aerial pose is mirrored into the main world by LOTUSim core:

```text
ROS2Interface + PhysicsInterfacePlugin
```

The aerial world publishes `/aerialWorld/poses`, then the main world's physics interface subscribes to that pose stream and applies the pose to the main-world x500 entity.

## Full Flow

```mermaid
flowchart LR
    User["User launches scenario"] --> Config["Scenario JSON<br>x500 px4=true"]
    Config --> AM["AgentsManager<br>creates X500 Python ROS 2 node"]
    AM --> X500Agent["X500 Python agent node"]

    X500Agent -->|MAS CREATE_CMD| MainMAS["/energy/mas_cmd"]

    subgraph MainWorld["Gazebo main world: energy"]
        MainMAS --> AEM["AerialMAS<br>inherits MultiAgentSystem"]

        AEM -->|base EntitySpawner addEntity| MainX500["Main-world x500 entity<br>LOTUSim and Unity render object"]
        AEM -->|customUserAddEntity forwards command| AerialMASOut["forward to aerial world"]

        PhysicsPlugin["PhysicsInterfacePlugin"] -->|getNewState| ROS2Interface["ROS2Interface<br>stores latest aerial pose"]
        ROS2Interface -->|returns aerial pose| PhysicsPlugin
        PhysicsPlugin -->|SetComponentData Pose| MainX500

        MainX500 --> RenderPlugin["RenderPlugin"]
        MainX500 --> MainPosePub["MultiAgentSystem pose publishing<br>inside AerialMAS"]

        RenderPlugin -->|/energy/renderer_cmd| ROS2["ROS 2 graph"]
        RenderPlugin -->|/energy/renderer_poses| ROS2
        MainPosePub -->|/energy/poses| ROS2
    end

    AerialMASOut --> AerialMAS["/aerialWorld/mas_cmd"]

    subgraph AerialWorld["Gazebo aerial world: aerialWorld"]
        AerialMAS --> AerialEM["MultiAgentSystem"]
        AerialEM -->|addEntity cmd| RealX500["x500_px40<br>x500_px4 Gazebo model"]

        RealX500 --> MotorPlugins["MulticopterMotorModel plugins"]
        MotorPlugins --> Physics["Gazebo physics<br>gravity and 500 Hz"]
        Physics -->|updates model pose| RealX500

        RealX500 --> Sensors["IMU / barometer / GPS / magnetometer"]
        RealX500 --> AerialPosePub["MultiAgentSystem pose publishing"]
        AerialPosePub -->|/aerialWorld/poses| ROS2
    end

    ROS2 -->|/aerialWorld/poses| ROS2Interface

    X500Agent -->|launches PX4 process<br>PX4_GZ_WORLD and PX4_GZ_MODEL_NAME| PX4["PX4 SITL"]
    QGC["QGroundControl"] -->|MAVLink commands<br>arm / takeoff / manual control| PX4
    PX4 -->|MAVLink telemetry<br>state / health / position| QGC

    PX4 -->|motor_speed commands| MotorPlugins
    Sensors -->|simulated sensor data| PX4

    ROS2 --> ROSTCP["ros_tcp_endpoint"]
    ROSTCP --> Unity["Unity executable"]
    Unity --> UnityDrone["Visible x500 drone"]
```

## Main-World Detail

The main world does not load a separate normal `MultiAgentSystem` plugin. It loads `AerialMAS`, which inherits from `MultiAgentSystem`.

That means `AerialMAS` does both jobs:

- receives `/energy/mas_cmd`;
- spawns/registers the main-world x500 entity through the base `EntitySpawner::addEntity(cmd)`;
- forwards the same aerial command to `/aerialWorld/mas_cmd`.

```mermaid
flowchart LR
    X500Agent["X500 Python agent"] -->|MAS CREATE_CMD| MainMAS["/energy/mas_cmd"]

    subgraph MainWorld["energy"]
        MainMAS --> AEM["AerialMAS<br>inherits MultiAgentSystem"]

        AEM -->|base EntitySpawner addEntity| MainX500["Main-world x500 entity"]
        AEM -->|aerial hook customUserAddEntity| Forward["Forward same command"]

        MainX500 --> RenderPlugin["RenderPlugin"]
        MainX500 --> PoseTopic["/energy/poses"]
    end

    Forward --> AerialMAS["/aerialWorld/mas_cmd"]
```

## Aerial Pose Mirroring Detail

The real PX4-controlled drone moves in `aerialWorld`. Unity follows the main-world entity, so LOTUSim must copy the aerial pose back into the main world.

This is done through `ROS2Interface` and `PhysicsInterfacePlugin`, not through a Python bridge.

```mermaid
flowchart LR
    subgraph AerialWorld["aerialWorld"]
        RealX500["x500_px40 real PX4 model"] --> AerialEM["MultiAgentSystem"]
        AerialEM -->|publishes| AerialPoses["/aerialWorld/poses"]
    end

    AerialPoses --> ROS2Interface["ROS2Interface<br>stores latest aerial pose"]

    subgraph MainWorld["energy"]
        PhysicsPlugin["PhysicsInterfacePlugin"] -->|getNewState| ROS2Interface
        ROS2Interface -->|returns aerial pose| PhysicsPlugin
        PhysicsPlugin -->|SetComponentData Pose| MainX500["Main-world x500 entity"]
        MainX500 --> RenderPlugin["RenderPlugin"]
        RenderPlugin --> RendererPoses["/energy/renderer_poses"]
    end

    RendererPoses --> ROSTCP["ros_tcp_endpoint"]
    ROSTCP --> Unity["Unity executable"]
```

## Spawn Sequence

```mermaid
sequenceDiagram
    participant XA as X500 Python agent
    participant AEM as energy AerialMAS
    participant MX as Main-world x500 entity
    participant AEM2 as aerialWorld MultiAgentSystem
    participant RX as Real x500_px40 PX4 model
    participant RI as ROS2Interface
    participant RP as RenderPlugin
    participant U as Unity

    XA->>AEM: MAS CREATE_CMD on /energy/mas_cmd
    AEM->>AEM: MultiAgentSystem handleMASCmd
    AEM->>AEM: EntitySpawner addEntity
    AEM->>MX: Spawn/register main-world x500
    AEM->>AEM2: Forward same MAS command
    AEM2->>AEM2: EntitySpawner addEntity
    AEM2->>RX: Spawn real x500_px40 in aerialWorld
    RX-->>RI: Aerial pose via /aerialWorld/poses
    RI->>MX: PhysicsInterfacePlugin applies pose to main-world x500
    RP->>U: renderer_cmd creates visual object
    RP->>U: renderer_poses moves visual object
```

## Code References

Generic Scenario always launches the aerial world (`utils.AERIAL_WORLD_FILE`, `aerialWorld.world`) next to the scenario world:

```text
src/simulation_run/simulation_run/utils.py
src/simulation_run/simulation_run/simulation_runner.py
```

`X500` starts PX4 and points it to the spawned Gazebo model:

```text
src/lotusim_sdk/lotusim_sdk/agents/entity/physical/x500.py
```

Main-world `AerialMAS` receives the main MAS command and forwards aerial commands:

```text
LOTUSim/systems/aerial_demo_entity_manager/src/aerial_mas.cpp
```

`MultiAgentSystem` spawns models and publishes `/<world>/poses`:

```text
LOTUSim/systems/lotusim_mas/src/multi_agent_system.cpp
```

`ROS2Interface` subscribes to aerial poses:

```text
LOTUSim/systems/physics_engine_interface/src/ros2_interface.cpp
```

`PhysicsInterfacePlugin` applies the mirrored aerial pose to the main-world x500:

```text
LOTUSim/systems/physics_engine_interface/src/physics_interface_plugin.cpp
```

`RenderPlugin` publishes Unity render create and pose messages:

```text
LOTUSim/systems/render_interface/src/render_plugin.cpp
LOTUSim/systems/render_interface/src/ros_interface.cpp
```

## Short Explanation

The real drone is the `x500_px40` model in `aerialWorld`. PX4 controls it through Gazebo motor commands, and Gazebo physics updates its pose.

The aerial world's `MultiAgentSystem` publishes that pose on `/aerialWorld/poses`. The main world's `ROS2Interface` subscribes to that pose stream and the `PhysicsInterfacePlugin` applies the pose to the main-world x500 entity. Then `RenderPlugin` reads the main-world x500 pose and publishes `/energy/renderer_poses`, which reaches Unity through `ros_tcp_endpoint`.

So the Unity path is:

```text
aerialWorld x500_px40 moves
-> /aerialWorld/poses
-> ROS2Interface
-> PhysicsInterfacePlugin updates main-world x500
-> RenderPlugin publishes /energy/renderer_poses
-> ros_tcp_endpoint
-> Unity moves visible drone
```