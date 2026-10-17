#include "pch.h"
#include "Camera.h"
#include <imgui.h> // ImGuiを使用する場合

using namespace DirectX;

uint32_t Camera::Counter = 0;

std::unique_ptr<Camera> Camera::Create(const InitDesc& desc)
{
    // privateコンストラクタをunique_ptrで安全に呼び出すための記述
    return std::unique_ptr<Camera>(new Camera(desc));
}

Camera::Camera(const InitDesc& desc)
    : fovAngleY(desc.fovAngleY), aspectRatio(desc.aspectRatio), nearZ(desc.nearZ), farZ(desc.farZ)
{
    index = ++Counter;
    UpdateProjectionMatrix();
    UpdateViewMatrix();
}

void Camera::SetAspectRatio(float ratio)
{
    aspectRatio = ratio;
    UpdateProjectionMatrix();
}

void Camera::Update(float deltaTime)
{
    // 右クリック長押し中のみカメラ操作を受け付ける
    if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
    {
        // --- 1. マウス移動による視界回転（Yaw / Pitch） ---
        ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
        const float mouseSensitivity = 0.003f; // マウス感度

        yaw += mouseDelta.x * mouseSensitivity;
        pitch += mouseDelta.y * mouseSensitivity;

        // 上下回転の首振り制限（-89度〜+89度）
        const float pitchLimit = XMConvertToRadians(89.0f);
        if (pitch > pitchLimit)  pitch = pitchLimit;
        if (pitch < -pitchLimit) pitch = -pitchLimit;

        // --- 2. WASD / QE キーによるカメラ移動 ---
        XMMATRIX rotation = XMMatrixRotationRollPitchYaw(pitch, yaw, 0.0f);

        XMVECTOR forward = XMVector3TransformCoord(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation);
        XMVECTOR right = XMVector3TransformCoord(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rotation);
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        XMVECTOR moveDir = XMVectorZero();

        // Shiftキーで加速
        float moveSpeed = 10.0f;
        if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
        {
            moveSpeed *= 2.5f;
        }

        if (ImGui::IsKeyDown(ImGuiKey_W)) moveDir += forward;
        if (ImGui::IsKeyDown(ImGuiKey_S)) moveDir -= forward;
        if (ImGui::IsKeyDown(ImGuiKey_D)) moveDir += right;
        if (ImGui::IsKeyDown(ImGuiKey_A)) moveDir -= right;
        if (ImGui::IsKeyDown(ImGuiKey_E)) moveDir += up;
        if (ImGui::IsKeyDown(ImGuiKey_Q)) moveDir -= up;

        if (!XMVector3Equal(moveDir, XMVectorZero()))
        {
            moveDir = XMVector3Normalize(moveDir);

            XMVECTOR pos = XMLoadFloat3(&position);
            pos += moveDir * moveSpeed * deltaTime;
            XMStoreFloat3(&position, pos);
        }
    }

    UpdateViewMatrix();
}

XMMATRIX Camera::GetViewMatrix() const
{
    return XMLoadFloat4x4(&viewMatrix);
}

XMMATRIX Camera::GetProjMatrix() const
{
    return XMLoadFloat4x4(&projMatrix);
}

XMMATRIX Camera::GetViewProjMatrix() const
{
    return XMMatrixMultiply(GetViewMatrix(), GetProjMatrix());
}

XMFLOAT4X4 Camera::GetViewProjFloat4x4() const
{
    XMFLOAT4X4 result;
    XMStoreFloat4x4(&result, GetViewProjMatrix());
    return result;
}

void Camera::SetPosition(const XMFLOAT3& pos)
{
    position = pos;
    UpdateViewMatrix();
}

void Camera::SetRotation(float newYaw, float newPitch)
{
    yaw = newYaw;
    pitch = newPitch;
    UpdateViewMatrix();
}

void Camera::UpdateViewMatrix()
{
    XMMATRIX rotation = XMMatrixRotationRollPitchYaw(pitch, yaw, 0.0f);
    XMVECTOR targetOffset = XMVector3TransformCoord(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation);
    XMVECTOR eye = XMLoadFloat3(&position);
    XMVECTOR target = eye + targetOffset;

    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(eye, target, up);
    XMStoreFloat4x4(&viewMatrix, view);
}

void Camera::UpdateProjectionMatrix()
{
    XMMATRIX proj = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, nearZ, farZ);
    XMStoreFloat4x4(&projMatrix, proj);
}