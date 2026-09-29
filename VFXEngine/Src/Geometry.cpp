#include "pch.h"
#include "Geometry.h"

bool Geometry::IsVisible(const Frustum& frustum, const DirectX::XMFLOAT3& aabbMin, const DirectX::XMFLOAT3& aabbMax)
{
    for (int i = 0; i < 6; i++) {
        using namespace DirectX;

        // 平面の法線の向きに合わせて、最も「内側（正の方向）」にある頂点を選ぶ
        // 平面定数を w に持つベクトルを取得
        XMFLOAT4 p;
        XMStoreFloat4(&p, frustum.planes[i]);

        // 各軸で法線と同じ方向の AABB 頂点を選択
        float px = (p.x >= 0.0f) ? aabbMax.x : aabbMin.x;
        float py = (p.y >= 0.0f) ? aabbMax.y : aabbMin.y;
        float pz = (p.z >= 0.0f) ? aabbMax.z : aabbMin.z;

        // 点と平面の距離 d = ax + by + cz + w
        // これが 0 より小さければ、ボックスの最も内側の点ですら平面の外側にあることになる
        float distance = p.x * px + p.y * py + p.z * pz + p.w;

        if (distance < -1.0f) {
            return false; // 完全に外側
        }
    }
    return true; // どこかしらが視錐台に入っている
}


Frustum Geometry::ExtractFrustum(const DirectX::XMMATRIX& viewProj) {

    using namespace DirectX;

    Frustum f{};

    XMFLOAT4X4 m;
    XMStoreFloat4x4(&m, viewProj);  // ← Transposeしない

    // Left
    f.planes[0] = XMPlaneNormalize(
        XMVectorSet(m._14 + m._11, m._24 + m._21, m._34 + m._31, m._44 + m._41));

    // Right
    f.planes[1] = XMPlaneNormalize(
        XMVectorSet(m._14 - m._11, m._24 - m._21, m._34 - m._31, m._44 - m._41));

    // Bottom
    f.planes[2] = XMPlaneNormalize(
        XMVectorSet(m._14 + m._12, m._24 + m._22, m._34 + m._32, m._44 + m._42));

    // Top
    f.planes[3] = XMPlaneNormalize(
        XMVectorSet(m._14 - m._12, m._24 - m._22, m._34 - m._32, m._44 - m._42));

    // Near
    f.planes[4] = XMPlaneNormalize(
        XMVectorSet(m._14 + m._13,
            m._24 + m._23,
            m._34 + m._33,
            m._44 + m._43));

    // Far
    f.planes[5] = XMPlaneNormalize(
        XMVectorSet(m._14 - m._13,
            m._24 - m._23,
            m._34 - m._33,
            m._44 - m._43));

    return f;
}

DirectX::XMMATRIX Geometry::CreateWorldMatrix(
    DirectX::XMFLOAT3 scale, 
    DirectX::XMFLOAT3 rotation, 
    DirectX::XMFLOAT3 position)
{
    using namespace DirectX;

    XMMATRIX S = XMMatrixScaling(scale.x, scale.y, scale.z);

    // 回転
    XMMATRIX R = XMMatrixRotationRollPitchYaw(
        XMConvertToRadians(rotation.x),
        XMConvertToRadians(rotation.y),
        XMConvertToRadians(rotation.z)
    );

    // 平行移動
    XMMATRIX T = XMMatrixTranslation(position.x, position.y, position.z);

    return S * R * T;
}