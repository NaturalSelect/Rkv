#pragma once
#ifndef _RKV_MIGRATIONMANGER_HPP
#define _RKV_MIGRATIONMANGER_HPP

#include <rkv/Migration.hpp>
#include <rkv/RaftLog.hpp>
#include <rkv/KeyValueService.hpp>

namespace rkv
{
    class MigrationManger
    {
    private:
        using Self = rkv::MigrationManger;
        using IndexType = std::vector<std::uint64_t>;
        
        static sharpen::ByteBuffer indexKey_;

        static std::once_flag flag_;

        static void InitKeys();

        static sharpen::ByteBuffer FormatMigrationKey(std::uint64_t id);

        IndexType GenrateIndex() const;

        const rkv::KeyValueService *service_;
        std::vector<rkv::Migration> migrations_;
        std::uint64_t nextGroupId_;
        std::uint64_t nextMigrationId_;
    public:
    
        explicit MigrationManger(const rkv::KeyValueService &service);
    
        MigrationManger(const Self &other) = default;
    
        MigrationManger(Self &&other) noexcept = default;
    
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
                this->migrations_ = std::move(other.migrations_);
                this->nextGroupId_ = other.nextGroupId_;
            }
            return *this;
        }
    
        ~MigrationManger() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        void Flush();

        template<typename _InsertIterator,typename _Check = decltype(*std::declval<_InsertIterator&>()++ = std::declval<const rkv::Migration&>())>
        inline void GetMigrations(_InsertIterator inserter,const sharpen::IpEndPoint &destination) const
        {
            for (auto begin = this->migrations_.begin(),end = this->migrations_.end(); begin != end; ++begin)
            {
                if (begin->Destination() == destination)
                {
                    *inserter++ = *begin;   
                }
            }
        }

        template<typename _InsertIterator,typename _Check = decltype(*std::declval<_InsertIterator&>()++ = std::declval<const rkv::Migration&>())>
        inline void GetMigrations(_InsertIterator inserter,sharpen::Uint64 groupId) const
        {
            for (auto begin = this->migrations_.begin(),end = this->migrations_.end(); begin != end; ++begin)
            {
                if(begin->GetGroupId() == groupId)
                {
                    *inserter++ = *begin;
                }
            }
        }

        template<typename _InsertIterator,typename _Check = decltype(*std::declval<_InsertIterator&>()++ = std::declval<const rkv::Migration&>())>
        inline void GetMigrations(_InsertIterator inserter,const sharpen::ByteBuffer &beginKey) const
        {
            for (auto begin = this->migrations_.begin(),end = this->migrations_.end(); begin != end; ++begin)
            {
                if(begin->BeginKey() == beginKey)
                {
                    *inserter++ = *begin;
                }   
            }
        }

        bool Contain(const sharpen::ByteBuffer &beginKey) const noexcept;

        template<typename _InsertIterator,typename _Iterator,typename _Check = decltype(*std::declval<_InsertIterator&>()++ = std::declval<rkv::RaftLog&>(),std::declval<rkv::Migration&>() = *std::declval<_Iterator&>())>
        inline std::uint64_t GenrateEmplaceLogs(_InsertIterator inserter,_Iterator begin,_Iterator end,std::uint64_t beginIndex,std::uint64_t term) const
        {
            std::size_t size{sharpen::GetRangeSize(begin,end)};
            if(size)
            {
                IndexType index{this->GenrateIndex()};
                while (begin != end)
                {
                    const rkv::Migration &migration{*begin};
                    rkv::RaftLog log;
                    log.SetOperation(rkv::RaftLog::Operation::Put);
                    log.SetIndex(beginIndex++);
                    log.SetTerm(term);
                    log.Key() = Self::FormatMigrationKey(migration.GetId());
                    sharpen::ByteBuffer buf;
                    migration.Serialize().StoreTo(buf);
                    log.Value() = std::move(buf);
                    index.emplace_back(migration.GetId());
                    *inserter++ = std::move(log);
                    ++begin;
                }
                rkv::RaftLog log;
                log.SetOperation(rkv::RaftLog::Operation::Put);
                log.SetIndex(beginIndex);
                log.SetTerm(term);
                log.Key() = Self::indexKey_;
                sharpen::ByteBuffer buf;
                sharpen::BinarySerializator::StoreTo(index,buf);
                log.Value() = std::move(buf);
                *inserter++ = std::move(log);
            }
            return beginIndex;
        }

        template<typename _InsertIterator,typename _Iterator,typename _Check = decltype(*std::declval<_InsertIterator&>()++ = std::declval<rkv::RaftLog&>(),std::declval<std::uint64_t&>() = *std::declval<_Iterator&>())>
        inline std::uint64_t GenrateRemoveLogs(_InsertIterator inserter,_Iterator begin,_Iterator end,std::uint64_t beginIndex,std::uint64_t term) const
        {
            std::size_t size{sharpen::GetRangeSize(begin,end)};
            if(size)
            {
                IndexType index{this->GenrateIndex()};
                for (auto ite = begin; ite != end; ++ite)
                {
                    for (auto indexBegin = index.begin(); indexBegin != index.end();)
                    {
                        if(*indexBegin == *ite)
                        {
                            indexBegin = index.erase(indexBegin);
                        }
                        else
                        {
                            ++indexBegin;
                        }   
                    }
                }
                rkv::RaftLog log;
                log.SetOperation(rkv::RaftLog::Operation::Put);
                log.SetIndex(beginIndex);
                log.SetTerm(term);
                log.Key() = Self::indexKey_;
                sharpen::ByteBuffer buf;
                sharpen::BinarySerializator::StoreTo(index,buf);
                log.Value() = std::move(buf);
                *inserter++ = std::move(log);
                while (begin != end)
                {
                    ++beginIndex;
                    std::uint64_t id{*begin};
                    rkv::RaftLog log;
                    log.SetOperation(rkv::RaftLog::Operation::Delete);
                    log.SetIndex(beginIndex);
                    log.SetTerm(term);
                    log.Key() = Self::FormatMigrationKey(id);
                    *inserter++ = std::move(log);
                    ++begin;
                }
            }
            return beginIndex;
        }

        inline std::uint64_t GetNextGroupId() const noexcept
        {
            return this->nextGroupId_;
        }

        inline std::uint64_t GetNextMirgationId() const noexcept
        {
            return this->nextMigrationId_;
        }
    };
}

#endif