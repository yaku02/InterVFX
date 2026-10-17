#pragma once
#include "Camera.h"

class SceneCamera : public Camera
{
public:
    using Camera::Camera; // 基底クラスのコンストラクタを引き継ぐ場合
    ~SceneCamera() override = default;

    // Updateをオーバーライドして独自の操作処理を記述
    void Update(float deltaTime) override;
};