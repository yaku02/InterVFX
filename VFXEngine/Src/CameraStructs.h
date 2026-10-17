#pragma once

struct CameraParam {
    DirectX::XMFLOAT4X4 viewProj{};
    DirectX::XMFLOAT4X4 invViewProj{};
    DirectX::XMFLOAT3 position = { 0.0f, 2.0f, -10.0f };
    uint32_t index = 0;
};