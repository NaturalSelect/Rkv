#include <rkv/Migration.hpp>

std::size_t rkv::Migration::ComputeSize() const noexcept
{
    std::size_t size{0};
    sharpen::Varuint64 builder{this->id_};
    size += Helper::ComputeSize(builder);
    builder.Set(this->source_);
    size += Helper::ComputeSize(builder);
    size += Helper::ComputeSize(this->destination_);
    size += Helper::ComputeSize(this->beginKey_);
    size += Helper::ComputeSize(this->endKey_);
    return size;
}

std::size_t rkv::Migration::LoadFrom(const char *data,std::size_t size)
{
    if(size < 4 + Helper::ComputeSize(this->destination_))
    {
        throw std::invalid_argument("invalid migration buffer");
    }
    std::size_t offset{0};
    
    return offset;
}