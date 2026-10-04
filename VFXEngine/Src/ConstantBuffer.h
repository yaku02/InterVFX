#pragma once
#include "GpuResourceManager.h"


template <typename T>
class ConstantBuffer {
public:
    Microsoft::WRL::ComPtr<ID3D12Resource> resource = nullptr;
    uint32_t descriptorIndex = UINT32_MAX;
    T* mapData = nullptr;

    ConstantBuffer() = default;

    // --- コピー禁止 ---
    ConstantBuffer(const ConstantBuffer&) = delete;
    ConstantBuffer& operator=(const ConstantBuffer&) = delete;

    // --- ムーブ許可 ---
    ConstantBuffer(ConstantBuffer&& other) noexcept
        : resource(std::move(other.resource))
        , descriptorIndex(other.descriptorIndex)
        , mapData(other.mapData) {
        other.descriptorIndex = UINT32_MAX;
        other.mapData = nullptr; // 所有権を移動したため元のポインタをクリア
    }

    ConstantBuffer& operator=(ConstantBuffer&& other) noexcept {
        if (this != &other) {
            Unmap(); // 自身がマップ中なら解除
            resource = std::move(other.resource);
            descriptorIndex = other.descriptorIndex;
            mapData = other.mapData;

            other.descriptorIndex = UINT32_MAX;
            other.mapData = nullptr;
        }
        return *this;
    }

    ~ConstantBuffer() {
        Unmap();
    }

    HRESULT Map() {
        if (!resource) return E_POINTER;
        D3D12_RANGE readRange = { 0, 0 };
        return resource->Map(0, &readRange, reinterpret_cast<void**>(&mapData));
    }

    void Unmap() {
        if (resource && mapData) {
            resource->Unmap(0, nullptr);
            mapData = nullptr;
        }
    }

    static ConstantBuffer<T> Create(GpuResourceManager& gpuResMgr) {
        ConstantBuffer<T> cb;

        // 256バイト境界アラインメント計算
        uint32_t bufferSize = (sizeof(T) + 255) & ~255;

        // 定数バッファを作成（※void**等で受ける場合は reinterpret_cast が必要な場合があります）
        gpuResMgr.CreateConstantBuffer<T>(
            cb.resource,
            bufferSize,
            &cb.mapData,
            L"ConstantBuffer"
        );

        cb.descriptorIndex = gpuResMgr.CreateCBV(cb.resource, bufferSize);
        return cb; // ムーブコンストラクタ経由で安全に返却される
    }

    void Upload(const T& data) {
        if (mapData) {
            *mapData = data; // memcpy(mapData, &data, sizeof(T)); と同等
        }
    }
};