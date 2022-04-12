#pragma once
#ifndef _RKV_MESSAGEHEADER_HPP
#define _RKV_MESSAGEHEADER_HPP

#include <cstdint>

namespace rkv
{
    enum class MessageType:std::uint64_t
    {
        AppendEntiresRequest,
        AppendEntiresResponse,
        VoteRequest,
        VoteResponse,
        LeaderRedirectRequest,
        LeaderRedirectResponse,
        GetRequest,
        GetResponse,
        PutRequest,
        PutResponse,
        DeleteReqeust,
        DeleteResponse,
        SetupNewRaft
    };

    struct MessageHeader
    {
        std::uint64_t size_;
        std::uint64_t type_;
    };

    inline rkv::MessageHeader MakeMessageHeader(rkv::MessageType type,std::uint64_t size) noexcept
    {
        rkv::MessageHeader header;
        header.size_ = size;
        header.type_ = static_cast<std::uint64_t>(type);
        return header;
    }

    inline rkv::MessageType GetMessageType(std::uint64_t type)
    {
        return static_cast<rkv::MessageType>(type);
    }

    inline rkv::MessageType GetMessageType(const rkv::MessageHeader &hader)
    {
        return rkv::GetMessageType(hader.type_);
    }
}

#endif