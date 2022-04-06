#pragma once
#ifndef _RKV_RAFTSERVEROPTION_HPP
#define _RKV_RAFTSERVEROPTION_HPP

#include <vector>

#include <sharpen/IpEndPoint.hpp>
#include <sharpen/IteratorOps.hpp>

namespace rkv
{
    class RaftServerOption
    {
    private:
        using Self = rkv::RaftServerOption;
        using MemberIds = std::vector<sharpen::IpEndPoint>;
    public:
        using Iterator = typename MemberIds::iterator;
        using ConstIterator = typename MemberIds::const_iterator;
    private:
    
        sharpen::IpEndPoint selfId_;
        MemberIds memberIds_;
    public:
    
        template<typename _Iterator,typename _Check = decltype(std::declval<sharpen::IpEndPoint&>() = *std::declval<_Iterator&>()++)>
        RaftServerOption(const sharpen::IpEndPoint &selfId,_Iterator begin,_Iterator end)
            :selfId_(selfId)
            ,memberIds_()
        {
            std::size_t size{sharpen::GetRangeSize(begin,end)};
            if(size)
            {
                this->memberIds_.reserve(size);
                while (begin != end)
                {
                    this->memberIds_.emplace_back(*begin);
                    ++begin;
                }
            }
        }
    
        RaftServerOption(const Self &other) = default;
    
        RaftServerOption(Self &&other) noexcept = default;
    
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
            }
            return *this;
        }
    
        ~RaftServerOption() noexcept = default;
        
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