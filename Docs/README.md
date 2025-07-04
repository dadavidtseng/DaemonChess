# ChessSimulator

<div style="width: 100%;">
  <img src="https://github.com/user-attachments/assets/8b79aafe-c6cd-4925-af62-19b2ac03692a" alt="Logo" style="width: 100%;">
</div>

## Table of Contents

* [Project Overview](#project-overview)
* [Technical Specifications](#technical-specifications)
* [Key Features](#key-features)
* [System Architecture](#system-architecture)
* [How to Build and Use](#how-to-build-and-use)
* [Development Roadmap](#development-roadmap)
* [Performance Considerations](#performance-considerations)
* [Contributing](#contributing)
* [License](#license)
* [Contact](#contact)
* [Acknowledgments](#acknowledgments)

## Project Overview

ChessSimulator is a turn-based 3D chess simulator that delivers an immersive gaming experience using modern graphics technology. This project showcases advanced 3D graphics programming techniques, including the Blinn-Phong shading model, real-time lighting effects, and upcoming networked multiplayer functionality.

More than just a fully functional chess game, this project serves as a comprehensive learning platform demonstrating modern game development techniques, covering everything from low-level graphics APIs to high-level game logic implementation.

## Technical Specifications

- **Programming Language**: C++/HLSL
- **Graphics API**: DirectX 11
- **Target Platform**: Microsoft Windows (x64)
- **Build System**: Visual Studio 2022, JetBrains Rider
- **Minimum Requirements**: Windows 10, DirectX 11 compatible graphics card

## Key Features

### Graphics Rendering

- **Blinn-Phong Shader**: Realistic lighting effects implementation
- **Specular Lighting**: Reflective lighting simulation
- **Gloss Mapping**: Surface glossiness control
- **Emissive Effects**: Self-illuminating effects

### Game Features

- **Turn-based Game Logic**: Complete chess rule implementation
- **3D Board Interface**: Intuitive 3D visual presentation
- **Piece Animations**: Smooth movement animation effects

### Technical Features

- **XML Data Loading**: Currently uses XML format for model data
- **OBJ File Support**: Planned upgrade to OBJ file format
- **TCP Networking**: Multiplayer support in development

## System Architecture

![UML](https://cdn-0.plantuml.com/plantuml/png/PP312i8m38RlUOgTXRw2R2fu4do5q4KiTEt8ua7KTtTBjKHt2VnV_fIFjfCWoss803xYD3NTEC83uFNDHnne1ZMV8zw9zpa9vnY9xUw4ugy_vK2ULk10bv22X9piQfUHUmiwBvaBqBF6kUVsGxLimQpTR9mhkilGf48rhjt_GMcfjCwQhRC-gfkYhDbSGakfNtxq3G00)

## How to Build and Use

### Build Requirements

- Visual Studio 2022 or later
- Windows 10 SDK
- DirectX 11 Runtime

### Build Instructions

1. **Clone the Repository**
   ```bash
   git clone https://github.com/dadavidtseng/ChessSimulator.git
   cd ChessSimulator
   ```

2. **Open the Project**
    - Open the `.sln` file with Visual Studio 2022
    - Ensure the project is set to x64 platform

3. **Build the Project**
    - Select `Release` or `Debug` mode
    - Press `Ctrl+Shift+B` to build the project

4. **Run the Game**
    - Ensure your system supports DirectX 11
    - Execute the generated `.exe` file

### Usage Instructions

- **Mouse Controls**: Click on pieces to select and move them
- **Camera Adjustment**: Use mouse drag to adjust viewing angle
- **Game Rules**: Follows standard chess rules

## Development Roadmap

### Completed Features

- [x] Basic 3D rendering engine
- [x] Blinn-Phong lighting system
- [x] Basic chess rules implementation
- [x] XML data loading system

### In Development

- [ ] TCP networked multiplayer
- [ ] OBJ file loader
- [ ] Advanced lighting effects
- [ ] Audio system

### Future Plans

- [ ] AI opponent functionality
- [ ] Replay system
- [ ] Custom board themes
- [ ] Tournament mode

## Performance Considerations

This project has been optimized for performance through several key improvements:

- **Batch Rendering**: Reduces the number of draw calls
- **Frustum Culling**: Only renders visible objects
- **Memory Management**: Efficient resource loading and cleanup
- **Shader Optimization**: Streamlined HLSL code

## Contributing

Contributions are welcome! Please follow these steps:

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Open a Pull Request

---

**Last Updated**: July 3, 2025
