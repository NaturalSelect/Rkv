#include <rkv/Migration.hpp>

std::size_t rkv::Migration::ComputeSize() const noexcept
{
    std::size_t size{0};
    sharpen::Varuint64 builder{this->id_};
    size += Helper::ComputeSize(builder);
    builder.Set(this->groupId_);
    size += Helper::ComputeSize(this->groupId_);
    builder.Set(this->source_);
    size += Helper::ComputeSize(builder);
    size += Helper::ComputeSize(this->destination_);
    size += this->beginKey_.ComputeSize();
    size += this->endKey_.ComputeSize();
    return size;
}

std::size_t rkv::Migration::LoadFrom(const char *data,std::size_t size)
{
    if(size < 5 + Helper::ComputeSize(this->destination_))
    {
        throw std::invalid_argument("invalid migration buffer");
    }
    std::size_t offset{0};
    sharpen::Varuint64 builder{0};
    offset += Helper::LoadFrom(builder,data,size);
    this->id_ = builder.Get();
    if(size < offset + 4 + Helper::ComputeSize(this->destination_))
    {
        throw sharpen::DataCorruptionException("migration corruption");
    }
    offset += Helper::LoadFrom(builder,data + offset,size - offset);
    this->groupId_ = builder.Get();
    if(size < offset + 3 + Helper::ComputeSize(this->destination_))
    {
        throw sharpen::DataCorruptionException("migration corruption");
    }
    offset += Helper::LoadFrom(builder,data + offset,size - offset);
    this->source_ = builder.Get();
    if(size < offset + 2 + Helper::ComputeSize(this->destination_))
    {
        throw sharpen::DataCorruptionException("migration corruption");
    }
    offset += Helper::LoadFrom(this->destination_,data + offset,size - offset);
    if(size < offset + 2)
    {
        throw sharpen::DataCorruptionException("migration corruption");
    }
    offset += this->beginKey_.LoadFrom(data + offset,size - offset);
    if(size == offset)
    {
        throw sharpen::DataCorruptionException("migration corruption");
    }
    offset += this->endKey_.LoadFrom(data + offset,size - offset);
    return offset;
}

std::size_t rkv::Migration::UnsafeStoreTo(char *data) const noexcept
{
    std::size_t offset{0};
    sharpen::Varuint64 builder{this->id_};
    offset += Helper::UnsafeStoreTo(builder,data);
    builder.Set(this->groupId_);
    offset += Helper::UnsafeStoreTo(builder,data + offset);
    builder.Set(this->source_);
    offset += Helper::UnsafeStoreTo(builder,data + offset);
    offset += Helper::UnsafeStoreTo(this->destination_,data + offset);
    offset += this->beginKey_.UnsafeStoreTo(data + offset);
    offset += this->endKey_.UnsafeStoreTo(data + offset);
    return offset;
}