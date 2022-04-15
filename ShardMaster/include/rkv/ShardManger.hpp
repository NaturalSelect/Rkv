#pragma once
#ifndef _RKV_SHARDMANAGER_HPP
#define _RKV_SHARDMANAGER_HPP

#include <rkv/Shard.hpp>
#include <rkv/RaftLog.hpp>
#include <rkv/KeyValueService.hpp>

namespace rkv
{
    class ShardManger
    {
    private:
        using Self = rkv::ShardManger;
    
        static sharpen::ByteBuffer countKey_;

        static std::once_flag flag_;

        static void InitKeys();

        static sharpen::ByteBuffer FormatShardKey(std::uint64_t id);

        static bool CompareShards(const rkv::Shard &left,const rkv::Shard &right) noexcept;

        static bool CompareShards(const rkv::Shard &left,const sharpen::ByteBuffer &right) noexcept;

        rkv::KeyValueService *service_;
        std::vector<rkv::Shard> shards_;
    public:
    
        ShardManger(rkv::KeyValueService &service);
    
        ShardManger(const Self &other) = default;
    
        ShardManger(Self &&other) noexcept = default;
    
        inline Self &operator=(const Self &other)
        {
            Self tmp{other};
            std::swap(tmp,*this);
            return *this;
        }
    
        inline Self &operator=(Self &&other) noexcept
        {
            if(this != std::addressof(other))
            {
                this->service_ = other.service_;
                this->shards_ = std::move(other.shards_);
            }
            return *this;
        }
    
        ~ShardManger() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        template<typename _InsertIterator,typename _Iterator,typename _Check = decltype(*std::declval<_InsertIterator&>()++ = std::declval<rkv::RaftLog&&>(),std::declval<const rkv::Shard&>() = *std::declval<_Iterator&>())>
        std::uint64_t GenrateEmplaceLogs(_InsertIterator inserter,_Iterator begin,_Iterator end,std::uint64_t beginIndex,std::uint64_t term) const
        {
            std::uint64_t size{sharpen::GetRangeSize(begin,end)};
            if(size)
            {
                
                std::uint64_t oldSize{this->shards_.size()};
                while (begin != end)
                {
                    const rkv::Shard &shard{*begin};
                    rkv::RaftLog log;
                    log.SetOperation(rkv::RaftLog::Operation::Put);
                    log.SetIndex(beginIndex++);
                    log.SetTerm(term);
                    log.Key() = Self::FormatShardKey(shard.GetId());
                    sharpen::ByteBuffer buf;
                    shard.Serialize().StoreTo(buf);
                    log.Value() = std::move(buf);
                    *inserter++ = std::move(log);
                }
                rkv::RaftLog log;
                log.SetOperation(rkv::RaftLog::Operation::Put);
                log.SetIndex(beginIndex);
                log.SetTerm(term);
                log.Key() = Self::countKey_;
                sharpen::ByteBuffer buf{sizeof(size)};
                buf.As<std::uint64_t>() = size + oldSize;
                log.Value() = std::move(buf);
                *inserter++ = std::move(log);
            }
            return beginIndex;
        }

        void Flush();

        const rkv::Shard *GetShardPtr(const sharpen::ByteBuffer &key) const noexcept;

        template<typename _InsertIterator,typename _Check = decltype(*std::declval<_InsertIterator&>()++ = std::declval<const rkv::Shard&>())>
        void GetShards(_InsertIterator inserter,const sharpen::IpEndPoint &id) const noexcept
        {
            for (auto begin = this->shards_.begin(),end = this->shards_.end(); begin != end; ++begin)
            {
                *inserter++ = *begin;   
            }
        }

        const rkv::Shard *FindShardPtr(const sharpen::ByteBuffer &key) const noexcept;

        inline std::size_t GetSize() const noexcept
        {
            return this->shards_.size();
        }

        inline std::uint64_t GetNextIndex() const noexcept
        {
            return this->GetSize();
        }

        inline bool Empty() const noexcept
        {
            return this->shards_.empty();
        }
    };
} 

#endif