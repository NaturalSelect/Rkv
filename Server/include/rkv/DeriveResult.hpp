#pragma once
#ifndef _RKV_DERIVERESULT_HPP
#define _RKV_DERIVERESULT_HPP

#include <cstdint>

namespace rkv
{
    enum class DeriveResult:std::uint32_t
    {
        NotCommit,
        Commited,
        Appiled,
        LeakOfWorker
    };
}

#endif