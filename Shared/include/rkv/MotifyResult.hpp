#pragma once
#ifndef _RKV_MOTIFYRESULT_HPP
#define _RKV_MOTIFYRESULT_HPP

#include <cstdint>

namespace rkv
{
    enum class MotifyResult:std::uint32_t
    {
        NotCommit,
        Commited,
        Appiled,
        NotStore
    };   
}

#endif