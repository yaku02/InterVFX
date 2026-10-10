#include "pch.h"
#include "PreviewCamera.h"

using namespace DirectX;

void PreviewCamera::Init(float fovAngleY, float aspectRatio, float nearZ, float farZ)
{
    s_fovAngleY = fovAngleY;
    s_aspectRatio = aspectRatio;
    s_nearZ = nearZ;
    s_farZ = farZ;

    UpdateProjectionMatrix();
    UpdateViewMatrix();
}

void PreviewCamera::SetAspectRatio(float aspectRatio)
{
    s_aspectRatio = aspectRatio;
    UpdateProjectionMatrix();
}

void PreviewCamera::Update(float deltaTime)
{
    // 位置が固定のため、入力による移動や回転の処理は行いません。
    // 必要であればアニメーションや外部からのパラメータ変化に応じた UpdateViewMatrix() の呼び出し等を行えます。

    // 現在は静的なため何も処理していませんが、行列の整合性を保つため必要に応じて呼ぶことも可能です。
}

XMMATRIX PreviewCamera::GetViewMatrix()
{
    return XMLoadFloat4x4(&s_viewMatrix);
}

XMMATRIX PreviewCamera::GetProjMatrix()
{
    return XMLoadFloat4x4(&s_projMatrix);
}

XMMATRIX PreviewCamera::GetViewProjMatrix()
{
    return XMMatrixMultiply(GetViewMatrix(), GetProjMatrix());
}

XMFLOAT4X4 PreviewCamera::GetViewProjFloat4x4()
{
    XMFLOAT4X4 result;
    XMStoreFloat4x4(&result, GetViewProjMatrix());
    return result;
}

void PreviewCamera::SetPosition(const XMFLOAT3& pos)
{
    s_position = pos;
    UpdateViewMatrix();
}

void PreviewCamera::SetLookAt(const XMFLOAT3& eye, const XMFLOAT3& target, const XMFLOAT3& up)
{
    s_position = eye;

    XMVECTOR vEye = XMLoadFloat3(&eye);
    XMVECTOR vTarget = XMLoadFloat3(&target);
    XMVECTOR vUp = XMLoadFloat3(&up);

    XMMATRIX view = XMMatrixLookAtLH(vEye, vTarget, vUp);
    XMStoreFloat4x4(&s_viewMatrix, view);

    // ※もし必要であれば、ここから逆算して s_yaw / s_pitch を更新することも可能です。
}

void PreviewCamera::UpdateViewMatrix()
{
    // 従来の EditorCamera と同様に Yaw/Pitch から視線を決定する場合
    XMMATRIX rotation = XMMatrixRotationRollPitchYaw(s_pitch, s_yaw, 0.0f);
    XMVECTOR targetOffset = XMVector3TransformCoord(XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f), rotation);
    XMVECTOR eye = XMLoadFloat3(&s_position);
    XMVECTOR target = eye + targetOffset;

    XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    XMMATRIX view = XMMatrixLookAtLH(eye, target, up);
    XMStoreFloat4x4(&s_viewMatrix, view);
}

void PreviewCamera::UpdateProjectionMatrix()
{
    XMMATRIX proj = XMMatrixPerspectiveFovLH(s_fovAngleY, s_aspectRatio, s_nearZ, s_farZ);
    XMStoreFloat4x4(&s_projMatrix, proj);
}