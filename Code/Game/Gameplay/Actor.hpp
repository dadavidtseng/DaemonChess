//----------------------------------------------------------------------------------------------------
// Actor.hpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Math/EulerAngles.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec3.hpp"

//----------------------------------------------------------------------------------------------------
class Match;

//----------------------------------------------------------------------------------------------------
class Actor
{
public:
    explicit Actor(Match* owner);
    virtual  ~Actor() =0;


    virtual void  Update(float deltaSeconds) = 0;
    virtual void  Render() const = 0;
    virtual Mat44 GetModelToWorldTransform() const;
    virtual void  SetOrientation(EulerAngles const& newOrientation);

    Match*      m_match           = nullptr;
    IntVec2     m_coords          = IntVec2::ZERO;
    Vec3        m_position        = Vec3::ZERO;
    Vec3        m_velocity        = Vec3::ZERO;
    EulerAngles m_orientation     = EulerAngles::ZERO;
    EulerAngles m_angularVelocity = EulerAngles::ZERO;
    Rgba8       m_color           = Rgba8::WHITE;
};
