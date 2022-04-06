#include <rkv/RaftLog.hpp>

sharpen::Size rkv::RaftLog::ComputeSize() const noexcept
{
    sharpen::Size size{0};
    sharpen::Varuint64 builder{this->index_};
    size += Helper::ComputeSize(builder);
    builder.Set(this->term_);
    size += Helper::ComputeSize(builder);
    builder.Set(static_cast<sharpen::Uint64>(this->operation_));
    size += Helper::ComputeSize(builder);
    builder.Set(this->key_.GetSize());
    size += Helper::ComputeSize(builder);
    size += this->key_.GetSize();
    builder.Set(this->value_.GetSize());
    size += Helper::ComputeSize(builder);
    size += this->key_.GetSize();
    return size;
}

sharpen::Size rkv::RaftLog::LoadFrom(const char *data,sharpen::Size size)
{
    if(size < 5)
    {
        throw std::invalid_argument("invalid raft log buffer");
    }
    sharpen::Varuint64 builder{0};
    sharpen::Size offset{0};
    offset += Helper::LoadFrom(builder,data,size);
    this->index_ = builder.Get();
    if(size <= offset)
    {
        throw sharpen::DataCorruptionException("raft log corruption");
    }
    offset += Helper::LoadFrom(builder,data + offset,size - offset);
    this->term_ = builder.Get();
    if(size <= offset)
    {
        throw sharpen::DataCorruptionException("raft log corruption");
    }
    sharpen::Uint64 ope{0};
    offset += Helper::LoadFrom(builder,data + offset,size - offset);
    ope = builder.Get();
    this->operation_ = static_cast<Operation>(ope);
    if(size <= offset)
    {
        throw sharpen::DataCorruptionException("raft log corruption");
    }
    offset += Helper::LoadFrom(builder,data + offset,size - offset);
    sharpen::Size keySize{sharpen::IntCast<sharpen::Size>(builder.Get())};
    if(size <= offset + keySize)
    {
        throw sharpen::DataCorruptionException("raft log corruption");
    }
    sharpen::ByteBuffer key{keySize};
    std::memcpy(key.Data(),data + offset,keySize);
    offset += keySize;
    this->key_ = std::move(key);
    if(size <= offset)
    {
        throw sharpen::DataCorruptionException("raft log corruption");
    }
    offset += Helper::LoadFrom(builder,data + offset,size - offset);
    sharpen::Size valueSize{sharpen::IntCast<sharpen::Size>(builder.Get())};
    if(valueSize)
    {
        if(size < offset + valueSize)
        {
            throw sharpen::DataCorruptionException("raft log corruption");
        }
        sharpen::ByteBuffer value{valueSize};
        std::memcpy(value.Data(),data + offset,valueSize);
        offset += valueSize;
        this->value_ = std::move(value);
    }
    return offset;
}

sharpen::Size rkv::RaftLog::UnsafeStoreTo(char *data) const noexcept
{
    sharpen::Size offset{0};
    sharpen::Varuint64 builder{this->GetIndex()};
    offset += Helper::UnsafeStoreTo(builder,data);
    builder.Set(this->GetTerm());
    offset += Helper::UnsafeStoreTo(builder,data + offset);
    builder.Set(static_cast<sharpen::Uint64>(this->operation_));
    offset += Helper::UnsafeStoreTo(builder,data + offset);
    builder.Set(this->key_.GetSize());
    offset += Helper::UnsafeStoreTo(builder,data + offset);
    std::memcpy(data + offset,this->key_.Data(),this->key_.GetSize());
    offset += this->key_.GetSize();
    builder.Set(this->value_.GetSize());
    offset += Helper::UnsafeStoreTo(builder,data + offset);
    if(!this->value_.Empty())
    {
        std::memcpy(data + offset,this->value_.Data(),this->value_.GetSize());
        offset += this->value_.GetSize();
    }
    return offset;
}