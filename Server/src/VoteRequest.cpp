#include <rkv/VoteRequest.hpp>

std::size_t rkv::VoteRequest::ComputeSize() const noexcept
{
    std::size_t size{0};
    size += Helper::ComputeSize(this->id_);
    sharpen::Varuint64 builder{this->term_};
    size += Helper::ComputeSize(builder);
    builder.Set(this->lastIndex_);
    size += Helper::ComputeSize(builder);
    builder.Set(this->lastTerm_);
    size += Helper::ComputeSize(builder);
    return size;
}

std::size_t rkv::VoteRequest::LoadFrom(const char *data,std::size_t size)
{
    if(size < 3 + Helper::ComputeSize(this->id_))
    {
        throw std::invalid_argument("invalid vote request buffer");
    }
    std::size_t offset{0};
    offset += Helper::LoadFrom(this->id_,data,size);
    if(size <= offset)
    {
        throw sharpen::DataCorruptionException("vote request corruption");
    }
    sharpen::Varuint64 builder{0};
    offset += Helper::LoadFrom(builder,data + offset,size - offset);
    this->term_ = builder.Get();
    if(size <= offset)
    {
        throw sharpen::DataCorruptionException("vote request corruption");
    }
    offset += Helper::LoadFrom(builder,data + offset,size - offset);
    this->lastIndex_ = builder.Get();
    if(size <= offset)
    {
        throw sharpen::DataCorruptionException("vote request corruption");
    }
    offset += Helper::LoadFrom(builder,data + offset,size - offset);
    this->lastTerm_ = builder.Get();
    return offset;
}

std::size_t rkv::VoteRequest::UnsafeStoreTo(char *data) const noexcept
{
    std::size_t offset{0};
    offset += Helper::UnsafeStoreTo(this->id_,data);
    sharpen::Varuint64 builder{this->term_};
    offset += Helper::UnsafeStoreTo(builder,data + offset);
    builder.Set(this->lastIndex_);
    offset += Helper::UnsafeStoreTo(builder,data + offset);
    builder.Set(this->lastTerm_);
    offset += Helper::UnsafeStoreTo(builder,data + offset);
    return offset;
}