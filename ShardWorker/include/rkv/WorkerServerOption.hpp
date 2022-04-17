#pragma once
#ifndef _RKV_RAFTSERVEROPTION_HPP
#define _RKV_RAFTSERVEROPTION_HPP

#include <sharpen/IpEndPoint.hpp>
#include <sharpen/IteratorOps.hpp>

namespace rkv
{
    class WorkerServerOption
    {
    private:
        using Self = rkv::WorkerServerOption;
        using MasterEndpoints = std::vector<sharpen::IpEndPoint>;
    public:
        using Iterator = typename MasterEndpoints::iterator;
        using ConstIterator = typename MasterEndpoints::const_iterator;
    private:
    
        sharpen::IpEndPoint bindEndpoint_;
        sharpen::IpEndPoint selfId_;
        MasterEndpoints masterEndpoints_;
    public:
    
        template<typename _Iterator,typename _Check = decltype(std::declval<sharpen::IpEndPoint&>() = *std::declval<_Iterator&>())>
        WorkerServerOption(const sharpen::IpEndPoint &bindEndpoint,const sharpen::IpEndPoint &selfId,_Iterator masterBegin,_Iterator masterEnd)
            :bindEndpoint_(bindEndpoint)
            ,selfId_(selfId)
            ,masterEndpoints_()
        {
            std::size_t size{sharpen::GetRangeSize(masterBegin,masterEnd)};
            assert(size != 0);
            while (masterBegin != masterEnd)
            {
                this->masterEndpoints_.emplace_back(*masterBegin);
                ++masterBegin;
            }
        }
    
        WorkerServerOption(const Self &other) = default;
    
        WorkerServerOption(Self &&other) noexcept = default;
    
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
                this->bindEndpoint_ = std::move(other.bindEndpoint_);
            }
            return *this;
        }
    
        ~WorkerServerOption() noexcept = default;

        inline sharpen::IpEndPoint &SelfId() noexcept
        {
            return this->selfId_;
        }

        inline const sharpen::IpEndPoint &SelfId() const noexcept
        {
            return this->selfId_;
        }

        inline sharpen::IpEndPoint &BindEndpoint() noexcept
        {
            return this->bindEndpoint_;
        }

        inline const sharpen::IpEndPoint &BindEndpoint() const noexcept
        {
            return this->bindEndpoint_;
        }

        inline Iterator MasterBegin() noexcept
        {
            return this->masterEndpoints_.begin();
        }
        
        inline ConstIterator MasterBegin() const noexcept
        {
            return this->masterEndpoints_.begin();
        }
        
        inline Iterator MasterEnd() noexcept
        {
            return this->masterEndpoints_.end();
        }
        
        inline ConstIterator MasterEnd() const noexcept
        {
            return this->masterEndpoints_.end();
        }

        inline std::size_t GetMasterSize() const noexcept
        {
            return this->masterEndpoints_.size();
        }

        inline bool MasterEmpty() const noexcept
        {
            return this->masterEndpoints_.empty();
        }
    };
}

#endif