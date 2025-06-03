//----------------------------------------------------------------------------------------------------
// PlayerController.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Controller.hpp"
#include "Game/Gameplay/Actor.hpp"

//----------------------------------------------------------------------------------------------------
class Camera;

//----------------------------------------------------------------------------------------------------
class PlayerController : public Controller
{
public:
    explicit PlayerController(Game* owner);
    ~PlayerController() override;

    void Update(float deltaSeconds) override;
    void Render() const;
    void UpdateFromKeyBoard();
    void UpdateFromController();

    Camera* GetCamera() const;
    Mat44   GetModelToWorldTransform() const;

private:
    Camera*     m_worldCamera     = nullptr;
    Vec3        m_velocity        = Vec3::ZERO;
    EulerAngles m_angularVelocity = EulerAngles::ZERO;
    int         m_index           = 0;
};
