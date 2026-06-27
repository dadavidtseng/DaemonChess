<a id="readme-top"></a>

<!-- PROJECT SHIELDS -->
[![Contributors][contributors-shield]][contributors-url]
[![Forks][forks-shield]][forks-url]
[![Stargazers][stars-shield]][stars-url]
[![Issues][issues-shield]][issues-url]
[![Apache 2.0 License][license-shield]][license-url]
[![LinkedIn][linkedin-shield]][linkedin-url]

<!-- PROJECT TITLE -->
<div align="center">
  <h1>Daemon Chess</h1>
  <p>A 3D chess game with procedural piece geometry, Blinn-Phong shading, and raycast-based interaction</p>
</div>

<!-- TODO: Replace with actual gameplay GIF or screenshot -->
<!-- <div align="center">
  <img src="docs/images/demo.gif" alt="Daemon Chess Gameplay" width="720">
</div> -->

<!-- TECH STACK BADGES -->
![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![DirectX](https://img.shields.io/badge/DirectX-11-107C10?style=for-the-badge&logo=xbox&logoColor=white)
![FMOD](https://img.shields.io/badge/FMOD-000000?style=for-the-badge&logo=fmod&logoColor=white)
![Windows](https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white)

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [How to Install](#how-to-install)
- [How to Use](#how-to-use)
- [Project Structure](#project-structure)
- [Future Roadmap](#future-roadmap)
- [Acknowledgements](#acknowledgements)
- [License](#license)
- [Contact](#contact)

---

## Overview

Daemon Chess is a 3D chess game where every piece is procedurally constructed from geometric primitives — spheres, cylinders, AABB3s, and OBB3s — defined in XML and rendered with Blinn-Phong shading (diffuse, normal, and specular/gloss/emit maps). Players interact with the board through a free-fly 3D camera: look at a piece or square, click to select, then click a valid destination to move. A ghost piece preview shows where the piece will land before committing.

The game implements a complete chess rule engine covering all standard moves — pawn double-step, en passant, castling (kingside and queenside), pawn promotion, path blocking, and the king adjacency rule. Captured pieces play a sinking animation before removal, and pieces smoothly interpolate to their target positions with a knight-specific hop arc.

Built on a custom [Daemon Engine](https://github.com/dadavidtseng/Engine) providing DirectX 11 rendering, FMOD audio, a developer console with V8 JavaScript integration, and a resource system with OBJ model loading.

## Features

- [Complete Chess Rule Engine](#complete-chess-rule-engine)
- [3D Procedural Piece Geometry](#3d-procedural-piece-geometry)
- [Raycast-Based Interaction](#raycast-based-interaction)
- [Animated Piece Movement & Capture](#animated-piece-movement--capture)

---

### Complete Chess Rule Engine

The Match class validates every move through a multi-step pipeline: coordinate bounds → piece existence → ownership check → zero-distance check → destination occupancy → piece-specific movement rules → path clearance → king adjacency rule → move type determination.

Supported move types:

| Move Type | Validation |
|-----------|------------|
| Normal move | Per-piece shape rules (rook: orthogonal, bishop: diagonal, etc.) |
| Capture | Destination occupied by opponent piece |
| En passant | Last move was a 2-square pawn advance to adjacent file |
| Pawn promotion | Pawn reaches 8th rank; promotes to queen, rook, bishop, or knight |
| Kingside castling | King and rook unmoved, path clear, no check traversal |
| Queenside castling | Same rules, opposite side |
| King adjacency | Kings cannot move to squares adjacent to the enemy king |

All move results are enumerated in `eMoveResult` with 20+ distinct states for precise error reporting via the developer console.

<!-- ![Rules](docs/images/feature-rules.png) -->

### 3D Procedural Piece Geometry

Each piece type is defined in `PieceDefinition.xml` as a composition of geometric primitives:

- **Pawn** — sphere cap + 3 stacked cylinders
- **Bishop** — small sphere tip + sphere head + 3 cylinders
- **Knight** — 2 oriented OBB3s (head/neck) + AABB3 body + cylinder base
- **Rook** — 2 AABB3s (battlements/tower) + AABB3 body + cylinder base
- **Queen** — 4 rotated OBB3 crown points + 3 cylinders
- **King** — sphere + 2 cross OBB3s + 3 cylinders

All pieces use Blinn-Phong shading with diffuse, normal, and specular/gloss/emit texture maps. Two mesh variants are generated per piece type (one per player side) via `CreateMeshByID`.

<!-- ![Pieces](docs/images/feature-pieces.png) -->

### Raycast-Based Interaction

Instead of a 2D cursor on a flat board, players aim a 3D camera ray to interact:

1. The camera's forward vector casts a ray into the scene
2. The ray tests against AABB3 volumes (board squares, 0.2 unit height) and CylinderZ3D volumes (pieces, 0.25 radius × 1.0 height)
3. The closest hit determines the highlighted element
4. Left-click selects; a second left-click on a valid target executes the move
5. Right-click cancels selection

When a piece is selected and the ray hits a valid destination, a ghost piece renders at the target position (slightly elevated to avoid z-fighting), giving visual feedback before committing.

<!-- ![Raycast](docs/images/feature-raycast.png) -->

### Animated Piece Movement & Capture

Pieces don't teleport — they smoothly interpolate from source to destination over 2 seconds:

- **Standard pieces** — linear position interpolation via `UpdatePositionByCoords`
- **Knights** — parabolic hop arc via `CalculateKnightHopPosition`
- **Captured pieces** — a sinking animation plays for 2 seconds (`StartCaptureAnimation`), then the piece is removed via the `PendingRemoval` queue

The delayed removal system (`SchedulePieceForRemoval` → `UpdatePendingRemovals`) ensures the capture animation completes before cleanup. If a king is captured, the game transitions to the FINISHED state.

<!-- ![Animation](docs/images/feature-animation.png) -->

---

## How to Install

### Prerequisites

- Visual Studio 2022 (or 2019) with C++ desktop development workload
- Windows 10/11 (x64)
- DirectX 11 compatible GPU
- [Daemon Engine](https://github.com/dadavidtseng/Engine) cloned as a sibling directory

### Installation

```bash
# Clone both repos side by side
git clone https://github.com/dadavidtseng/Engine.git
git clone https://github.com/dadavidtseng/DaemonChess.git

# Directory layout should be:
# ├── Engine/
# └── DaemonChess/
```

1. Open `DaemonChess.sln` in Visual Studio
2. Set configuration to `Debug | x64`
3. Build the solution (the Engine project is referenced automatically)
4. The executable is deployed to `Run/` via post-build event

## How to Use

### Controls

| Action | Key |
|--------|-----|
| Move camera | W / A / S / D / Q / E |
| Look | Mouse |
| Select piece / Execute move | Left Mouse Button |
| Cancel selection | Right Mouse Button |
| Toggle fixed / free camera | F4 |
| Fullscreen | Numpad 0 |
| Windowed | Numpad 1 |
| Cheat mode (teleport) | Ctrl (hold) |

### Debug Controls

| Action | Key |
|--------|-----|
| Developer console | ~ |
| Pause / Unpause | P |
| Step single frame | O |
| Slow-mo (0.1×) | T (hold) |
| Cycle render mode forward | F7 |
| Cycle render mode backward | F6 |
| Adjust sun direction | F2 / F3 |

### Console Commands

Moves can also be entered via the developer console:

```
ChessMove from=e2 to=e4              # Normal move
ChessMove from=e7 to=e8 promoteTo=queen  # Pawn promotion
ChessMove from=e1 to=g1              # Kingside castling
```

### Game Flow

1. **Attract Mode** — Title screen; press Space to start a match
2. **Match** — Turn-based chess; Player 0 (white) moves first, then Player 1 (black)
3. **Finished** — Triggered when a king is captured; press ESC to return to Attract

### Running

Launch `Run/DaemonChess_Debug_x64.exe` from the `Run/` directory (working directory must be `Run/` for asset loading).

## Project Structure

```
DaemonChess/
├── Code/Game/
│   ├── Definition/                    # Data-driven definitions
│   │   ├── PieceDefinition            # Piece geometry, textures, shaders (XML-loaded)
│   │   └── BoardDefinition            # Board layout, square info, piece placement
│   ├── Framework/                     # Application framework
│   │   ├── Main_Windows.cpp           # WinMain entry point
│   │   ├── App                        # Application lifecycle
│   │   ├── GameCommon                 # Global pointers, debug draw helpers
│   │   ├── Controller                 # Base controller (camera, viewport, position)
│   │   ├── PlayerController           # Free-fly camera, keyboard/mouse input
│   │   ├── AIController               # AI controller (stub)
│   │   └── MatchCommon                # Move result enums, raycast structs, piece move records
│   ├── Gameplay/                      # Core game logic
│   │   ├── Game                       # State machine (Attract → Match → Finished), player management
│   │   ├── Match                      # Chess rule engine, move validation, execution, event handling
│   │   ├── Board                      # 8×8 grid, square info, coordinate conversion, rendering
│   │   ├── Piece                      # Piece entity with animation, selection, capture states
│   │   └── Actor                      # Base class (position, orientation, color)
│   └── Subsystem/
│       ├── Console/                   # Console subsystem (stub)
│       ├── Light/                     # Directional lighting with Blinn-Phong support
│       └── Widget/                    # Widget subsystem
├── Run/                               # Runtime directory
│   ├── Data/Audio/                    # Sound effects
│   ├── Data/Definitions/              # XML config (PieceDefinition, BoardDefinition)
│   ├── Data/Fonts/                    # Bitmap fonts
│   ├── Data/Images/                   # Textures (Phong maps, marble, UV test)
│   ├── Data/Models/                   # OBJ models (Cube, Woman, TutorialBox)
│   └── Data/Shaders/                  # HLSL (BlinnPhong, Bloom, Default, Diffuse)
├── Docs/                              # Documentation
└── DaemonChess.sln                    # Visual Studio solution
```

| Module | Description |
|--------|-------------|
| Definition | XML-driven piece geometry (6 types × primitives) and board layout |
| Framework | App lifecycle, player/AI controllers, move result types |
| Gameplay | Chess rule engine (Match), board grid (Board), animated pieces (Piece) |
| Light Subsystem | Directional sun light with adjustable direction and intensity |

## Future Roadmap

- [x] 3D board and procedural piece rendering with Blinn-Phong shading
- [x] Complete move validation for all 6 piece types
- [x] En passant, castling (kingside + queenside), pawn promotion
- [x] Raycast-based piece selection with ghost piece preview
- [x] Animated piece movement with knight hop arc
- [x] Capture animation with delayed removal system
- [x] Turn-based two-player local play
- [x] Developer console with ChessMove command
- [ ] Check and checkmate detection
- [ ] AI opponent (AIController implementation)
- [ ] Online multiplayer via NetworkSubsystem
- [ ] Move history panel and undo/redo
- [ ] Stalemate, draw, and threefold repetition rules
- [ ] OBJ model pieces as alternative to procedural geometry

See the [open issues](https://github.com/dadavidtseng/DaemonChess/issues) for a full list of proposed features and known issues.

## Acknowledgements

- [Daemon Engine](https://github.com/dadavidtseng/Engine) — Custom C++ engine providing DirectX 11 rendering, FMOD audio, input, resource loading, and developer console
- SMU Guildhall — Academic environment and engine development curriculum

## License

Copyright 2025 Yu-Wei Tseng

Licensed under the [Apache License, Version 2.0](../LICENSE).

## Contact

**Yu-Wei Tseng**
- Portfolio: [dadavidtseng.info](https://dadavidtseng.info)
- GitHub: [@dadavidtseng](https://github.com/dadavidtseng)
- LinkedIn: [dadavidtseng](https://www.linkedin.com/in/dadavidtseng/)
- Email: dadavidtseng@gmail.com

Project Link: [github.com/dadavidtseng/DaemonChess](https://github.com/dadavidtseng/DaemonChess)

<p align="right">(<a href="#readme-top">back to top</a>)</p>

<!-- REFERENCE-STYLE LINKS -->
[contributors-shield]: https://img.shields.io/github/contributors/dadavidtseng/DaemonChess.svg?style=for-the-badge
[contributors-url]: https://github.com/dadavidtseng/DaemonChess/graphs/contributors
[forks-shield]: https://img.shields.io/github/forks/dadavidtseng/DaemonChess.svg?style=for-the-badge
[forks-url]: https://github.com/dadavidtseng/DaemonChess/network/members
[stars-shield]: https://img.shields.io/github/stars/dadavidtseng/DaemonChess.svg?style=for-the-badge
[stars-url]: https://github.com/dadavidtseng/DaemonChess/stargazers
[issues-shield]: https://img.shields.io/github/issues/dadavidtseng/DaemonChess.svg?style=for-the-badge
[issues-url]: https://github.com/dadavidtseng/DaemonChess/issues
[license-shield]: https://img.shields.io/github/license/dadavidtseng/DaemonChess.svg?style=for-the-badge
[license-url]: https://github.com/dadavidtseng/DaemonChess/blob/main/LICENSE
[linkedin-shield]: https://img.shields.io/badge/-LinkedIn-black.svg?style=for-the-badge&logo=linkedin&colorB=555
[linkedin-url]: https://linkedin.com/in/dadavidtseng
