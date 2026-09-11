// VFXComputeSystem.hlsli

#pragma once
#include "CommonSystem.hlsli"
#include "VFXStructs.hlsli"
#include "FroxelStructs.hlsli"


//------------------------------------------------------------
// BaseInst
//------------------------------------------------------------
ConstantBuffer<BaseInstParam> _GetBaseInstParam()
{
    return ResourceDescriptorHeap[g_InstCBVIdx];
}
#define g_BaseInstParam _GetBaseInstParam()

//------------------------------------------------------------
// BaseAsset
//------------------------------------------------------------
ConstantBuffer<BaseAssetParam> _GetBaseAssetParam()
{
    return ResourceDescriptorHeap[g_AssetCBVIdx];
}
#define g_BaseAssetParam _GetBaseAssetParam()


//------------------------------------------------------------
// VFX Pass 専用 CBV
//------------------------------------------------------------
struct PassIndices
{
    uint froxelSettingCBV;
    uint froxelInjectUAV;
    
    float2 padding;
};

ConstantBuffer<PassIndices> _GetPass()
{
    return ResourceDescriptorHeap[g_PassParamCBVIdx];
}
#define g_Pass _GetPass()

//------------------------------------------------------------
// Particle SRV
//------------------------------------------------------------
StructuredBuffer<Particle> _GetInject()
{
    return ResourceDescriptorHeap[g_BaseInstParam.srvIndex];
}
#define g_InjectParticles _GetInject()

//------------------------------------------------------------
// Particle UAV
//------------------------------------------------------------
RWStructuredBuffer<Particle> _GetUpdate()
{
    return ResourceDescriptorHeap[g_BaseInstParam.uavIndex];
}
#define g_UpdateParticles _GetUpdate()

//------------------------------------------------------------
// Froxel UAV
//------------------------------------------------------------
RWStructuredBuffer<FroxelData> _GetFroxel()
{
    return ResourceDescriptorHeap[g_Pass.froxelInjectUAV];
}
#define g_OutputFroxels _GetFroxel()

//------------------------------------------------------------
// FroxelSetting
//------------------------------------------------------------
ConstantBuffer<FroxelSetting> _GetFroxelSetting()
{
    return ResourceDescriptorHeap[g_Pass.froxelSettingCBV];
}
#define g_FroxelSetting _GetFroxelSetting()