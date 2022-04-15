#pragma once
#ifndef _RKV_RAFTLOG_HPP
#define _RKV_RAFTLOG_HPP

#include <sharpen/BinarySerializable.hpp>

namespace rkv
{
    class RaftLog:public sharpen::BinarySerializable<rkv::RaftLog>
    {
    public:
        enum class Operation:std::uint64_t
        {
            Put,
            Delete
        };
    private:
        using Self = rkv::RaftLog;
    
        sharpen::Uint64 index_;
        sharpen::Uint64 term_;
        Operation operation_;
        sharpen::ByteBuffer key_;
        sharpen::ByteBuffer value_;
    public:
    
        RaftLog()
            :index_(0)
            ,term_(0)
            ,operation_(Operation::Put)
            ,key_()
            ,value_()
        {}
    
        RaftLog(const Self &other) = default;
    
        RaftLog(Self &&other) noexcept = default;
    
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
                this->index_ = other.index_;
                this->term_ = other.term_;
                this->operation_ = other.operation_;
                this->key_ = std::move(other.key_);
                this->value_ = std::move(other.value_);
            }
            return *this;
        }
    
        ~RaftLog() noexcept = default;

        inline sharpen::Uint64 GetIndex() const noexcept
        {
            return this->index_;
        }

        inline sharpen::Uint64 GetTerm() const noexcept
        {
            return this->term_;
        }

        inline void SetIndex(sharpen::Uint64 index) noexcept
        {
            this->index_ = index;
        }

        inline void SetTerm(sharpen::Uint64 term) noexcept
        {
            this->term_ = term;
        }

        inline Operation GetOperation() const noexcept
        {
            return this->operation_;
        }

        inline void SetOperation(Operation ope) noexcept
        {
            this->operation_ = ope;
        }

        sharpen::ByteBuffer &Key() noexcept
        {
            return this->key_;
        }

        const sharpen::ByteBuffer &Key() const noexcept
        {
            return this->key_;
        }

        sharpen::ByteBuffer &Value() noexcept
        {
            return this->value_;
        }

        const sharpen::ByteBuffer &Value() const noexcept
        {
            return this->value_;
        }

        sharpen::Size ComputeSize() const noexcept;

        sharpen::Size LoadFrom(const char *data,sharpen::Size size);

        sharpen::Size UnsafeStoreTo(char *data) const noexcept;
    };
}

#endif