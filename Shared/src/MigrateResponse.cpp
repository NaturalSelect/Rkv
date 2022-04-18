#include <rkv/MigrateResponse.hpp>

std::size_t rkv::MigrateResponse::ComputeSize() const noexcept
{
    std::size_t size{0};
    sharpen::Varuint64 builder{this->map_.size()};
    size += Helper::ComputeSize(builder);
    for(auto begin = this->map_.begin(),end = this->map_.end(); begin != end; ++begin)
    {
        size += begin->first.ComputeSize();
        size += begin->second.ComputeSize();
    }   
    return size;
}

std::size_t rkv::MigrateResponse::LoadFrom(const char *data,std::size_t size)
{
    if(!size)
    {
        throw std::invalid_argument("invalid buffer");
    }
    std::size_t offset{0};
    sharpen::Varuint64 builder{0};
    offset += Helper::LoadFrom(builder,data,size);
    for (std::size_t i = 0,count = sharpen::IntCast<std::size_t>(builder.Get()); i != count; ++i)
    {
        if(size == offset)
        {
            throw sharpen::DataCorruptionException("migrate response corruption");
        }
        sharpen::ByteBuffer key;
        offset += key.LoadFrom(data + offset,size - offset);
        if (size == offset)
        {
            throw sharpen::DataCorruptionException("migrate response corruption");
        }
        sharpen::ByteBuffer value;
        offset += value.LoadFrom(data + offset,size - offset);
        this->map_.emplace(std::move(key),std::move(value));
    }   
    return offset;
}

std::size_t rkv::MigrateResponse::UnsafeStoreTo(char *data) const noexcept
{
    std::size_t offset{0};
    sharpen::Varuint64 builder{this->map_.size()};
    offset += Helper::UnsafeStoreTo(builder,data);
    for(auto begin = this->map_.begin(),end = this->map_.end(); begin != end; ++begin)
    {
        offset += begin->first.UnsafeStoreTo(data + offset);
        offset += begin->second.UnsafeStoreTo(data + offset);
    }   
    return offset;
}