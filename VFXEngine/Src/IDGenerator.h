#pragma once
#include <cstdint>

class IDGenerator
{
public:
    static constexpr uint32_t INVALID_ID = 0;

    static uint32_t Generate()
    {
        return ++s_id;
    }

private:
    inline static uint32_t s_id = 0;
};