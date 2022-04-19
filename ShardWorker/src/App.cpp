#include <cstdio>
#include <sharpen/EventEngine.hpp>
#include <sharpen/INetStreamChannel.hpp>
#include <sharpen/RaftWrapper.hpp>
#include <sharpen/FileOps.hpp>
#include <sharpen/CtrlHandler.hpp>
#include <sharpen/Converter.hpp>
#include <rkv/Utility.hpp>
#include <rkv/MasterClient.hpp>
#include <rkv/WorkerServer.hpp>

// static void StopServer(rkv::KvServer *server)
// {
//     assert(server != nullptr);
//     server->Stop();
// }

static void Entry()
{
    
    sharpen::StartupNetSupport();
    const char *bindName = "./Config/Bind.txt";
    const char *idName = "./Config/Id.txt";
    const char *masterName = "./Config/Masters.txt";
    std::puts("[Info]Starting worker server");
    //cretae config directory
    std::puts("[Info]Reading configurations");
    sharpen::MakeDirectory("./Config");
    sharpen::IpEndPoint bind;
    sharpen::IpEndPoint id;
    std::vector<sharpen::IpEndPoint> masters;
    std::vector<std::string> lines;
    rkv::ReadAllLines(sharpen::EventEngine::GetEngine(),std::back_inserter(lines),bindName);
    if (lines.empty())
    {
        std::fprintf(stderr,"[Error]Please edit %s to set server bind endpoint(ip port)\n",bindName);
        return;
    }
    std::string first{std::move(lines.front())};
    try
    {
        sharpen::IpEndPoint tmp{rkv::ConvertStringToEndPoint(first)};
        bind = std::move(tmp);
    }
    catch(const std::exception& e)
    {
        std::fprintf(stderr,"[Error]Connot parse bind configuration %s because %s\n",first.c_str(),e.what());
        return;
    }
    lines.clear();
    rkv::ReadAllLines(sharpen::EventEngine::GetEngine(),std::back_inserter(lines),idName);
    if(lines.empty())
    {
        std::fprintf(stderr,"[Error]Please edit %s to set server id(ip port)\n",idName);
        return;
    }
    first = std::move(lines.front());
    try
    {
        sharpen::IpEndPoint tmp{rkv::ConvertStringToEndPoint(first)};
        id = std::move(tmp);
    }
    catch(const std::exception& e)
    {
        std::fprintf(stderr,"[Error]Connot parse id configuration %s because %s\n",first.c_str(),e.what());
        return;   
    }
    lines.clear();
    rkv::ReadAllLines(sharpen::EventEngine::GetEngine(),std::back_inserter(lines),masterName);
    if(lines.empty())
    {
        std::fprintf(stderr,"[Error]Please edit %s to set master members\n",masterName);
        return;
    }
    std::puts("[Info]Master configurations are");
    for (auto begin = lines.begin(),end = lines.end(); begin != end; ++begin)
    {
        std::printf("\t%s\n",begin->c_str());
    }
    for (auto begin = lines.begin(),end = lines.end(); begin != end; ++begin)
    {
        try
        {
            sharpen::IpEndPoint ep{rkv::ConvertStringToEndPoint(*begin)};
            masters.emplace_back(std::move(ep));
        }
        catch(const std::exception& e)
        {
            std::fprintf(stderr,"[Error]Cannot parse master configuration line %s because %s\n",begin->data(),e.what());
            return;
        }
    }
    lines.clear();
    rkv::WorkerServerOption opt{bind,id,masters.begin(),masters.end()};
    rkv::WorkerServer server{sharpen::EventEngine::GetEngine(),opt};
    server.RunAsync();
    // std::vector<sharpen::IpEndPoint> members;
    // {
    //     sharpen::IpEndPoint ep;
    //     ep.SetAddrByString("127.0.0.1");
    //     for (size_t i = 0; i != 3; ++i)
    //     {
    //         ep.SetPort(8080 + i);   
    //         members.emplace_back(ep);
    //     }
    // }
    // sharpen::ByteBuffer key{};
    // rkv::MasterClient client{sharpen::EventEngine::GetEngine(),members.begin(),members.end(),std::chrono::seconds{1},10};
    // std::vector<rkv::Shard> shards;
    // sharpen::IpEndPoint id;
    // id.SetAddrByString("127.0.0.1");
    // id.SetPort(8083);
    // client.GetShard(std::back_inserter(shards),id);
    // sharpen::ByteBuffer beginKey{"1021",5};
    // sharpen::ByteBuffer endKey{"10022",5};
    //auto r = client.DeriveShard(0,beginKey,endKey);
    sharpen::CleanupNetSupport();
}

int main(int argc, char const *argv[])
{
    sharpen::EventEngine &engine = sharpen::EventEngine::SetupEngine();
    engine.Startup(&Entry);
    return 0;
}