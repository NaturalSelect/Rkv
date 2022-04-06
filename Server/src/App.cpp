#include <cstdio>
#include <sharpen/EventEngine.hpp>
#include <sharpen/INetStreamChannel.hpp>
#include <sharpen/RaftWrapper.hpp>
#include <sharpen/FileOps.hpp>
#include <rkv/RaftServer.hpp>

template<typename _InserterIterator,typename _Check = decltype(*std::declval<_InserterIterator&>()++ = std::declval<sharpen::IpEndPoint&>())>
inline void ReadMembersConfig(_InserterIterator inserter,const char *filename)
{
    sharpen::FileChannelPtr config = sharpen::MakeFileChannel(filename,sharpen::FileAccessModel::Read,sharpen::FileOpenModel::CreateOrOpen);
    config->Register(sharpen::EventEngine::GetEngine());
    std::uint64_t size{config->GetFileSize()};
    if(!size)
    {
        std::printf("[Info]Please edit members config file %s\n",filename);
        return;
    }
    sharpen::ByteBuffer buf{sharpen::IntCast<sharpen::Size>(size)};
    config->ReadAsync(buf,0);
    std::size_t sz{0};
    
}

static void Entry()
{
    sharpen::StartupNetSupport();
    std::puts("start server");
    //cretae config directory
    sharpen::MakeDirectory("./Config");
    //read config
    sharpen::IpEndPoint id;
    std::vector<sharpen::IpEndPoint> members{2};
    rkv::RaftServerOption opt{id,members.begin(),members.end()};
    std::puts("[Info]Start Server");
    rkv::RaftServer server{sharpen::EventEngine::GetEngine(),opt};
    server.RunAsync();
    //ReadMembersConfig(std::back_inserter(members),"./config/member.txt");
    std::puts("stop server");
    sharpen::CleanupNetSupport();
}

int main(int argc, char const *argv[])
{
    sharpen::EventEngine &engine = sharpen::EventEngine::SetupSingleThreadEngine();
    engine.Startup(&Entry);
    return 0;
}