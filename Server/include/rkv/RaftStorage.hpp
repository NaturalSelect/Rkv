#pragma once
#ifndef _RKV_RAFTSTORAGE_HPP
#define _RKV_RAFTSTORAGE_HPP

#include <utility>

#include <sharpen/LevelTable.hpp>
#include <sharpen/IpEndPoint.hpp>

#include "RaftLog.hpp"

namespace rkv
{
    class RaftStorage:public sharpen::Noncopyable
    {
    private:
        using Self = rkv::RaftStorage;
    
        std::unique_ptr<sharpen::LevelTable> table_;

        static sharpen::ByteBuffer countkey_;

        static sharpen::ByteBuffer voteKey_;

        static sharpen::ByteBuffer termKey_;

        static sharpen::ByteBuffer appiledKey_;

        static std::once_flag flag_;

        static sharpen::ByteBuffer FormatLogKey(std::uint64_t index);

        static void InitKeys();
    public:
    
        explicit RaftStorage(sharpen::EventEngine &eng,const std::string &logName)
            :table_(new sharpen::LevelTable{eng,logName,"logdb"})
        {
            using FnPtr = void(*)();
            std::call_once(Self::flag_,static_cast<FnPtr>(&Self::InitKeys));
        }
    
        RaftStorage(Self &&other) noexcept = default;
    
        inline Self &operator=(Self &&other) noexcept
        {
            if(this != std::addressof(other))
            {
                this->table_ = std::move(other.table_);
            }
            return *this;
        }
    
        ~RaftStorage() noexcept = default;

        bool EmptyLogs() const;

        bool CheckLog(std::uint64_t index,std::uint64_t term) const;

        bool ContainLog(std::uint64_t index) const;

        void AppendLog(const rkv::RaftLog &log);

        void SetVotedFor(const sharpen::IpEndPoint &id);

        void SetCurrentTerm(std::uint64_t term);

        void IncreaseCurrentTerm();

        std::uint64_t GetCurrentTerm() const;

        sharpen::IpEndPoint GetVotedFor() const;

        bool IsVotedFor() const;

        void RemoveLogsAfter(std::uint64_t beginIndex);

        rkv::RaftLog GetLog(std::uint64_t index) const;

        rkv::RaftLog GetLastLog() const;

        std::uint64_t GetLogsCount() const;

        void ResetVotedFor();

        std::uint64_t GetLastLogIndex() const;

        std::uint64_t GetLastAppiledIndex() const;

        void SetLastAppiledIndex(std::uint64_t index);
    };
}

#endif