//----------------------------------------------------------------------------------------------------
// GameCommon.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Game/Subsystem/Light/LightSubsystem.hpp"

#if defined ERROR
class LightSubsystem;
#undef ERROR
#endif

#if defined min
#undef min
#endif

#if defined max
#undef max
#endif

//----------------------------------------------------------------------------------------------------
struct Rgba8;
struct Vec2;
class App;
class Game;

// one-time declaration
extern App*                   g_app;
extern Game*                  g_game;
extern LightSubsystem*       g_lightSubsystem;

//-----------------------------------------------------------------------------------------------
// DebugRender-related
//
void DebugDrawRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color);
void DebugDrawLine(Vec2 const& start, Vec2 const& end, float thickness, Rgba8 const& color);
void DebugDrawGlowCircle(Vec2 const& center, float radius, Rgba8 const& color, float glowIntensity);
void DebugDrawGlowBox(Vec2 const& center, Vec2 const& dimensions, Rgba8 const& color, float glowIntensity);
void DebugDrawBoxRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color);

//----------------------------------------------------------------------------------------------------
template <typename T>
void GAME_SAFE_RELEASE(T*& pointer)
{
    if (pointer != nullptr)
    {
        delete pointer;
        pointer = nullptr;
    }
}

char const* GetDebugIntString(int debugInt);
