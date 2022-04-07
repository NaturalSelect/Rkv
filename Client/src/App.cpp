#include <cstdio>
#include <sharpen/EventEngine.hpp>
#include <sharpen/INetStreamChannel.hpp>
#include <sharpen/IpEndPoint.hpp>
#include <sharpen/Converter.hpp>
#include <rkv/PutRequest.hpp>
#include <rkv/PutResponse.hpp>
#include <rkv/GetRequest.hpp>
#include <rkv/GetResponse.hpp>
#include <rkv/DeleteRequest.hpp>
#include <rkv/DeleteResponse.hpp>
#include <rkv/LeaderRedirectResponse.hpp>
#include <rkv/MessageHeader.hpp>

static sharpen::NetStreamChannelPtr ConnectTo(const char *ip,std::uint16_t port)
{
    sharpen::IpEndPoint ep{0,0};
    sharpen::NetStreamChannelPtr channel = sharpen::MakeTcpStreamChannel(sharpen::AddressFamily::Ip);
    channel->Bind(ep);
    channel->Register(sharpen::EventEngine::GetEngine());
    ep.SetAddrByString(ip);
    ep.SetPort(port);
    channel->ConnectAsync(ep);
    return channel;
}

static void WriteMessage(sharpen::NetStreamChannelPtr channel,const rkv::MessageHeader &header)
{
    std::size_t sz{channel->WriteObjectAsync(header)};
    if(sz != sizeof(header))
    {
        sharpen::ThrowSystemError(sharpen::ErrorConnectReset);
    }
}

static void WriteMessage(sharpen::NetStreamChannelPtr channel,const rkv::MessageHeader &header,const sharpen::ByteBuffer &request)
{
    WriteMessage(channel,header);
    std::size_t sz{channel->WriteAsync(request)};
    if(sz != request.GetSize())
    {
        sharpen::ThrowSystemError(sharpen::ErrorConnectReset);
    }
}

static void ReadMessage(sharpen::NetStreamChannelPtr channel,rkv::MessageType expectedType,sharpen::ByteBuffer &response)
{
    rkv::MessageHeader header;
    std::size_t sz{channel->ReadObjectAsync(header)};
    if(sz != sizeof(header))
    {
        sharpen::ThrowSystemError(sharpen::ErrorConnectReset);
    }
    if(rkv::GetMessageType(header) != expectedType)
    {
        throw std::logic_error("got unexpected response");
    }
    response.ExtendTo(sharpen::IntCast<std::size_t>(header.size_));
    sz = channel->ReadFixedAsync(response);
    if(sz != response.GetSize())
    {
        sharpen::ThrowSystemError(sharpen::ErrorConnectReset);
    }
}

static void Entry()
{
    sharpen::StartupNetSupport();
    sharpen::CleanupNetSupport();
}

int main(int argc, char const *argv[])
{
    sharpen::EventEngine &engine = sharpen::EventEngine::SetupSingleThreadEngine();
    engine.Startup(&Entry);
    return 0;
}