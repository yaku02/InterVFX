#pragma once

struct AABB {
    DirectX::XMFLOAT3 min;
    DirectX::XMFLOAT3 max;

    AABB GetTransformed(const DirectX::XMMATRIX& matrix) const {
        using namespace DirectX;

        XMVECTOR corners[8] = {
            XMVectorSet(min.x, min.y, min.z, 1.0f), // 000
            XMVectorSet(min.x, min.y, max.z, 1.0f), // 001
            XMVectorSet(min.x, max.y, min.z, 1.0f), // 010
            XMVectorSet(min.x, max.y, max.z, 1.0f), // 011
            XMVectorSet(max.x, min.y, min.z, 1.0f), // 100
            XMVectorSet(max.x, min.y, max.z, 1.0f), // 101
            XMVectorSet(max.x, max.y, min.z, 1.0f), // 110
            XMVectorSet(max.x, max.y, max.z, 1.0f)  // 111
        };

        XMVECTOR minV = XMVectorSet(FLT_MAX, FLT_MAX, FLT_MAX, 0.0f);
        XMVECTOR maxV = XMVectorSet(-FLT_MAX, -FLT_MAX, -FLT_MAX, 0.0f);

        // 8頂点をすべて変換し、その中での最小・最大を再計算
        for (int i = 0; i < 8; ++i) {
            XMVECTOR transformed = XMVector3Transform(corners[i], matrix);
            minV = XMVectorMin(minV, transformed);
            maxV = XMVectorMax(maxV, transformed);
        }

        AABB result{};
        XMStoreFloat3(&result.min, minV);
        XMStoreFloat3(&result.max, maxV);
        return result;
    }
};

struct Frustum {
    DirectX::XMVECTOR planes[6]; // 左、右、下、上、近、遠
};


struct SortItem {

    float depth;

    size_t index;

};

class Geometry 
{
public:
    static bool IsVisible(const Frustum& frustum, const DirectX::XMFLOAT3& aabbMin, const DirectX::XMFLOAT3& aabbMax);
    static Frustum ExtractFrustum(const DirectX::XMMATRIX& viewProj);

    static DirectX::XMMATRIX CreateWorldMatrix(DirectX::XMFLOAT3 scale, DirectX::XMFLOAT3 rotation, DirectX::XMFLOAT3 position);
};