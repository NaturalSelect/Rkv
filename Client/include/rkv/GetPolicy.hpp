#pragma once
#ifndef _RKV_GETPOLICY_HPP
#define _RKV_GETPOLICY_HPP

#include <cstdint>

namespace rkv
{
    enum class GetPolicy:std::uint32_t
    {
        Relaxed,
        Strict
    };   
}

#endif