#include <rkv/LeaderRedirectResponse.hpp>

std::size_t rkv::LeaderRedirectResponse::ComputeSize() const noexcept
{
    std::size_t size{0};
    size += Helper::ComputeSize(this->knowLeader_);
    if(this->knowLeader_)
    {
        size += Helper::ComputeSize(this->ep_);
    }
    return size;
}

std::size_t rkv::LeaderRedirectResponse::LoadFrom(const char *data,std::size_t size)
{
    if(!size)
    {
        throw std::invalid_argument("invalid leader redirect response buffer");
    }
    std::size_t offset{0};
    offset += Helper::LoadFrom(this->knowLeader_,data,size);
    if(this->knowLeader_)
    {
        if(size < offset + sizeof(this->ep_))
        {
            throw sharpen::DataCorruptionException("leader redirect response corruption");
        }
        offset += Helper::LoadFrom(this->ep_,data + offset,size - offset);
    }
    return offset;
}

std::size_t rkv::LeaderRedirectResponse::UnsafeStoreTo(char *data) const noexcept
{
    std::size_t offset{0};
    offset += Helper::UnsafeStoreTo(this->knowLeader_,data);
    if(this->knowLeader_)
    {
        offset += Helper::UnsafeStoreTo(this->ep_,data + offset);
    }
    return offset;
}