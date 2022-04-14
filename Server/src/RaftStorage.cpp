#include <rkv/RaftStorage.hpp>

sharpen::ByteBuffer rkv::RaftStorage::countkey_;

sharpen::ByteBuffer rkv::RaftStorage::voteKey_;

sharpen::ByteBuffer rkv::RaftStorage::termKey_;

sharpen::ByteBuffer rkv::RaftStorage::appiledKey_;

std::once_flag rkv::RaftStorage::flag_;

sharpen::ByteBuffer rkv::RaftStorage::FormatLogKey(std::uint64_t index)
{
    sharpen::ByteBuffer key{sizeof(index)};
    key.As<std::uint64_t>() = index;
    return key;
}

void rkv::RaftStorage::InitKeys()
{
    char countKey[] = "sz";
    Self::countkey_.ExtendTo(sizeof(countKey) - 1);
    std::memcpy(Self::countkey_.Data(),countKey,sizeof(countKey) - 1);
    char voteKey[] = "vt";
    Self::voteKey_.ExtendTo(sizeof(voteKey) - 1);
    std::memcpy(Self::voteKey_.Data(),voteKey,sizeof(voteKey) - 1);
    char termKey[] = "tr";
    Self::termKey_.ExtendTo(sizeof(termKey) - 1);
    std::memcpy(Self::termKey_.Data(),termKey,sizeof(termKey) - 1);
    char appiledKey[] = "ap";
    Self::appiledKey_.ExtendTo(sizeof(appiledKey) - 1);
    std::memcpy(Self::appiledKey_.Data(),appiledKey,sizeof(appiledKey) - 1);
}

std::uint64_t rkv::RaftStorage::GetLogsCount() const
{
    sharpen::Optional<sharpen::ByteBuffer> value{this->table_->TryGet(Self::countkey_)};
    if(value.Exist())
    {
        return value.Get().As<std::uint64_t>();
    }
    return 0;
}

bool rkv::RaftStorage::EmptyLogs() const
{
    return !this->GetLogsCount();
}

bool rkv::RaftStorage::ContainLog(std::uint64_t index) const
{
    return this->GetLogsCount() >= index;
}

rkv::RaftLog rkv::RaftStorage::GetLog(std::uint64_t index) const
{
    sharpen::ByteBuffer val{this->table_->Get(this->FormatLogKey(index))};
    rkv::RaftLog log;
    log.Unserialize().LoadFrom(val);
    return log;
}

bool rkv::RaftStorage::CheckLog(std::uint64_t index,std::uint64_t term) const
{
    if(!this->ContainLog(index))
    {
        return false;
    }
    return this->GetLog(index).GetTerm() == term;
}

void rkv::RaftStorage::AppendLog(const rkv::RaftLog &log)
{
    std::uint64_t count{this->GetLogsCount()};
    count += 1;
    if(count != log.GetIndex())
    {
        throw std::invalid_argument("invalid log index");
    }
    sharpen::ByteBuffer key{this->FormatLogKey(count)};
    sharpen::ByteBuffer val;
    sharpen::ByteBuffer countBuf{sizeof(count)};
    countBuf.As<std::uint64_t>() = count;
    log.Serialize().StoreTo(val);
    this->table_->Put(std::move(key),std::move(val));
    this->table_->Put(this->countkey_,std::move(countBuf));
}

void rkv::RaftStorage::SetVotedFor(const sharpen::IpEndPoint &id)
{
    sharpen::ByteBuffer buf;
    id.StoreTo(buf);
    this->table_->Put(this->voteKey_,std::move(buf));
}

void rkv::RaftStorage::SetCurrentTerm(std::uint64_t term)
{
    sharpen::ByteBuffer buf{sizeof(term)};
    buf.As<std::uint64_t>() = term;
    this->table_->Put(this->termKey_,std::move(buf));
}

std::uint64_t rkv::RaftStorage::GetCurrentTerm() const
{
    sharpen::Optional<sharpen::ByteBuffer> buf{this->table_->TryGet(this->termKey_)};
    if(!buf.Exist())
    {
        return 0;
    }
    return buf.Get().As<std::uint64_t>();
}

sharpen::IpEndPoint rkv::RaftStorage::GetVotedFor() const
{
    sharpen::IpEndPoint id;
    sharpen::ByteBuffer buf{this->table_->Get(this->voteKey_)};
    id.LoadFrom(buf);
    return id;
}

void rkv::RaftStorage::IncreaseCurrentTerm()
{
    sharpen::Optional<sharpen::ByteBuffer> buf{this->table_->TryGet(this->termKey_)};
    if(!buf.Exist())
    {
        buf.Construct();
        buf.Get().ExtendTo(sizeof(std::uint64_t));
        buf.Get().As<std::uint64_t>() = 1;
    }
    else
    {
        buf.Get().As<std::uint64_t>() += 1;
    }
    this->table_->Put(this->termKey_,std::move(buf.Get()));
}

bool rkv::RaftStorage::IsVotedFor() const
{
    return this->table_->Exist(this->voteKey_) == sharpen::ExistStatus::Exist;
}

void rkv::RaftStorage::RemoveLogsAfter(std::uint64_t index)
{
    assert(index != 0);
    sharpen::ByteBuffer buf{sizeof(index)};
    buf.As<std::uint64_t>() = index - 1;
    this->table_->Put(this->countkey_,std::move(buf));
}

rkv::RaftLog rkv::RaftStorage::GetLastLog() const
{
    std::uint64_t count{this->GetLogsCount()};
    if(!count)
    {
        throw std::out_of_range("logs is empty");
    }
    sharpen::ByteBuffer key{this->FormatLogKey(count)};
    sharpen::ByteBuffer value{this->table_->Get(key)};
    rkv::RaftLog log;
    log.Unserialize().LoadFrom(value);
    return log;
}

void rkv::RaftStorage::ResetVotedFor()
{
    this->table_->Delete(this->voteKey_);
}

std::uint64_t rkv::RaftStorage::GetLastLogIndex() const
{
    if(this->EmptyLogs())
    {
        return 0;
    }
    return this->GetLastLog().GetIndex();
}

std::uint64_t rkv::RaftStorage::GetLastAppiledIndex() const
{
    sharpen::Optional<sharpen::ByteBuffer> buf{this->table_->TryGet(this->appiledKey_)};
    if(!buf.Exist())
    {
        return 0;
    }
    return buf.Get().As<std::uint64_t>();
}

void rkv::RaftStorage::SetLastAppiledIndex(std::uint64_t index)
{
    sharpen::ByteBuffer buf{sizeof(index)};
    buf.As<std::uint64_t>() = index;
    this->table_->Put(this->appiledKey_,std::move(buf));
}