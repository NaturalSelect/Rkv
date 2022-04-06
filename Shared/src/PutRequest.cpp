#include <rkv/PutRequest.hpp>

std::size_t rkv::PutRequest::ComputeSize() const noexcept
{
    std::size_t size{0};
    sharpen::Varuint64 builder{this->key_.GetSize()};
    size += Helper::ComputeSize(builder);
    size += this->key_.GetSize();
    builder.Set(this->value_.GetSize());
    size += Helper::ComputeSize(builder);
    size += this->value_.GetSize();
    return size;
}

std::size_t rkv::PutRequest::LoadFrom(const char *data,std::size_t size)
{
    if(size < 2)
    {
        throw std::invalid_argument("invalid put request buffer");
    }
    std::size_t offset{0};
    sharpen::Varuint64 builder{0};
    offset += Helper::LoadFrom(builder,data,size);
    std::size_t sz{builder.Get()};
    if(size < offset + sz)
    {
        throw sharpen::DataCorruptionException("put request corruption");
    }
    sharpen::ByteBuffer key{sharpen::IntCast<std::size_t>(sz)};
    std::memcpy(key.Data(),data + offset,sz);
    offset += sz;
    this->key_ = std::move(key);
    if (size <= offset)
    {
        throw sharpen::DataCorruptionException("put request corruption");
    }
    offset += Helper::LoadFrom(builder,data + offset,size - offset);
    sz = builder.Get();
    if(sz)
    {
        if(size < offset + sz)
        {
            throw sharpen::DataCorruptionException("put request corruption");
        }
        sharpen::ByteBuffer value{sharpen::IntCast<std::size_t>(sz)};
        std::memcpy(value.Data(),data + offset,sz);
        offset += sz;
    }
    return offset;
}

std::size_t rkv::PutRequest::UnsafeStoreTo(char *data) const noexcept
{
    std::size_t offset{0};
    sharpen::Varuint64 builder{this->key_.GetSize()};
    offset += Helper::UnsafeStoreTo(builder,data);
    std::memcpy(data + offset,this->key_.Data(),this->key_.GetSize());
    offset += this->key_.GetSize();
    builder.Set(this->value_.GetSize());
    offset += Helper::UnsafeStoreTo(builder,data + offset);
    std::memcpy(data + offset,this->value_.Data(),this->value_.GetSize());
    offset += this->value_.GetSize();
    return offset;
}