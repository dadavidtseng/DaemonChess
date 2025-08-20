//----------------------------------------------------------------------------------------------------
// PlayerController.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Game/Framework/Controller.hpp"
#include "Game/Gameplay/Actor.hpp"

//----------------------------------------------------------------------------------------------------
class Camera;

//----------------------------------------------------------------------------------------------------
enum class ePlayerType : uint8_t
{
    INVALID,
    PLAYER,
    OPPONENT,
    SPECTATOR
};

//----------------------------------------------------------------------------------------------------
class PlayerController final : public Controller
{
public:
    explicit PlayerController(Game* owner);
    ~PlayerController() override;

    void Update(float deltaSeconds) override;
    void Render() const;
    void UpdateFromInput();

    Camera*     GetCamera() const;
    Mat44       GetModelToWorldTransform() const;
    String      GetName() const;
    ePlayerType GetType() const;

    void SetName(String const& name);
    void SetType(ePlayerType const& type);

private:
    String      m_name        = "DEFAULT";
    ePlayerType m_type        = ePlayerType::INVALID;
    bool        m_isConnected = false;

    Vec3        m_velocity        = Vec3::ZERO;
    EulerAngles m_angularVelocity = EulerAngles::ZERO;
};
