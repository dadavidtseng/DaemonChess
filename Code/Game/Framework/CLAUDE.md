# Framework Module

[Root Directory](../../../../CLAUDE.md) > [Code](../../) > [Game](../) > **Framework**

## Module Responsibilities

The Framework module provides the foundational application architecture and controller systems for DaemonChess. It handles application lifecycle management, windowing, input processing, controller abstractions, and event-driven communication between system components.

## Entry and Startup

### Primary Entry Point
- **Main_Windows.cpp**: WinMain entry point for Windows platform
  - Creates and manages global App instance
  - Handles application startup, main loop, and shutdown sequence
  - Platform-specific windowing initialization

### Application Lifecycle
- **App.hpp/cpp**: Core application class managing the main game loop
  - Startup/Shutdown procedures
  - Frame-by-frame update and rendering coordination  
  - Event system registration and handling
  - Camera and input management integration

## External Interfaces

### Event System Integration
- **OnCloseButtonClicked**: Application termination handling
- **OnChessServerInfo**: Network server information events
- **OnChessListen**: Server listening state management
- **OnChessConnect/Disconnect**: Client connection lifecycle
- **OnRemoteCmd**: Remote command processing for multiplayer
- **OnEcho**: Network echo/ping functionality

### Input Processing
- Cursor mode management and input state handling
- Integration with PlayerController for user input processing
- Camera control and viewport management

## Key Dependencies and Configuration

### Engine Dependencies
- **Engine/Core/EngineCommon.hpp**: Core engine functionality
- **Engine/Core/EventSystem.hpp**: Event-driven architecture foundation
- **Engine/Math**: Mathematical utilities and transformations
- **Engine/Renderer**: Graphics rendering pipeline integration

### Configuration Systems  
- **Game configuration loading from XML**: `LoadGameConfig()` method
- **Engine build preferences**: Controlled via `EngineBuildPreferences.hpp`
- **Debug render system**: Conditional compilation for debugging features

## Data Models

### Controller Hierarchy
```cpp
Controller (Base Class)
├── PlayerController (Human player input)
├── AIController (AI opponent logic)
└── Future: NetworkController (Remote player proxy)
```

### Application State
- **Application lifecycle states**: Startup, Running, Shutdown
- **Game state integration**: Manages transitions between attract, lobby, match states
- **Input and camera state**: Cursor modes, camera positioning

### Event Arguments Structure
- Event-driven communication using `EventArgs` base class
- Type-safe event parameter passing
- Subscription-based event handling system

## Testing and Quality

### Debug Features
- **Console subsystem integration**: Command processing and debug output
- **Developer camera**: Independent camera for debugging and development
- **Event system debugging**: Event firing and handling verification
- **Input state visualization**: Cursor mode and input state debugging

### Error Handling
- **Safe memory management**: `GAME_SAFE_RELEASE` template for pointer cleanup
- **Event system error handling**: Graceful handling of event processing failures  
- **Platform-specific error handling**: Windows API error management

## FAQ

### Q: How is the application lifecycle managed?
A: The App class follows a strict startup → main loop → shutdown pattern. The main loop calls BeginFrame() → Update() → Render() → EndFrame() in sequence, with proper frame timing and input processing.

### Q: How does the event system work?
A: The framework uses a centralized event system where components can subscribe to events by name. Events are fired with typed arguments and handled through static callback functions.

### Q: How are different controller types handled?
A: Controllers inherit from a base Controller class and override virtual methods for Update() and input processing. The system supports human players, AI opponents, and future network players.

### Q: What's the relationship with the Game class?
A: The App class creates and owns the Game instance, delegating game-specific logic while handling platform concerns like windowing and system-level events.

## Related File List

### Core Framework Files
- `Framework/Main_Windows.cpp` - Windows platform entry point
- `Framework/App.hpp/.cpp` - Main application class
- `Framework/GameCommon.hpp/.cpp` - Global game systems and utilities  

### Controller System
- `Framework/Controller.hpp/.cpp` - Base controller abstraction
- `Framework/PlayerController.hpp/.cpp` - Human player input handling
- `Framework/AIController.hpp/.cpp` - AI opponent controller

### Common Systems
- `Framework/MatchCommon.hpp/.cpp` - Match-related data structures and utilities
- `EngineBuildPreferences.hpp` - Compile-time engine configuration

## Changelog
- **2025-09-10**: Initial module documentation - Framework architecture and controller system documented