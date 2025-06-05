//----------------------------------------------------------------------------------------------------
// Controller.cpp
//----------------------------------------------------------------------------------------------------

//----------------------------------------------------------------------------------------------------
#include "Game/Framework/Controller.hpp"

Controller::Controller(Game* owner)
    : m_owner(owner)
{
}

//----------------------------------------------------------------------------------------------------
void Controller::SetControllerIndex(int const newIndex)
{
    m_index = newIndex;
}

void Controller::SetControllerPosition(Vec3 const& newPosition)
{
    m_position = newPosition;
}

void Controller::SetControllerOrientation(EulerAngles const& newOrientation)
{
    m_orientation = newOrientation;
}

//----------------------------------------------------------------------------------------------------
int Controller::GetControllerIndex() const
{
    return m_index;
}
