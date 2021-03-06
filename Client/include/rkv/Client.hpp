#pragma once
#ifndef _RKV_CLIENTOPERATOR_HPP
#define _RKV_CLIENTOPERATOR_HPP

#include <random>
#include <unordered_map>

#include <sharpen/INetStreamChannel.hpp>
#include <sharpen/IpEndPoint.hpp>
#include <sharpen/IteratorOps.hpp>

#include <rkv/LeaderRedirectRequest.hpp>
#include <rkv/LeaderRedirectResponse.hpp>
#include <rkv/MessageHeader.hpp>

namespace rkv
{
    class Client:public sharpen::Noncopyable
    {
    private:
        using Self = rkv::Client;
    protected:
        static void WriteMessage(sharpen::NetStreamChannelPtr channel,const rkv::MessageHeader &header);

        static void WriteMessage(sharpen::NetStreamChannelPtr channel,const rkv::MessageHeader &header,const sharpen::ByteBuffer &request);

        static void ReadMessage(sharpen::NetStreamChannelPtr channel,rkv::MessageType expectedType,sharpen::ByteBuffer &response);

        static sharpen::Optional<sharpen::IpEndPoint> GetLeaderId(sharpen::NetStreamChannelPtr channel,std::uint64_t group);

        static sharpen::Optional<sharpen::IpEndPoint> GetLeaderId(sharpen::NetStreamChannelPtr channel);

        sharpen::IpEndPoint GetRandomId() const noexcept;

        static sharpen::NetStreamChannelPtr MakeConnection(sharpen::EventEngine &engine,const sharpen::IpEndPoint &id);

        sharpen::NetStreamChannelPtr GetConnection(const sharpen::IpEndPoint &id) const;

        sharpen::NetStreamChannelPtr MakeRandomConnection() const;

        void EraseConnection(const sharpen::IpEndPoint &id);

        void FillLeaderId();

        sharpen::EventEngine *engine_;
        mutable std::minstd_rand random_;
        mutable std::uniform_int_distribution<std::size_t> distribution_;
        sharpen::Optional<sharpen::IpEndPoint> leaderId_;
        mutable std::unordered_map<sharpen::IpEndPoint,sharpen::NetStreamChannelPtr> serverMap_;
        sharpen::TimerPtr timer_;
        std::chrono::milliseconds restoreTimeout_;
        std::size_t maxTimeoutCount_;
        sharpen::Optional<std::uint64_t> group_;
    public:
    
        template<typename _Iterator,typename _Rep,typename _Period,typename _Check = decltype(std::declval<sharpen::IpEndPoint&>() = *std::declval<_Iterator&>()++)>
        Client(sharpen::EventEngine &engine,_Iterator begin,_Iterator end,const std::chrono::duration<_Rep,_Period> &restoreTimeout,std::size_t maxTimeoutCount)
            :engine_(&engine)
            ,random_(std::random_device{}())
            ,distribution_(1,sharpen::GetRangeSize(begin,end))
            ,leaderId_(sharpen::EmptyOpt)
            ,serverMap_()
            ,timer_(sharpen::MakeTimer(*this->engine_))
            ,restoreTimeout_(restoreTimeout)
            ,maxTimeoutCount_(maxTimeoutCount)
            ,group_(sharpen::EmptyOpt)
        {
            assert(begin != end);
            while (begin != end)
            {
                this->serverMap_.emplace(*begin,nullptr);
                ++begin;
            }
        }
    
        Client(Self &&other) noexcept;
    
        inline Self &operator=(Self &&other) noexcept
        {
            if(this != std::addressof(other))
            {
                this->engine_ = other.engine_;
                this->random_ = std::move(other.random_);
                this->distribution_ = std::move(other.distribution_);
                this->leaderId_ = std::move(other.leaderId_);
                this->serverMap_ = std::move(other.serverMap_);
                this->timer_ = std::move(other.timer_);
                this->restoreTimeout_ = other.restoreTimeout_;
                this->maxTimeoutCount_ = other.maxTimeoutCount_;
            }
            return *this;
        }
    
        ~Client() noexcept = default;
    
        inline const Self &Const() const noexcept
        {
            return *this;
        }

        inline sharpen::Optional<std::uint64_t> &Group() noexcept
        {
            return this->group_;
        }

        inline const sharpen::Optional<std::uint64_t> &Group() const noexcept
        {
            return this->group_;
        }

        inline void Cancel()
        {
            for (auto begin = this->serverMap_.begin(),end = this->serverMap_.end(); begin != end; ++begin)
            {
                if(begin->second)
                {
                    begin->second->Cancel();
                    begin->second->Close();
                }
            }
        }

        inline void Reset()
        {
            this->leaderId_.Reset();
            for (auto begin = this->serverMap_.begin(),end = this->serverMap_.end(); begin != end; ++begin)
            {
                begin->second.reset();
            }
        }
    };
}

#endif