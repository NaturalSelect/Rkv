#include <rkv/GetResponse.hpp>

std::size_t rkv::GetResponse::ComputeSize() const noexcept
{
    std::size_t size{0};
    sharpen::Varuint64 builder{this->value_.GetSize()};
    size += Helper::ComputeSize(builder);
    size += this->value_.GetSize();
    return size;
}

std::size_t rkv::GetResponse::LoadFrom(const char *data,std::size_t size)
{
    if(!size)
    {
        throw std::invalid_argument("invalid get response buffer");
    }
    std::size_t offset{0};
    sharpen::Varuint64 builder{0};
    offset += Helper::LoadFrom(builder,data,size);
    std::size_t sz{sharpen::IntCast<sharpen::Size>(builder.Get())};
    if(sz)
    {
        if(size < offset + sz)
        {
            throw sharpen::DataCorruptionException("get response corruption");
        }
        sharpen::ByteBuffer value{sharpen::IntCast<std::size_t>(sz)};
        std::memcpy(value.Data(),data + offset,sz);
        this->value_ = std::move(value);
        offset += sz;
    }
    return offset;
}

std::size_t rkv::GetResponse::UnsafeStoreTo(char *data) const noexcept
{
    std::size_t offset{0};
    sharpen::Varuint64 builder{this->value_.GetSize()};
    offset += Helper::UnsafeStoreTo(builder,data);
    std::memcpy(data + offset,this->value_.Data(),this->value_.GetSize());
    offset += this->value_.GetSize();
    return offset;
}