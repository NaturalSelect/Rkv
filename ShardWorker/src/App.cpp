#include <cstdio>
#include <sharpen/EventEngine.hpp>
#include <sharpen/INetStreamChannel.hpp>
#include <sharpen/RaftWrapper.hpp>
#include <sharpen/FileOps.hpp>
#include <sharpen/CtrlHandler.hpp>
#include <sharpen/Converter.hpp>
#include <rkv/Utility.hpp>
#include <rkv/MasterClient.hpp>

// static void StopServer(rkv::KvServer *server)
// {
//     assert(server != nullptr);
//     server->Stop();
// }

static void Entry()
{
    
    sharpen::StartupNetSupport();
    
    //get migrations
    //do migrations
    //get shards
    //get completed migrations
    //cleanup data
    //start server
    std::vector<sharpen::IpEndPoint> members;
    {
        sharpen::IpEndPoint ep;
        ep.SetAddrByString("127.0.0.1");
        for (size_t i = 0; i != 3; ++i)
        {
            ep.SetPort(8080 + i);   
            members.emplace_back(ep);
        }
    }
    sharpen::ByteBuffer key{};
    rkv::MasterClient client{sharpen::EventEngine::GetEngine(),members.begin(),members.end(),std::chrono::seconds{1},10};
    std::vector<rkv::Shard> shards;
    sharpen::IpEndPoint id;
    id.SetAddrByString("127.0.0.1");
    id.SetPort(8083);
    //client.GetShard(std::back_inserter(shards),id);
    sharpen::ByteBuffer beginKey{"1021",5};
    sharpen::ByteBuffer endKey{"10022",5};
    auto r = client.DeriveShard(0,beginKey,endKey);
    sharpen::CleanupNetSupport();
}

int main(int argc, char const *argv[])
{
    sharpen::EventEngine &engine = sharpen::EventEngine::SetupEngine();
    engine.Startup(&Entry);
    return 0;
}