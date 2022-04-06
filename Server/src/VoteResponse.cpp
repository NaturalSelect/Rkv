#include <rkv/VoteResponse.hpp>

std::size_t rkv::VoteResponse::ComputeSize() const noexcept
{
    std::size_t size{0};
    size += Helper::ComputeSize(this->success_);
    sharpen::Varuint64 builder{this->term_};
    size += Helper::ComputeSize(builder);
    return size;
}

std::size_t rkv::VoteResponse::LoadFrom(const char *data,std::size_t size)
{
    std::size_t offset{0};
    if(size < 2)
    {
        throw std::invalid_argument("invalid vote response buffer");
    }
    offset += Helper::LoadFrom(this->success_,data,size);
    if(size <= offset)
    {
        throw sharpen::DataCorruptionException("vote response corruption");
    }
    sharpen::Varuint64 builder{0};
    offset += Helper::LoadFrom(builder,data + offset,size - offset);
    return offset;
}

std::size_t rkv::VoteResponse::UnsafeStoreTo(char *data) const noexcept
{
    std::size_t offset{0};
    offset += Helper::UnsafeStoreTo(this->success_,data);
    sharpen::Varuint64 builder{this->term_};
    offset += Helper::UnsafeStoreTo(builder,data + offset);
    return offset;
}