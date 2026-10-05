#include "pch.h"
#include "EditorCamera.h"

using namespace DirectX;

void EditorCamera::Init(float fovAngleY, float aspectRatio, float nearZ, float farZ)
{
    s_fovAngleY = fovAngleY;
    s_aspectRatio = aspectRatio;
    s_nearZ = nearZ;
    s_farZ = farZ;

    UpdateProjectionMatrix();
    UpdateViewMatrix();
}

void EditorCamera::SetAspectRatio(float aspectRatio)
{
    s_aspectRatio = aspectRatio;
    UpdateProjectionMatrix();
}

void EditorCamera::Update(float deltaTime)
{
    // 右クリック長押し中のみカメラ操作を受け付ける
    if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
    {
        // --- 1. マウス移動による視界回転（Yaw / Pitch） ---
        ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
        const float mouseSensitivity = 0.003f; // マウス感度

        s_yaw += mouseDelta.x * mouseSensitivity;
        s_pitch += mouseDelta.y * mouseSensitivity;

        // 上下回転の首振り制限（-89度〜+89度）
        const float pitchLimit = XMConvertToRadians(89.0f);
        if (s_pitch > pitchLimit)  s_pitch = pitchLimit;
        if (s_pitch < -pitchLimit) s_pitch = -pitchLimit;

        // --- 2. WASD / QE キーによるカメラ移動 ---
        XMMATRIX rotation = XMMatrixRotationRollPitchYaw(s_pitch, s_yaw, 0.0f);

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

            XMVECTOR pos = XMLoadFloat3(&s_position);
            pos += moveDir * moveSpeed * deltaTime;
            XMStoreFloat3(&s_position, pos);
        }
    }

    UpdateViewMatrix();
}

XMMATRIX EditorCamera::GetViewMatrix()
{
    return XMLoadFloat4x4(&s_viewMatrix);
}

XMMATRIX EditorCamera::GetProjMatrix()
{
    return XMLoadFloat4x4(&s_projMatrix);
}

XMMATRIX EditorCamera::GetViewProjMatrix()
{
    return XMMatrixMultiply(GetViewMatrix(), GetProjMatrix());
}

XMFLOAT4X4 EditorCamera::GetViewProjFloat4x4()
{
    XMFLOAT4X4 result;
    XMStoreFloat4x4(&result, GetViewProjMatrix());
    return result;
}

void EditorCamera::SetPosition(const XMFLOAT3& pos)
{
    s_position = pos;
    UpdateViewMatrix();
}

void EditorCamera::UpdateViewMatrix()
{
    XMMATRIX rotation = XMMatrixRotationRollPitchYaw(s_pitch, s_yaw, 0.0f);
    XMVECTOR targetOffset = XMVector3TransformCoord(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation);
    XMVECTOR eye = XMLoadFloat3(&s_position);
    XMVECTOR target = eye + targetOffset;

    // 【修正箇所】Up ベクトルは常にワールドの Y 上方向 (0, 1, 0) に固定
    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(eye, target, up);
    XMStoreFloat4x4(&s_viewMatrix, view);
}

void EditorCamera::UpdateProjectionMatrix()
{
    XMMATRIX proj = XMMatrixPerspectiveFovLH(s_fovAngleY, s_aspectRatio, s_nearZ, s_farZ);
    XMStoreFloat4x4(&s_projMatrix, proj);
}