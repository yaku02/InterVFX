#pragma once

#include <memory>
#include <DirectXMath.h>
#include "CameraStructs.h"

class Camera {
public:
    static uint32_t Counter;

    struct InitDesc {
        float fovAngleY = DirectX::XMConvertToRadians(45.0f);
        float aspectRatio = 16.0f / 9.0f;
        float nearZ = 0.1f;
        float farZ = 1000.0f;
    };

    // インスタンスの生成
    static std::unique_ptr<Camera> Create(const InitDesc& desc);

    ~Camera() = default;

    // 毎フレームの入力・カメラ位置・視点更新
    void Update(float deltaTime);

    // アスペクト比の更新（画面サイズ変更時など）
    void SetAspectRatio(float aspectRatio);

    // --- 各種行列・パラメータの取得 ---
    DirectX::XMMATRIX GetViewMatrix() const;
    DirectX::XMMATRIX GetProjMatrix() const;
    DirectX::XMMATRIX GetViewProjMatrix() const;
    DirectX::XMFLOAT4X4 GetViewProjFloat4x4() const;

    // パラメータの設定・取得
    DirectX::XMFLOAT3 GetPosition() const { return position; }
    void SetPosition(const DirectX::XMFLOAT3& pos);

    void SetRotation(float yaw, float pitch);
    float GetYaw() const { return yaw; }
    float GetPitch() const { return pitch; }

    uint32_t GetID() const { return index; }

private:
    // privateコンストラクタ（Create経由で生成させる）
    Camera(const InitDesc& desc);

    void UpdateViewMatrix();
    void UpdateProjectionMatrix();

    uint32_t index = 0;

    DirectX::XMFLOAT4X4 viewMatrix{};
    DirectX::XMFLOAT4X4 projMatrix{};
    DirectX::XMFLOAT3 position = { 0.0f, 2.0f, -10.0f };

    float yaw = 0.0f;   // Y軸回転
    float pitch = 0.0f; // X軸回転

    float fovAngleY;
    float aspectRatio;
    float nearZ;
    float farZ;
};