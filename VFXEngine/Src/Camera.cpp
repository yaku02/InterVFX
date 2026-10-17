#include "pch.h"
#include "Camera.h"

using namespace DirectX;
uint32_t Camera::Counter = 0;

std::unique_ptr<Camera> Camera::Create(const InitDesc& desc)
{
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

// 基底クラス側では何もしない、または基本処理のみにする
void Camera::Update(float deltaTime)
{
    // 必要であればここで共通の更新処理を行う
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

CameraParam Camera::GetParam() const
{
    CameraParam result;

    DirectX::XMMATRIX viewProj = GetViewProjMatrix();
    DirectX::XMVECTOR det;
    DirectX::XMMATRIX invViewProj = DirectX::XMMatrixInverse(&det, viewProj);

    DirectX::XMStoreFloat4x4(&result.viewProj, DirectX::XMMatrixTranspose(viewProj));
    DirectX::XMStoreFloat4x4(&result.invViewProj, DirectX::XMMatrixTranspose(invViewProj));
    result.position = position;
    result.index = index;

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