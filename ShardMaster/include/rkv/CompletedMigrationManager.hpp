#pragma once
#ifndef _RKV_COMPLETEDMIGRATIONMANAGER_HPP
#define _RKV_COMPLETEDMIGRATIONMANAGER_HPP

#include <rkv/CompletedMigration.hpp>
#include <rkv/RaftLog.hpp>
#include <rkv/KeyValueService.hpp>

namespace rkv
{
    class CompletedMigrationManager
    {
    private:
        using Self = rkv::CompletedMigrationManager;

        static sharpen::ByteBuffer countKey_;

        static std::once_flag flag_;

        static void InitKeys();

        static sharpen::ByteBuffer FormatMigrationKey(std::uint64_t id);

        const rkv::KeyValueService *service_;
        std::vector<rkv::CompletedMigration> migrations_;
    public:

        explicit CompletedMigrationManager(const rkv::KeyValueService &service);

        CompletedMigrationManager(const Self &other) = default;

        CompletedMigrationManager(Self &&other) noexcept = default;

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
            }
            return *this;
        }

        ~CompletedMigrationManager() noexcept = default;

        inline const Self &Const() const noexcept
        {
            return *this;
        }

        template<typename _InsertIterator,typename _Check = decltype(*std::declval<_InsertIterator &>()++ = std::declval<const rkv::CompletedMigration &>())>
        inline void GetCompletedMigrations(_InsertIterator inserter,std::uint64_t source,std::uint64_t beginId)
        {
            std::size_t size{this->migrations_.size()};
            if(size >= beginId)
            {
                auto begin = this->migrations_.begin(),end = this->migrations_.end();
                begin = sharpen::IteratorForward(begin,beginId);
                while(begin != end)
                {
                    if(begin->source_ == source)
                    {
                        *inserter++ = *begin;
                    }
                    ++begin;
                }
            }
        }

        template<typename _InsertIterator,typename _Iterator,typename _Check = decltype(*std::declval<_InsertIterator&>()++ = std::declval<rkv::RaftLog&&>(),std::declval<const rkv::CompletedMigration&>() = *std::declval<_Iterator>())>
        inline std::uint64_t GenrateEmplaceLogs(_InsertIterator inserter,_Iterator beign,_Iterator end,std::uint64_t beginIndex,std::uint64_t term)
        {
            std::size_t size{sharpen::GetRangeSize(beign,end)};
            if(size)
            {
                std::uint64_t oldSize{this->migrations_.size()};
                while (begin != end)
                {
                    const rkv::CompletedMigration &migration{*beign};
                    rkv::RaftLog log;
                    log.SetOperation(rkv::RaftLog::Operation::Put);
                    log.SetIndex(beginIndex++);
                    log.SetTerm(term);
                    log.Key() = Self::FormatMigrationKey(migration.GetId());
                    sharpen::ByteBuffer buf;
                    migration.Serialize().StoreTo(buf);
                    log.Value() = std::move(buf);
                    *inserter++ = std::move(log);
                    ++begin;
                }
                oldSize += size;
                rkv::RaftLog log;
                log.SetOperation(rkv::RaftLog::Operation::Put);
                log.SetIndex(beginIndex);
                log.SetTerm(term);
                log.Key() = Self::countKey_;
                sharpen::ByteBuffer buf{sizeof(oldSize)};
                buf.As<std::uint64_t>() = oldSize;
                log.Value() = std::move(buf);
                *inserter++ = std::move(log);
            }
            return beginIndex;
        }

        inline std::size_t GetSize() const noexcept
        {
            return this->migrations_.size();
        }

        inline std::size_t GetNextId() const noexcept
        {
            return this->GetSize();
        }

        inline bool Empty() const noexcept
        {
            return this->migrations_.empty();
        }

        void Flush();
    };
}

#endif