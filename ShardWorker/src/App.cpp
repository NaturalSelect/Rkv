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
    sharpen::CleanupNetSupport();
}

int main(int argc, char const *argv[])
{
    sharpen::EventEngine &engine = sharpen::EventEngine::SetupEngine();
    engine.Startup(&Entry);
    return 0;
}