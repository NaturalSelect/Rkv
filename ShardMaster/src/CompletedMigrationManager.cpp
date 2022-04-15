#include <rkv/CompletedMigrationManager.hpp>

void rkv::CompletedMigrationManager::InitKeys()
{
    countKey_.ExtendTo(2);
    countKey_[0] = 'c';
    countKey_[1] = 'c';
}

rkv::CompletedMigrationManager::CompletedMigrationManager(const rkv::KeyValueService &service)
    :service_(&service)
    ,migrations_()
{
    using FnPtr = void(*)();
    std::call_once(Self::flag_,static_cast<FnPtr>(&Self::InitKeys));
    this->Flush();
}

sharpen::ByteBuffer rkv::CompletedMigrationManager::FormatMigrationKey(std::uint64_t id)
{
    sharpen::ByteBuffer key{1};
    sharpen::Varuint64 buidler{id};
    sharpen::BinarySerializator::StoreTo(buidler,key,1);
    key[0] = 'c';
    return key;
}

void rkv::CompletedMigrationManager::Flush()
{
    sharpen::Optional<sharpen::ByteBuffer> countBuf{this->service_->TryGet(Self::countKey_)};
    if(countBuf.Exist())
    {
        std::uint64_t count{countBuf.Get().As<std::uint64_t>()};
        if (count != this->migrations_.size())
        {
            this->migrations_.clear();
            this->migrations_.reserve(sharpen::IntCast<std::size_t>(count));
            for (std::size_t i = 0; i != count; ++i)
            {
                sharpen::ByteBuffer buf{this->service_->Get(Self::FormatMigrationKey(i))};
                rkv::CompletedMigration migration;
                migration.Unserialize().LoadFrom(buf);
                this->migrations_.emplace_back(migration);
            }
        }
    }
}