// //----------------------------------------------------------------------------------------------------
// // Player.hpp
// //----------------------------------------------------------------------------------------------------
//
// //----------------------------------------------------------------------------------------------------
// #pragma once
// #include "Game/Actor.hpp"
//
// //----------------------------------------------------------------------------------------------------
// class Camera;
//
// //----------------------------------------------------------------------------------------------------
// class Player : public Actor
// {
// public:
//     explicit Player(Game* owner);
//     ~Player() override;
//
//     void Update(float deltaSeconds) override;
//     void Render() const override;
//     void UpdateFromKeyBoard();
//     void UpdateFromController();
//
//     Camera* GetCamera() const;
//
// private:
//     Camera* m_worldCamera = nullptr;
// };
