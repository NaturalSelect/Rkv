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

static void StopServer(rkv::WorkerServer *server)
{
    assert(server != nullptr);
    server->Stop();
}

static void Entry()
{
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
    sharpen::StartupNetSupport();
    rkv::WorkerServer server{sharpen::EventEngine::GetEngine(),opt};
    std::puts("[Info]Server started");
    std::puts("[Info]Please use ctrl+c to stop server");
    sharpen::RegisterCtrlHandler(sharpen::CtrlType::Interrupt,std::bind(&StopServer,&server));
    try
    {
        server.RunAsync();
    }
    catch(const std::exception& e)
    {
        std::fprintf(stderr,"[Error]Server terminated because %s\n",e.what());
        sharpen::CleanupNetSupport();
        return;   
    }
    sharpen::CleanupNetSupport();
}

int main(int argc, char const *argv[])
{
    sharpen::EventEngine &engine = sharpen::EventEngine::SetupEngine();
    engine.Startup(&Entry);
    return 0;
}