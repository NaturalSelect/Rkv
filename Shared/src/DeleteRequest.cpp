#include <rkv/DeleteRequest.hpp>

std::size_t rkv::DeleteRequest::ComputeSize() const noexcept
{
    std::size_t size{0};
    sharpen::Varuint64 builder{this->key_.GetSize()};
    size += Helper::ComputeSize(builder);
    size += this->key_.GetSize();
    return size;
}

std::size_t rkv::DeleteRequest::LoadFrom(const char *data,std::size_t size)
{
    if(!size)
    {
        throw std::invalid_argument("invalid Delete request buffer");
    }
    std::size_t offset{0};
    sharpen::Varuint64 builder{0};
    offset += Helper::LoadFrom(builder,data,size);
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
    sharpen::Varuint64 builder{this->key_.GetSize()};
    offset += Helper::UnsafeStoreTo(builder,data);
    std::memcpy(data + offset,this->key_.Data(),this->key_.GetSize());
    offset += this->key_.GetSize();
    return offset;
}