#include "dk_server_cmd.h"
#include "model.h"

#include "CmdExt.h"
#include "CmdList.h"
#include "dk_server.h"

#include <cstdint>
#include <sstream>
#include <string>

namespace
{
    uint16_t ParsePortOrDefault(const std::string& value, uint16_t default_port)
    {
        if (value.empty())
            return default_port;

        try
        {
            auto parsed = static_cast<uint32_t>(std::stoul(value));
            if (parsed == 0 || parsed > 65535)
                return default_port;
            return static_cast<uint16_t>(parsed);
        }
        catch (...)
        {
            return default_port;
        }
    }
}

DEFINE_CMD(serve_start)
{
    std::string host = "127.0.0.1";
    uint16_t port = 8080;

    if (args.size() >= 2)
        host = args[1];
    if (args.size() >= 3)
        port = ParsePortOrDefault(args[2], 8080);

    // Force CModelAccess singleton construction on the engine thread while
    // m_Client is valid. In blocking mode, request handling stays on this
    // command thread for the full serve session.
    try { (void)DK_MODEL_ACCESS->isTTD(); } catch (...) {}

    // Preserve a direct IDebugClient* reference so the underlying COM session
    // object stays alive for the duration of the server.  Released in Stop().
    IDebugClient* raw_client = nullptr;
    (void)EXT_D_IDebugClient->QueryInterface(IID_IDebugClient,
                                             reinterpret_cast<void**>(&raw_client));
    CDkEmbeddedServer::Instance()->SetPreservedClient(raw_client);

    std::string resolved_host = host;
    if (resolved_host.empty() || resolved_host == "localhost")
        resolved_host = "127.0.0.1";

    EXT_F_OUT("dk server (blocking mode) starting at http://%s:%u\n", resolved_host.c_str(), port);
    EXT_F_OUT("Use GET /api/server/stop to stop serving and return control to WinDbg.\n");

    std::string error_message;
    bool ok = CDkEmbeddedServer::Instance()->Start(host, port, error_message);
    if (!ok)
    {
        CDkEmbeddedServer::Instance()->SetPreservedClient(nullptr);
        EXT_F_ERR("Failed to start server: %s\n", error_message.c_str());
        return;
    }

    EXT_F_OUT("dk server stopped (blocking mode ended), control returned to WinDbg\n");
}

DEFINE_CMD(serve_stop)
{
    EXT_F_OUT("!dk serve_stop is disabled in blocking serve mode. Use GET /api/server/stop instead.\n");
}

DEFINE_CMD(serve_status)
{
    EXT_F_OUT("!dk serve_status is disabled in blocking serve mode. Use GET /api/server/status instead.\n");
}
