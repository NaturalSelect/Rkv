#include <rkv/AppendEntiresResponse.hpp>

std::size_t rkv::AppendEntiresResponse::ComputeSize() const noexcept
{
    std::size_t size{0};
    size += Helper::ComputeSize(this->success_);
    sharpen::Varuint64 builder{this->term_};
    size += Helper::ComputeSize(builder);
    builder.Set(this->appiledIndex_);
    size += Helper::ComputeSize(builder);
    return size;
}

std::size_t rkv::AppendEntiresResponse::LoadFrom(const char *data,std::size_t size)
{
    if(size < 3)
    {
        throw std::invalid_argument("invalid append entires response buffer");
    }
    std::size_t offset{0};
    offset += Helper::LoadFrom(this->success_,data,size);
    if (size <= offset)
    {
        throw sharpen::DataCorruptionException("append entires reponse corruption");   
    }
    sharpen::Varuint64 builder{0};
    offset += Helper::LoadFrom(builder,data + offset,size - offset);
    this->term_ = builder.Get();
    if (size <= offset)
    {
        throw sharpen::DataCorruptionException("append entires reponse corruption");   
    }
    offset += Helper::LoadFrom(builder,data + offset,size - offset);
    this->appiledIndex_ = builder.Get();
    return offset;
}

std::size_t rkv::AppendEntiresResponse::UnsafeStoreTo(char *data) const noexcept
{
    std::size_t offset{0};
    offset += Helper::UnsafeStoreTo(this->success_,data);
    sharpen::Varuint64 builder{this->term_};
    offset += Helper::UnsafeStoreTo(builder,data + offset);
    builder.Set(this->appiledIndex_);
    offset += Helper::UnsafeStoreTo(builder,data + offset);
    return offset;
}