#pragma once
#ifndef _RKV_RAFTSERVEROPTION_HPP
#define _RKV_RAFTSERVEROPTION_HPP

#include <vector>

#include <sharpen/IpEndPoint.hpp>
#include <sharpen/IteratorOps.hpp>

namespace rkv
{
    class MasterServerOption
    {
    private:
        using Self = rkv::MasterServerOption;
        using MemberIds = std::vector<sharpen::IpEndPoint>;
    public:
        using Iterator = typename MemberIds::iterator;
        using ConstIterator = typename MemberIds::const_iterator;
    private:
    
        sharpen::IpEndPoint selfId_;
        MemberIds memberIds_;
        MemberIds workerIds_;
    public:
    
        template<typename _MemberIterator,typename _WorkerIterator,typename _Check = decltype(std::declval<sharpen::IpEndPoint&>() = *std::declval<_MemberIterator&>()++,std::declval<sharpen::IpEndPoint&>() = *std::declval<_WorkerIterator&>()++)>
        MasterServerOption(const sharpen::IpEndPoint &selfId,_MemberIterator memberBegin,_MemberIterator memberEnd,_WorkerIterator workerBegin,_WorkerIterator workerEnd)
            :selfId_(selfId)
            ,memberIds_()
            ,workerIds_()
        {
            std::size_t size{sharpen::GetRangeSize(memberBegin,memberEnd)};
            if(size)
            {
                this->memberIds_.reserve(size);
                while (memberBegin != memberEnd)
                {
                    this->memberIds_.emplace_back(*memberBegin);
                    ++memberBegin;
                }
            }
            size = sharpen::GetRangeSize(workerBegin,workerEnd);
            assert(size);
            while (workerBegin != workerEnd)
            {
                this->workerIds_.emplace_back(*workerBegin);
                ++workerBegin;   
            }
        }
    
        MasterServerOption(const Self &other) = default;
    
        MasterServerOption(Self &&other) noexcept = default;
    
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
                this->selfId_ = std::move(other.selfId_);
                this->memberIds_ = std::move(other.memberIds_);
                this->workerIds_ = std::move(other.workerIds_);
            }
            return *this;
        }
    
        ~MasterServerOption() noexcept = default;
        
        inline Iterator MembersBegin() noexcept
        {
            return this->memberIds_.begin();
        }
        
        inline ConstIterator MembersBegin() const noexcept
        {
            return this->memberIds_.begin();
        }
        
        inline Iterator MembersEnd() noexcept
        {
            return this->memberIds_.end();
        }
        
        inline ConstIterator MembersEnd() const noexcept
        {
            return this->memberIds_.end();
        }

        inline std::size_t GetMembersSize() const noexcept
        {
            return this->memberIds_.size();
        }

        inline bool MembersIsEmpty() const noexcept
        {
            return this->memberIds_.empty();
        }

        inline Iterator WorkersBegin() noexcept
        {
            return this->workerIds_.begin();
        }
        
        inline ConstIterator WorkersBegin() const noexcept
        {
            return this->workerIds_.begin();
        }
        
        inline Iterator WorkersEnd() noexcept
        {
            return this->workerIds_.end();
        }
        
        inline ConstIterator WorkersEnd() const noexcept
        {
            return this->workerIds_.end();
        }

        inline std::size_t GetWorkersSize() const noexcept
        {
            return this->workerIds_.size();
        }

        inline bool WorkersIsEmpty() const noexcept
        {
            return this->workerIds_.empty();
        }

        inline sharpen::IpEndPoint &SelfId() noexcept
        {
            return this->selfId_;
        }

        inline const sharpen::IpEndPoint &SelfId() const noexcept
        {
            return this->selfId_;
        }
    };
}

#endif