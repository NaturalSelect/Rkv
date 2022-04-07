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

static sharpen::Optional<sharpen::IpEndPoint> RedirectLeader(sharpen::NetStreamChannelPtr channel)
{
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::LeaderRedirectRequest,0)};
    WriteMessage(channel,header);
    sharpen::ByteBuffer buf;
    ReadMessage(channel,rkv::MessageType::LeaderRedirectResponse,buf);
    rkv::LeaderRedirectResponse response;
    response.Unserialize().LoadFrom(buf);
    if(response.KnowLeader())
    {
        char ip[21] = {};
        response.Endpoint().GetAddrString(ip,sizeof(ip));
        std::printf("[Info]Got leader id %s:%hu\n",ip,response.Endpoint().GetPort());
        return response.Endpoint();
    }
    std::puts("[Info]Cannot get leader id");
    return sharpen::EmptyOpt;
}

static sharpen::ByteBuffer GetValue(sharpen::NetStreamChannelPtr channel,sharpen::ByteBuffer key)
{
    rkv::GetRequest request;
    request.Key() = std::move(key);
    sharpen::ByteBuffer buf;
    request.Serialize().StoreTo(buf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::GetRequest,buf.GetSize())};
    WriteMessage(channel,header,buf);
    ReadMessage(channel,rkv::MessageType::GetResponse,buf);
    rkv::GetResponse response;
    response.Unserialize().LoadFrom(buf);
    return response.Value();
}

static bool PutKeyValue(sharpen::NetStreamChannelPtr channel,sharpen::ByteBuffer key,sharpen::ByteBuffer value)
{
    rkv::PutRequest request;
    request.Key() = std::move(key);
    request.Value() = std::move(value);
    sharpen::ByteBuffer buf;
    request.Serialize().StoreTo(buf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::PutRequest,buf.GetSize())};
    WriteMessage(channel,header,buf);
    ReadMessage(channel,rkv::MessageType::PutResponse,buf);
    rkv::PutResponse response;
    response.Unserialize().LoadFrom(buf);
    return response.Success();
}

static bool DeleteKey(sharpen::NetStreamChannelPtr channel,sharpen::ByteBuffer key)
{
    rkv::DeleteRequest request;
    request.Key() = std::move(key);
    sharpen::ByteBuffer buf;
    request.Serialize().StoreTo(buf);
    rkv::MessageHeader header{rkv::MakeMessageHeader(rkv::MessageType::DeleteReqeust,buf.GetSize())};
    WriteMessage(channel,header,buf);
    ReadMessage(channel,rkv::MessageType::DeleteResponse,buf);
    rkv::DeleteResponse response;
    response.Unserialize().LoadFrom(buf);
    return response.Success();
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