# DaemonChess - 3D Chess Simulator

![DaemonChess Screenshot](https://github.com/user-attachments/assets/8b79aafe-c6cd-4925-af62-19b2ac03692a)

## 🎮 Game Overview

DaemonChess is a sophisticated turn-based 3D chess simulator built with modern graphics technology and the custom Daemon
Engine.
This project combines traditional chess gameplay with advanced 3D visualization, featuring realistic lighting effects,
smooth animations, and future networked multiplayer capabilities.
More than just a chess game, it serves as a comprehensive showcase of modern C++ game development techniques and
advanced graphics programming.

## 🎯 Core Gameplay

* **Turn-based Chess Logic**: Complete implementation of standard chess rules with legal move validation
* **3D Interactive Board**: Intuitive mouse-controlled piece selection and movement system
* **Real-time Lighting**: Advanced Blinn-Phong shading with specular highlights and dynamic lighting
* **Smooth Animations**: Fluid piece movement and transition effects

## 🌟 Key Features

* **Advanced Graphics Rendering**: Blinn-Phong shader implementation with specular and emissive lighting
* **3D Chess Environment**: Immersive 3D board with camera controls and visual feedback
* **Custom Game Engine Integration**: Built on top of the custom Daemon Engine architecture
* **Modular Design**: Clean separation between game logic, rendering, and engine systems
* **Future Multiplayer Support**: TCP networking foundation for online chess matches
* **Cross-Platform Potential**: Designed for easy porting to different platforms

## 🛠️ Technical Stack

* **Game Engine:** Custom Daemon Engine (C++)
* **Programming Languages:** C++, HLSL
* **Graphics Pipeline:** DirectX 11, Blinn-Phong Shading Model
* **Audio Engine:** Custom audio system (planned)
* **Networking:** TCP/IP for multiplayer (in development)
* **Platform:** Windows (x64), planned Linux/macOS support

## 📁 Project Architecture

```
├── Code/
│   ├── Game/
│   │   ├── Gameplay/
│   │   │   ├── Game.cpp/hpp
│   │   │   ├── Board.cpp/hpp
│   │   │   ├── Piece.cpp/hpp
│   │   │   ├── Match.cpp/hpp
│   │   │   └── Actor.cpp/hpp
│   │   ├── Definition/
│   │   ├── Framework/
│   │   └── Subsystem/
│   └── Engine/ (Daemon Engine)
├── Docs/
│   ├── README.md
│   └── structure.puml
├── Run/
├── Temporary/
└── DaemonChess.sln
```

## 🚀 Getting Started

### Prerequisites

* **Visual Studio 2022** or later
* **Windows 10 SDK** (10.0.19041.0 or later)
* **DirectX 11** compatible graphics card
* **Git** for version control

### Installation

1. Clone the repository
   ```bash
   git clone https://github.com/dadavidtseng/DaemonChess.git
   cd DaemonChess
   ```

2. Open the Visual Studio solution
   ```bash
   start DaemonChess.sln
   ```

3. Build the project
    - Set platform to `x64`
    - Choose `Debug` or `Release` configuration
    - Press `Ctrl+Shift+B` to build

4. Run the game
    - Press `F5` to start debugging or `Ctrl+F5` to run without debugging

## 🎮 How to Play

### Controls

* **Mouse Left Click:** Select and move chess pieces
* **Mouse Drag:** Rotate camera around the board
* **Mouse Wheel:** Zoom in/out
* **ESC:** Return to main menu

### Game Modes

* **Local Play:** Two players taking turns on the same computer
* **Network Play:** Online multiplayer (coming soon)

## 📈 Development Progress

### Current Status: Alpha

### Milestones

* [x] **Phase 1:** Core engine integration and basic rendering
* [x] **Phase 2:** Chess game logic and rule implementation
* [x] **Phase 3:** 3D graphics and lighting system
* [ ] **Phase 4:** Network multiplayer implementation
* [ ] **Phase 5:** AI opponent and advanced features

### Known Issues

* **Network System:** TCP multiplayer still in development
* **Audio System:** Sound effects and music not yet implemented
* **Performance:** Minor frame rate drops with complex lighting scenarios

## 🎨 Media

### System Architecture

![UML Diagram](https://cdn-0.plantuml.com/plantuml/png/PP312i8m38RlUOgTXRw2R2fu4do5q4KiTEt8ua7KTtTBjKHt2VnV_fIFjfCWoss803xYD3NTEC83uFNDHnne1ZMV8zw9zpa9vnY9xUw4ugy_vK2ULk10bv22X9piQfUHUmiwBvaBqBF6kUVsGxLimQpTR9mhkilGf48rhjt_GMcfjCwQhRC-gfkYhDbSGakfNtxq3G00)

### Gameplay Video

*(Demo video coming soon)*

## 📊 Research Focus

This project explores several key areas in game development and computer graphics:

### Research Objectives

* **Custom Engine Development:** Building a modular, reusable game engine architecture
* **Advanced Graphics Programming:** Implementing modern shading techniques and lighting models
* **Network Game Programming:** Developing robust multiplayer systems for turn-based games

### Methodology

The development follows an iterative approach with emphasis on:

- **Modular Architecture:** Clean separation of concerns between engine and game code
- **Performance Optimization:** Efficient rendering pipelines and memory management
- **Cross-Platform Design:** Writing portable code for future platform expansions

### Findings

* **Engine Modularity:** Custom engine approach provides better control over performance and features
* **Graphics Optimization:** Blinn-Phong shading offers excellent visual quality with reasonable performance
* **Code Architecture:** Object-oriented design with component patterns scales well for complex games

## 🤝 Contributing

Contributions are welcome! This being a learning project, I'm particularly interested in:

### Development Workflow

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Make your changes with clear commit messages
4. Submit a pull request with detailed description

### Areas for Contribution

* **Graphics Enhancements:** Additional shading models, post-processing effects
* **Game Features:** AI opponents, different chess variants, replay system
* **Performance:** Optimization suggestions and profiling improvements
* **Platform Support:** Linux and macOS porting assistance

## 📄 Documentation

* [Technical Design Document](Docs/structure.puml)
* [Engine Architecture Overview](../Engine/README.md)
* [Graphics Programming Guide](Docs/graphics-guide.md) *(planned)*
* [Network Protocol Specification](Docs/network-protocol.md) *(planned)*

## 📝 License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

* **Custom Daemon Engine:** Built as part of personal engine development journey
* **DirectX Documentation:** Microsoft's comprehensive graphics programming resources
* **Chess Programming Community:** Various online resources for chess rule implementation
* **Graphics Programming Texts:** Real-Time Rendering and GPU Gems series

## 📞 Contact

For questions about this project, please contact:

* **Developer:** Yu-Wei Tseng - [dadavidtseng@gmail.com](mailto:dadavidtseng@gmail.com)
* **GitHub:** [https://github.com/dadavidtseng](https://github.com/dadavidtseng)
* **Portfolio:** [https://dadavidtseng.info](https://dadavidtseng.info)

---
**Development Period:** 2023 - Present (Active Development)  
**Last Updated:** August 7, 2025