//----------------------------------------------------------------------------------------------------
// Controller.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Math/AABB2.hpp"
#include "Engine/Math/EulerAngles.hpp"

class Game;
class Camera;

//----------------------------------------------------------------------------------------------------
class Controller
{
public:
    // Construction / Destruction
    explicit Controller(Game* owner);

    virtual ~Controller() = default;

    virtual void Update(float deltaSeconds) = 0;

    // Setter
    void SetControllerIndex(int newIndex);
    void SetControllerPosition(Vec3 const& newPosition);
    void SetControllerOrientation(EulerAngles const& newOrientation);
    // Getter
    int   GetControllerIndex() const;

    int   m_index = -1;
    Game* m_owner = nullptr;

    Camera* m_viewCamera = nullptr; // Handle screen message and hud
    AABB2   m_screenViewport;
    AABB2   m_viewport; // viewport size

    Camera*     m_worldCamera = nullptr; // Our camera. Used as the world camera when rendering.
    Vec3        m_position; // 3D position, separate from our actor so that we have a transform for the free-fly camera, as a Vec3, in world units.
    EulerAngles m_orientation; // 3D orientation, separate from our actor so that we have a transform for the free-fly camera, as EulerAngles, in degrees.
};
