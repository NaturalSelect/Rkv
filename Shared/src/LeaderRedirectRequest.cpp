#include <rkv/LeaderRedirectRequest.hpp>

std::size_t rkv::LeaderRedirectRequest::ComputeSize() const noexcept
{
    std::size_t size{0};
    bool exist{this->group_.Exist()};
    size += Helper::ComputeSize(exist);
    if(exist)
    {
        sharpen::Varuint64 builder{this->group_.Get()};
        size += Helper::ComputeSize(builder);
    }
    return size;
}

std::size_t rkv::LeaderRedirectRequest::LoadFrom(const char *data,std::size_t size)
{
    if(!size)
    {
        throw std::invalid_argument("invalid buffer");
    }
    bool exist;
    std::size_t offset{0};
    offset += Helper::LoadFrom(exist,data,size);
    if(exist)
    {
        if(size == offset)
        {
            throw sharpen::DataCorruptionException("leader rediect request corruption");
        }
        sharpen::Varuint64 builder{0};
        offset += Helper::LoadFrom(builder,data + offset,size - offset);
        this->group_.Construct(builder.Get());
    }
    else
    {
        this->group_.Reset();
    }
    return offset;
}

std::size_t rkv::LeaderRedirectRequest::UnsafeStoreTo(char *data) const noexcept
{
    std::size_t offset{0};
    bool exist{this->group_.Exist()};
    offset += Helper::UnsafeStoreTo(exist,data);
    if(exist)
    {
        sharpen::Varuint64 builder{this->group_.Get()};
        offset += Helper::UnsafeStoreTo(builder,data + offset);
    }
    return offset;
}