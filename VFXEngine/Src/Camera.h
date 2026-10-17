#pragma once
#include "CameraStructs.h"

class Camera
{
public:
    struct InitDesc {
        float fovAngleY = DirectX::XM_PIDIV4;
        float aspectRatio = 16.0f / 9.0f;
        float nearZ = 0.1f;
        float farZ = 1000.0f;
    };

    static std::unique_ptr<Camera> Create(const InitDesc& desc);
    virtual ~Camera() = default;

    // 仮想関数に変更（必要に応じて pure virtual: = 0 にしてもOKです）
    virtual void Update(float deltaTime);

    void SetAspectRatio(float ratio);
    void SetPosition(const DirectX::XMFLOAT3& pos);
    void SetRotation(float newYaw, float newPitch);

    DirectX::XMMATRIX GetViewMatrix() const;
    DirectX::XMMATRIX GetProjMatrix() const;
    DirectX::XMMATRIX GetViewProjMatrix() const;
    DirectX::XMFLOAT4X4 GetViewProjFloat4x4() const;

    CameraParam GetParam() const;

protected:
    // 派生クラスから操作できるように protected にする
    Camera(const InitDesc& desc);
    void UpdateViewMatrix();
    void UpdateProjectionMatrix();

    DirectX::XMFLOAT3 position = { 0.0f, 0.0f, -5.0f };
    float yaw = 0.0f;
    float pitch = 0.0f;

    float fovAngleY;
    float aspectRatio;
    float nearZ;
    float farZ;

    DirectX::XMFLOAT4X4 viewMatrix{};
    DirectX::XMFLOAT4X4 projMatrix{};

    uint32_t index = 0;
    static uint32_t Counter;
};