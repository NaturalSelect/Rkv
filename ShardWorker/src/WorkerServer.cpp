#include <rkv/WorkerServer.hpp>

#include <sharpen/Quorum.hpp>
#include <sharpen/FileOps.hpp>

rkv::WorkerServer::WorkerServer(sharpen::EventEngine &engine,const rkv::WorkerServerOption &option)
    :TcpServer(sharpen::AddressFamily::Ip,option.BindEndpoint(),engine)
    ,selfId_(option.SelfId())
    ,app_(nullptr)
    ,client_(nullptr)
    ,groups_()
    ,counterFile_(nullptr)
{
    //make directories
    sharpen::MakeDirectory("./Storage");
    this->app_ = std::make_shared<rkv::KeyValueService>(engine,"./Storage/WorkerDb");
    //create master client
    assert(!option.MasterEmpty());
    this->client_.reset(new rkv::MasterClient{engine,option.MasterBegin(),option.MasterEnd(),std::chrono::milliseconds{1000},10});
    if(!this->client_)
    {
        throw std::bad_alloc();
    }
    //get migrations
    std::vector<rkv::Migration> migrations;
    this->client_->GetMigrations(std::back_inserter(migrations),this->selfId_);
    for (auto begin = migrations.begin(),end = migrations.end(); begin != end; ++begin)
    {
        this->ExecuteMigration(*begin);
    }
    //get shards
    std::vector<rkv::Shard> shards;
    this->client_->GetShard(std::back_inserter(shards),this->selfId_);
    for (auto begin = shards.begin(),end = shards.end(); begin != end; ++begin)
    {
        std::unique_ptr<rkv::RaftGroup> group{new rkv::RaftGroup{engine,this->selfId_,rkv::RaftStorage{engine,Self::FormatStorageName(begin->GetId())},this->app_}};
        if(!group)
        {
            throw std::bad_alloc();
        }
        std::uint64_t lastAppiled{group->Raft().GetLastApplied()};
        for(auto workerBegin = begin->Workers().begin(),workerEnd = begin->Workers().end(); workerBegin != workerEnd; ++workerBegin)
        {
            if(*workerBegin != this->selfId_)
            {
                rkv::RaftMember member{*workerBegin,engine};
                member.SetCurrentIndex(lastAppiled);
                group->Raft().Members().emplace(*workerBegin,std::move(member));
            }
        }
        this->groups_.emplace(begin->GetId(),std::move(group));
    }
    //get completed migrations
    this->counterFile_ = sharpen::MakeFileChannel("./Storage/CompletedCounter.bin",sharpen::FileAccessModel::All,sharpen::FileOpenModel::CreateOrOpen);
    this->counterFile_->Register(engine);
    std::uint64_t size{this->counterFile_->GetFileSize()};
    if(!size)
    {
        this->counterFile_->ZeroMemoryAsync(sizeof(size));
    }
    sharpen::FileMemory counterMemory{this->counterFile_->MapMemory(sizeof(size),0)};
    std::uint64_t *counter{reinterpret_cast<std::uint64_t*>(counterMemory.Get())};
    std::vector<rkv::CompletedMigration> completedMigations;
    std::uint64_t maxId{0};
    for (auto begin = completedMigations.begin(),end = completedMigations.end(); begin != end; ++begin)
    {
        auto ite = this->groups_.find(begin->GetDestination());
        if(ite == this->groups_.end())
        {
            this->CleaupCompletedMigration(*begin);
        }
    }
    *counter = maxId;
    counterMemory.FlushAndWait();
}

