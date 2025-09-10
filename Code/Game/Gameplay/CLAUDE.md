# Gameplay Module  

[Root Directory](../../../../CLAUDE.md) > [Code](../../) > [Game](../) > **Gameplay**

## Module Responsibilities

The Gameplay module implements the core chess game logic and simulation systems. It manages chess matches, board state, piece behavior, move validation, and game rules enforcement. This module contains the essential algorithms and data structures that make DaemonChess function as a complete chess simulator.

## Entry and Startup

### Primary Game Controller
- **Game.hpp/.cpp**: Main game state manager and coordinator
  - Manages game state transitions (ATTRACT, LOBBY, MATCH, FINISHED, PAUSED)
  - Coordinates between UI, match logic, and player controllers
  - Handles event-driven communication for game state changes

### Match Initialization  
- **Match.hpp/.cpp**: Central match controller and chess logic processor
  - Creates and manages Board and Piece instances
  - Initializes camera systems and game clock
  - Sets up chess rule validation and move processing systems

## External Interfaces

### Event System Integration
- **OnGameStateChanged**: Game state transition notifications
- **OnChessBegin**: Match initialization and setup
- **OnChessPlayerInfo**: Player information and spectator mode updates  
- **OnChessMove**: Move execution and validation requests
- **OnEnterMatchState/Turn**: Turn-based gameplay coordination
- **OnMatchInitialized**: Match setup completion notifications

### Input Processing
- **Player input handling**: Mouse-based piece selection and movement
- **Camera control**: 3D chess board navigation and viewing angles
- **UI interaction**: Board coordinate translation and visual feedback

## Key Dependencies and Configuration

### Engine Dependencies
- **Engine/Core/Clock.hpp**: Game timing and turn management
- **Engine/Math**: 3D positioning, transformations, collision detection
- **Engine/Renderer**: 3D model rendering, lighting, and visual effects

### Game Configuration
- **Chess rule configuration**: Standard chess rules implementation
- **Board layout**: 8x8 chess board with coordinate system (A1-H8)
- **Piece definitions**: Integration with Definition module for piece properties
- **Camera setup**: 3D viewing angles and perspective configuration

## Data Models

### Core Game Entities

#### Game State Management
```cpp
enum class eGameState : uint8_t
{
    ATTRACT,    // Main menu/title screen
    LOBBY,      // Player setup and game configuration  
    MATCH,      // Active chess gameplay
    FINISHED,   // Game completion state
    PAUSED      // Temporary pause state
};
```

#### Chess Match Structure  
```cpp
class Match
{
    Board* m_board;                     // Chess board representation
    PieceList m_pieceList;              // All active chess pieces
    PieceMoveList m_pieceMoveList;      // Move history tracking
    Camera* m_screenCamera;             // 3D view camera
    Clock* m_gameClock;                 // Turn timing management
};
```

#### Board Representation
```cpp
class Board : public Actor
{
    std::vector<sSquareInfo> m_squareInfoList;  // 64 chess squares
    std::vector<AABB3> m_AABBs;                 // 3D collision boundaries
    BoardDefinition* m_definition;              // Board configuration data
};
```

### Move Validation System
```cpp
enum class eMoveResult : uint8_t
{
    VALID_MOVE_NORMAL, VALID_MOVE_PROMOTION,
    VALID_CASTLE_KINGSIDE, VALID_CASTLE_QUEENSIDE,
    VALID_CAPTURE_NORMAL, VALID_CAPTURE_ENPASSANT,
    INVALID_MOVE_* // Various invalid move conditions
};
```

## Chess Logic Implementation

### Move Validation Pipeline
1. **Input Validation**: Coordinate bounds checking and piece existence  
2. **Piece-Specific Logic**: Pawn, Rook, Bishop, Knight, Queen, King movement rules
3. **Path Obstruction**: Clear path verification for sliding pieces
4. **Special Moves**: Castling, en passant, pawn promotion handling
5. **Check Detection**: King safety validation and checkmate detection

### Special Chess Rules
- **Castling**: Kingside and queenside castle validation with path/check verification
- **En Passant**: Pawn capture validation with timing requirements
- **Pawn Promotion**: End-rank pawn transformation to Queen/Rook/Bishop/Knight
- **Check/Checkmate**: King safety enforcement and game termination detection

### Turn-Based System
- **Player alternation**: White/Black turn management with controller switching
- **Move history**: Complete game replay capability with move list tracking
- **Clock management**: Optional time controls and turn timing enforcement

## Testing and Quality

### Debug Features
- **Ghost piece visualization**: Preview piece placement during moves
- **Move validation debugging**: Visual feedback for valid/invalid moves
- **Coordinate display**: Chess notation and grid coordinate visualization
- **Cheat mode**: Development testing with rule bypass capabilities

### Chess Rule Testing
- **Move validation**: Comprehensive testing of all piece movement patterns
- **Special move testing**: Castling, en passant, and promotion scenario validation
- **Edge case handling**: Board boundary conditions and unusual game states
- **Performance profiling**: Move generation and validation performance monitoring

## FAQ

### Q: How is chess move validation implemented?
A: The system uses a multi-stage pipeline: coordinate validation → piece-specific movement rules → path obstruction checking → special move handling → check/checkmate verification.

### Q: How are 3D coordinates mapped to chess board positions?
A: The Board class provides coordinate translation between 3D world positions, integer grid coordinates (0-7), and standard chess notation (A1-H8).

### Q: How does the turn-based system work?
A: The Game class manages player controllers and alternates between them. Each turn allows one move, with validation ensuring only the current player can move their pieces.

### Q: How are special chess moves like castling handled?
A: Special moves have dedicated validation functions that check prerequisites (piece movement history, path clearance, check status) before executing the multi-piece movement.

## Related File List

### Core Gameplay
- `Gameplay/Game.hpp/.cpp` - Main game state and coordination
- `Gameplay/Match.hpp/.cpp` - Chess match logic and move processing
- `Gameplay/Board.hpp/.cpp` - Chess board representation and coordinate systems
- `Gameplay/Piece.hpp/.cpp` - Individual chess piece behavior and rendering
- `Gameplay/Actor.hpp/.cpp` - Base class for game entities with 3D properties

## Changelog
- **2025-09-10**: Initial module documentation - Core chess gameplay logic and match system documented