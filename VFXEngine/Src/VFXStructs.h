#pragma once
#include "Geometry.h"

struct VFXRenderContext {
    uint32_t sceneCBVIndex;
    Frustum frustum;
    UINT currentFrameIndex;
};

struct Particle
{
    float position[3] = { 0.0f, 0.0f, 0.0f };
    float size = 1.0f;

    float velocity[3] = { 1.0f, 1.0f, 1.0f };
    float age = 9999.0f;

    float color[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    uint32_t seed = 12345;
    float custom[3] = { 0.0f, 0.0f, 0.0f };
};