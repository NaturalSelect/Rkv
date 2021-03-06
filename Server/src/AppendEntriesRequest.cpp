#include <rkv/AppendEntriesRequest.hpp>

std::size_t rkv::AppendEntriesRequest::ComputeSize() const noexcept
{
    std::size_t size{0};
    size += Helper::ComputeSize(this->logs_);
    size += Helper::ComputeSize(this->leaderId_);
    sharpen::Varuint64 builder{this->leaderTerm_};
    size += Helper::ComputeSize(builder);
    builder.Set(this->prevLogIndex_);
    size += Helper::ComputeSize(builder);
    builder.Set(this->prevLogTerm_);
    size += Helper::ComputeSize(builder);
    builder.Set(this->commitIndex_);
    size += Helper::ComputeSize(builder);
    bool exist{this->group_.Exist()};
    size += Helper::ComputeSize(exist);
    if(exist)
    {
        builder.Set(this->group_.Get());
        size += Helper::ComputeSize(builder);
    }
    return size;
}

std::size_t rkv::AppendEntriesRequest::LoadFrom(const char *data,std::size_t size)
{
    if(size < 5 + Helper::ComputeSize(this->leaderId_))
    {
        throw std::invalid_argument("invalid append entires request buffer");
    }
    std::size_t offset{0};
    offset += Helper::LoadFrom(this->logs_,data,size);
    if(size <= offset)
    {
        throw sharpen::DataCorruptionException("append entires request corruption");
    }
    offset += Helper::LoadFrom(this->leaderId_,data + offset,size - offset);
    if(size <= offset)
    {
        throw sharpen::DataCorruptionException("append entires request corruption");
    }
    sharpen::Varuint64 builder{0};
    offset += Helper::LoadFrom(builder,data + offset,size - offset);
    this->leaderTerm_ = builder.Get();
    if(size <= offset)
    {
        throw sharpen::DataCorruptionException("append entires request corruption");
    }
    offset += Helper::LoadFrom(builder,data + offset,size - offset);
    this->prevLogIndex_ = builder.Get();
    if(size <= offset)
    {
        throw sharpen::DataCorruptionException("append entires request corruption");
    }
    offset += Helper::LoadFrom(builder,data + offset,size - offset);
    this->prevLogTerm_ = builder.Get();
    if(size <= offset)
    {
        throw sharpen::DataCorruptionException("append entires request corruption");
    }
    offset += Helper::LoadFrom(builder,data + offset,size - offset);
    this->commitIndex_ = builder.Get();
    if(size <= offset)
    {
        throw sharpen::DataCorruptionException("append entires request corruption");
    }
    bool exist;
    offset += Helper::LoadFrom(exist,data + offset,size - offset);
    if(exist)
    {
        if(size <= offset)
        {
            throw sharpen::DataCorruptionException("append entires request corruption");
        }
        offset += Helper::LoadFrom(builder,data + offset,size - offset);
        this->group_.Construct(builder.Get());
    }
    else
    {
        this->group_.Reset();
    }
    return offset;
}

std::size_t rkv::AppendEntriesRequest::UnsafeStoreTo(char *data) const noexcept
{
    std::size_t offset{0};
    offset += Helper::UnsafeStoreTo(this->logs_,data);
    offset += Helper::UnsafeStoreTo(this->leaderId_,data + offset);
    sharpen::Varuint64 builder{this->leaderTerm_};
    offset += Helper::UnsafeStoreTo(builder,data + offset);
    builder.Set(this->prevLogIndex_);
    offset += Helper::UnsafeStoreTo(builder,data + offset);
    builder.Set(this->prevLogTerm_);
    offset += Helper::UnsafeStoreTo(builder,data + offset);
    builder.Set(this->commitIndex_);
    offset += Helper::UnsafeStoreTo(builder,data + offset);
    bool exist{this->group_.Exist()};
    offset += Helper::UnsafeStoreTo(exist,data + offset);
    if(exist)
    {
        builder.Set(this->group_.Get());
        offset += Helper::UnsafeStoreTo(builder,data + offset);
    }
    return offset;
}