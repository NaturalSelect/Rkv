#pragma once
#ifndef _RKV_COMPLETEMIGRATIONRESULT_HPP
#define _RKV_COMPLETEMIGRATIONRESULT_HPP

#include <cstdint>

namespace rkv
{
    enum class CompleteMigrationResult:std::uint32_t
    {
        NotCommit,
        Commited,
        Appiled
    };
}

#endif