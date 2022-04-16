#include <rkv/MigrationManger.hpp>

sharpen::ByteBuffer rkv::MigrationManger::indexKey_;

std::once_flag rkv::MigrationManger::flag_;

void rkv::MigrationManger::InitKeys()
{
    indexKey_.ExtendTo(2);
    indexKey_[0] = 't';
    indexKey_[1] = 'i';
}

sharpen::ByteBuffer rkv::MigrationManger::FormatMigrationKey(std::uint64_t id)
{
    sharpen::ByteBuffer key{1};
    sharpen::Varuint64 builder{id};
    sharpen::BinarySerializator::StoreTo(builder,key,1);
    key[0] = 'm';
    return key;
}

rkv::MigrationManger::MigrationManger(const rkv::KeyValueService &service)
    :service_(&service)
    ,migrations_()
    ,nextGroupId_(0)
    ,nextMigrationId_(0)
{
    using FnPtr = void(*)();
    std::call_once(Self::flag_,static_cast<FnPtr>(&Self::InitKeys));
    this->Flush();
}

void rkv::MigrationManger::Flush()
{
    sharpen::Optional<sharpen::ByteBuffer> indexBuf{this->service_->TryGet(Self::indexKey_)};
    if (indexBuf.Exist())
    {
        IndexType indexs;
        sharpen::BinarySerializator::LoadFrom(indexs,indexBuf.Get());
        if(!indexs.empty())
        {
            this->migrations_.clear();
            std::size_t size{indexs.size()};
            this->migrations_.reserve(size);
            std::uint64_t maxId{0};
            std::uint64_t maxGroupId{0};
            for (auto begin = indexs.begin(),end = indexs.end(); begin != end; ++begin)
            {
                sharpen::ByteBuffer buf{this->service_->Get(Self::FormatMigrationKey(*begin))};
                rkv::Migration migration;
                migration.Unserialize().LoadFrom(buf);
                if(migration.GetId() > maxId)
                {
                    maxId = migration.GetId();
                }
                if(migration.GetGroupId() > maxGroupId)
                {
                    maxGroupId = migration.GetGroupId();
                }
                this->migrations_.emplace_back(std::move(migration));
            }
            this->nextMigrationId_ = maxId + 1;
            this->nextGroupId_ = maxGroupId + 1;
        }
    }
}

rkv::MigrationManger::IndexType rkv::MigrationManger::GenrateIndex() const
{
    IndexType index;
    std::size_t size{this->migrations_.size()};
    if (size)
    {
        index.reserve(size);
        for (auto begin = this->migrations_.begin(),end = this->migrations_.end(); begin != end; ++begin)
        {
            index.emplace_back(begin->GetId());
        }
    }
    return index;
}

bool rkv::MigrationManger::Contain(const sharpen::ByteBuffer &beginKey) const noexcept
{
    for (auto begin = this->migrations_.begin(),end = this->migrations_.end(); begin != end; ++begin)
    {
        if(begin->BeginKey() == beginKey)
        {
            return true;
        }
    }
    return false;
}