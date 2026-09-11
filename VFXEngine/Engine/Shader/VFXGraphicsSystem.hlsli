#pragma once
#include "VFXStructs.hlsli"
#include "CommonSystem.hlsli"

/*
struct PassIndices
{
};

ConstantBuffer<PassIndices> _GetPass()
{
    return ResourceDescriptorHeap[g_PassParamCBVIdx];
}
#define g_Pass _GetPass()
*/

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
// Particle SRV
//------------------------------------------------------------
StructuredBuffer<Particle> _GetParticle()
{
    return ResourceDescriptorHeap[g_BaseInstParam.srvIndex];
}
#define g_Particles _GetParticle()