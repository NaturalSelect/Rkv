#include <rkv/DeleteRequest.hpp>

std::size_t rkv::DeleteRequest::ComputeSize() const noexcept
{
    std::size_t size{0};
    sharpen::Varuint64 builder{0};
    builder.Set(this->version_);
    size += Helper::ComputeSize(builder);
    builder.Set(this->key_.GetSize());
    size += Helper::ComputeSize(builder);
    size += this->key_.GetSize();
    return size;
}

std::size_t rkv::DeleteRequest::LoadFrom(const char *data,std::size_t size)
{
    if(size < 2)
    {
        throw std::invalid_argument("invalid Delete request buffer");
    }
    std::size_t offset{0};
    sharpen::Varuint64 builder{0};
    offset += Helper::LoadFrom(builder,data,size);
    this->version_ = builder.Get();
    if(offset == size)
    {
        throw sharpen::DataCorruptionException("Delete request corruption");
    }
    offset += Helper::LoadFrom(builder,data + offset,size - size);
    std::size_t sz{sharpen::IntCast<sharpen::Size>(builder.Get())};
    if(size < offset + sz)
    {
        throw sharpen::DataCorruptionException("Delete request corruption");
    }
    this->key_.ExtendTo(sz);
    std::memcpy(this->key_.Data(),data + offset,sz);
    offset += sz;
    return offset;
}

std::size_t rkv::DeleteRequest::UnsafeStoreTo(char *data) const noexcept
{
    std::size_t offset{0};
    sharpen::Varuint64 builder{0};
    builder.Set(this->version_);
    offset += Helper::UnsafeStoreTo(builder,data);
    builder.Set(this->key_.GetSize());
    offset += Helper::UnsafeStoreTo(builder,data + offset);
    std::memcpy(data + offset,this->key_.Data(),this->key_.GetSize());
    offset += this->key_.GetSize();
    return offset;
}