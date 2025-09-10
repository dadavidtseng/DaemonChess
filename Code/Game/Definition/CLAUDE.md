# Definition Module

[Root Directory](../../../../CLAUDE.md) > [Code](../../) > [Game](../) > **Definition**

## Module Responsibilities

The Definition module provides data-driven configuration for game entities, primarily chess pieces and board layouts. It implements a flexible definition system that separates data from code, allowing easy modification of piece properties, visual appearance, and board configurations without code changes.

## Entry and Startup

### Definition Loading System
- **PieceDefinition::InitializeDefs()**: Loads all piece definitions from XML configuration files
- **Static definition registry**: Maintains global access to piece and board definitions
- **XML-based configuration**: Data-driven approach for easy content modification

### Initialization Sequence
1. **XML file parsing**: Definition files loaded during application startup
2. **Definition registration**: Parsed definitions stored in static registries
3. **Resource validation**: Texture and shader resource verification
4. **Mesh generation**: 3D geometry creation for each piece type

## External Interfaces

### Definition Query API
```cpp
// Primary access methods
static PieceDefinition* GetDefByName(String const& name);
static void InitializeDefs(char const* path);
static void ClearAllDefs();
```

### Resource Integration
- **Shader system**: Integration with rendering pipeline for material properties
- **Texture management**: Diffuse, normal, and specular texture assignment
- **Geometry generation**: Procedural mesh creation from definition parameters

## Key Dependencies and Configuration

### Engine Dependencies
- **Engine/Core/XmlUtils.hpp**: XML parsing and validation utilities
- **Engine/Renderer/**: Graphics resource management (textures, shaders, buffers)
- **Engine/Math/**: 3D mathematics for piece geometry and positioning

### Configuration Systems
- **XML-based definitions**: External data files for easy content modification
- **Resource path management**: Automatic texture and shader loading
- **Static definition caching**: Performance optimization through pre-loading

## Data Models

### Piece Definition Structure
```cpp
struct PieceDefinition
{
    String m_name;                          // Piece identifier
    ePieceType m_type;                      // Chess piece type (PAWN, ROOK, etc.)
    Shader* m_shader;                       // Rendering shader
    Texture* m_diffuseTexture;              // Base color texture
    Texture* m_normalTexture;               // Normal mapping for lighting
    Texture* m_specularGlossEmitTexture;    // Material properties
    std::vector<sPiecePart> m_pieceParts;   // Multi-part geometry definition
    char m_glyph;                           // ASCII representation
    VertexBuffer* m_vertexBuffer[2];        // 3D geometry data
    IndexBuffer* m_indexBuffer[2];          // Triangle indices
};
```

### Piece Type Enumeration
```cpp
enum class ePieceType : int8_t
{
    NONE = -1,
    PAWN, BISHOP, KNIGHT, ROOK, QUEEN, KING
};
```

### Multi-Part Piece System
```cpp
struct sPiecePart
{
    String m_name;              // Part identifier
    Vec3 m_startPosition;       // Initial 3D position
    Vec3 m_endPosition;         // Target position (for animations)
    EulerAngles m_orientation;  // Rotation angles
    Vec3 m_halfDimension;       // Bounding box dimensions
    float m_radius;             // Collision sphere radius
};
```

### Board Definition Structure
- **BoardDefinition.hpp/.cpp**: Chess board layout and visual configuration
- **Square properties**: Color alternation, dimensions, material properties
- **Coordinate mapping**: Translation between chess notation and 3D coordinates

## Testing and Quality

### Definition Validation
- **XML schema validation**: Ensures proper definition file structure
- **Resource existence checking**: Validates all referenced textures and shaders exist
- **Geometry validation**: Verifies piece part dimensions and positioning
- **Type consistency**: Ensures piece types match expected chess piece behaviors

### Debug Features
- **Definition reloading**: Runtime definition refresh for development
- **Resource debugging**: Visual verification of loaded textures and materials
- **Geometry visualization**: Debug rendering of piece bounding volumes
- **Definition enumeration**: Runtime listing of all loaded definitions

## FAQ

### Q: How are new piece types added to the system?
A: Add new entries to the ePieceType enum, create corresponding XML definition files, and ensure the rendering system recognizes the new type. No code changes required for basic pieces.

### Q: How does the multi-part piece system work?
A: Complex pieces can be composed of multiple geometric parts, each with independent positioning, rotation, and scaling. This allows for detailed piece designs with moving elements.

### Q: How are textures and materials managed?
A: The definition system automatically loads and manages textures based on XML configuration. The rendering system uses these textures for realistic lighting and surface appearance.

### Q: Can definitions be modified at runtime?
A: While primarily designed for load-time initialization, the system supports runtime definition clearing and reloading for development purposes.

## Related File List

### Definition Files
- `Definition/PieceDefinition.hpp/.cpp` - Chess piece data structures and loading system
- `Definition/BoardDefinition.hpp/.cpp` - Chess board layout and visual configuration

### Configuration Data
- **XML definition files**: External data files defining piece properties and appearance
- **Texture resources**: Image files for piece materials and board surfaces
- **Shader programs**: Rendering programs for piece and board visualization

## Changelog
- **2025-09-10**: Initial module documentation - Data-driven definition system and piece configuration documented