#pragma once
#ifndef _RKV_KEYVALUESERVICE_HPP
#define _RKV_KEYVALUESERVICE_HPP

#include <sharpen/LevelTable.hpp>

#include "RaftLog.hpp"
#include "RaftMember.hpp"
#include "RaftStorage.hpp"

namespace rkv
{
    class KeyValueService:public sharpen::Noncopyable
    {
    private:
        using Self = rkv::KeyValueService;
    
        sharpen::LevelTable table_;
    public:
    
        KeyValueService(sharpen::EventEngine &eng,const std::string &dbName)
            :table_(eng,dbName,"kvdb")
        {}
    
        KeyValueService(Self &&other) noexcept = default;
    
        inline Self &operator=(Self &&other) noexcept
        {
            if(this != std::addressof(other))
            {
                this->table_ = std::move(other.table_);
            }
            return *this;
        }
    
        ~KeyValueService() noexcept = default;

        inline sharpen::ByteBuffer Get(const sharpen::ByteBuffer &key) const
        {
            return this->table_.Get(key);
        }

        inline sharpen::Optional<sharpen::ByteBuffer> TryGet(const sharpen::ByteBuffer &key) const
        {
            return this->table_.TryGet(key);
        }

        inline void Put(sharpen::ByteBuffer key,sharpen::ByteBuffer value)
        {
            this->table_.Put(std::move(key),std::move(value));
        }

        inline void Delete(sharpen::ByteBuffer key)
        {
            this->table_.Delete(std::move(key));
        }

        void Apply(rkv::RaftLog log);
    };
}

#endif