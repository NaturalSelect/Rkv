#include <rkv/CompletedMigration.hpp>

std::size_t rkv::CompletedMigration::ComputeSize() const noexcept
{
    std::size_t size{0};
    sharpen::Varuint64 builder{this->id_};
    size += Helper::ComputeSize(builder);
    builder.Set(this->source_);
    size += Helper::ComputeSize(builder);
    builder.Set(this->destination_);
    size += Helper::ComputeSize(builder);
    size += this->beginKey_.ComputeSize();
    size += this->endKey_.ComputeSize();
    return size;
}

std::size_t rkv::CompletedMigration::LoadFrom(const char *data,std::size_t size)
{
    if(size < 5)
    {
        throw std::invalid_argument("invalid completed migration buffer");
    }
    std::size_t offset{0};
    sharpen::Varuint64 builder{0};
    offset += Helper::LoadFrom(builder,data,size);
    this->id_ = builder.Get();
    if(size < offset + 4)
    {
        throw sharpen::DataCorruptionException("completed migration corruption");
    }
    offset += Helper::LoadFrom(builder,data + offset,size - offset);
    this->source_ = builder.Get();
    if(size < offset + 3)
    {
        throw sharpen::DataCorruptionException("completed migration corruption");
    }
    offset += Helper::LoadFrom(builder,data + offset,size - offset);
    this->destination_ = builder.Get();
    if (size < offset + 2)
    {
        throw sharpen::DataCorruptionException("completed migration corruption");
    }
    offset += this->beginKey_.LoadFrom(data + offset,size - offset);
    if (size == offset)
    {
        throw sharpen::DataCorruptionException("completed migration corruption");
    }
    offset += this->endKey_.LoadFrom(data + offset,size - offset);
    return offset;
}

std::size_t rkv::CompletedMigration::UnsafeStoreTo(char *data) const noexcept
{
    std::size_t offset{0};
    sharpen::Varuint64 builder{this->id_};
    offset += Helper::UnsafeStoreTo(builder,data);
    builder.Set(this->source_);
    offset += Helper::UnsafeStoreTo(builder,data + offset);
    builder.Set(this->destination_);
    offset += Helper::UnsafeStoreTo(builder,data + offset);
    offset += this->beginKey_.UnsafeStoreTo(data + offset);
    offset += this->endKey_.UnsafeStoreTo(data + offset);
    return offset;
}