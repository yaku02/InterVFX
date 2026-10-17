#include "pch.h"
#include "SceneCamera.h"

using namespace DirectX;

void SceneCamera::Update(float deltaTime)
{
    // --- 元のUpdate処理（ImGuiを使ったマウス・キーボード操作） ---
    if (ImGui::IsMouseDown(ImGuiMouseButton_Right))
    {
        // --- 1. マウス移動による視界回転（Yaw / Pitch） ---
        ImVec2 mouseDelta = ImGui::GetIO().MouseDelta;
        const float mouseSensitivity = 0.003f;

        yaw += mouseDelta.x * mouseSensitivity;
        pitch += mouseDelta.y * mouseSensitivity;

        const float pitchLimit = XMConvertToRadians(89.0f);
        if (pitch > pitchLimit)  pitch = pitchLimit;
        if (pitch < -pitchLimit) pitch = -pitchLimit;

        // --- 2. WASD / QE キーによるカメラ移動 ---
        XMMATRIX rotation = XMMatrixRotationRollPitchYaw(pitch, yaw, 0.0f);

        XMVECTOR forward = XMVector3TransformCoord(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation);
        XMVECTOR right = XMVector3TransformCoord(XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f), rotation);
        XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

        XMVECTOR moveDir = XMVectorZero();

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

    // 最後に基底クラスのビュー行列更新を呼び出す
    UpdateViewMatrix();
}