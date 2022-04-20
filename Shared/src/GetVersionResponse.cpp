#include <rkv/GetVersionResponse.hpp>

std::size_t rkv::GetVersionResponse::ComputeSize() const noexcept
{
    std::size_t size{0};
    sharpen::Varuint64 builder{this->version_};
    size += Helper::ComputeSize(builder);
    return size;
}

std::size_t rkv::GetVersionResponse::LoadFrom(const char *data,std::size_t size)
{
    if(!size)
    {
        throw std::invalid_argument("invalid buffer");
    }
    std::size_t offset{0};
    sharpen::Varuint64 builder{0};
    offset += Helper::LoadFrom(builder,data,size);
    this->version_ = builder.Get();
    return offset;
}

std::size_t rkv::GetVersionResponse::UnsafeStoreTo(char *data) const noexcept
{
    std::size_t offset{0};
    sharpen::Varuint64 builder{this->version_};
    offset += Helper::UnsafeStoreTo(builder,data);
    return offset;
}