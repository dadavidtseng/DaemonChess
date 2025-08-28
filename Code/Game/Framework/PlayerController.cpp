//----------------------------------------------------------------------------------------------------
// PlayerController.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Framework/PlayerController.hpp"

#include "Engine/Core/Clock.hpp"
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Game/Framework/GameCommon.hpp"
#include "Game/Gameplay/Game.hpp"
#include "Game/Subsystem/Light/LightSubsystem.hpp"

//----------------------------------------------------------------------------------------------------
PlayerController::PlayerController(Game* owner)
    : Controller(owner)
{
    m_worldCamera = new Camera();
    m_worldCamera->SetPerspectiveGraphicView(2.f, 60.f, 0.1f, 100.f);
    m_worldCamera->SetNormalizedViewport(AABB2::ZERO_TO_ONE);

    Mat44 c2r;
    c2r.SetIJK3D(Vec3::Z_BASIS, -Vec3::X_BASIS, Vec3::Y_BASIS);
    m_worldCamera->SetCameraToRenderTransform(c2r);
}

//----------------------------------------------------------------------------------------------------
PlayerController::~PlayerController()
{
    GAME_SAFE_RELEASE(m_worldCamera);
}

//----------------------------------------------------------------------------------------------------
void PlayerController::Update(float deltaSeconds)
{
    if (m_owner->IsFixedCameraMode()) return;

    Vec3 forward;
    Vec3 left;
    Vec3 up;
    m_orientation.GetAsVectors_IFwd_JLeft_KUp(forward, left, up);

    m_velocity                = Vec3::ZERO;
    float constexpr moveSpeed = 2.f;

    if (g_input->IsKeyDown(KEYCODE_W)) m_velocity += forward * moveSpeed;
    if (g_input->IsKeyDown(KEYCODE_S)) m_velocity -= forward * moveSpeed;
    if (g_input->IsKeyDown(KEYCODE_A)) m_velocity += left * moveSpeed;
    if (g_input->IsKeyDown(KEYCODE_D)) m_velocity -= left * moveSpeed;
    if (g_input->IsKeyDown(KEYCODE_Z)) m_velocity -= Vec3(0.f, 0.f, 1.f) * moveSpeed;
    if (g_input->IsKeyDown(KEYCODE_C)) m_velocity += Vec3(0.f, 0.f, 1.f) * moveSpeed;

    if (g_input->IsKeyDown(KEYCODE_SHIFT)) deltaSeconds *= 10.f;

    m_position += m_velocity * deltaSeconds;

    m_orientation.m_yawDegrees -= g_input->GetCursorClientDelta().x * 0.125f;
    m_orientation.m_pitchDegrees += g_input->GetCursorClientDelta().y * 0.125f;
    m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.f, 85.f);

    m_angularVelocity.m_rollDegrees = 0.f;

    if (g_input->IsKeyDown(KEYCODE_Q)) m_angularVelocity.m_rollDegrees = 90.f;
    if (g_input->IsKeyDown(KEYCODE_E)) m_angularVelocity.m_rollDegrees = -90.f;

    m_orientation.m_rollDegrees += m_angularVelocity.m_rollDegrees * deltaSeconds;
    m_orientation.m_rollDegrees = GetClamped(m_orientation.m_rollDegrees, -45.f, 45.f);

    m_worldCamera->SetPositionAndOrientation(m_position, m_orientation);
}

//----------------------------------------------------------------------------------------------------
void PlayerController::Render() const
{
}

//----------------------------------------------------------------------------------------------------
void PlayerController::UpdateFromInput()
{
}

//----------------------------------------------------------------------------------------------------
Camera* PlayerController::GetCamera() const
{
    return m_worldCamera;
}

Mat44 PlayerController::GetModelToWorldTransform() const
{
    Mat44 m2w;

    m2w.SetTranslation3D(m_position);

    m2w.Append(m_orientation.GetAsMatrix_IFwd_JLeft_KUp());

    return m2w;
}

String PlayerController::GetName() const
{
    return m_name;
}

ePlayerType PlayerController::GetType() const
{
    return m_type;
}

//----------------------------------------------------------------------------------------------------
void PlayerController::SetName(String const& name)
{
    m_name = name;
}

//----------------------------------------------------------------------------------------------------
void PlayerController::SetType(ePlayerType const& type)
{
    m_type = type;
}
