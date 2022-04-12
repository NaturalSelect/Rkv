#pragma once
#ifndef _RKV_MOTIFYRESULT_HPP
#define _RKV_MOTIFYRESULT_HPP

namespace rkv
{
    enum class MotifyResult
    {
        NotCommit,
        Commited,
        Appiled,
        NotStore
    };   
}

#endif