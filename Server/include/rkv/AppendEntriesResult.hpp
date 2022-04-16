#pragma once
#ifndef _RKV_APPENDENTRIESRESULT_HPP
#define _RKV_APPENDENTRIESRESULT_HPP

#include <cstdint>

namespace rkv
{
    enum class AppendEntriesResult:std::uint32_t
    {
        NotCommit,
        Commited,
        Appiled
    };   
}

#endif