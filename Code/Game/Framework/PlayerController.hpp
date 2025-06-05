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
class PlayerController final : public Controller
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
    Vec3        m_velocity        = Vec3::ZERO;
    EulerAngles m_angularVelocity = EulerAngles::ZERO;
};
