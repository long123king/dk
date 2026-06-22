#pragma once

#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>

struct IDebugClient; // forward declaration - full type available in dk_server.cpp via CmdExt.h

class CDkEmbeddedServer
{
public:
    static CDkEmbeddedServer* Instance();

    bool Start(const std::string& host, uint16_t port, std::string& error_message);
    void Stop();

    // Called from serve_start (engine thread) to preserve the IDebugClient
    // reference and force CModelAccess singleton construction while m_Client is
    // valid.  Released in Stop() after the server thread has exited.
    void SetPreservedClient(IDebugClient* client);

    bool IsRunning() const;
    std::string GetBindHost() const;
    uint16_t GetBindPort() const;
    uint64_t GetUptimeMs() const;

private:
    CDkEmbeddedServer() = default;
    ~CDkEmbeddedServer();

    CDkEmbeddedServer(const CDkEmbeddedServer&) = delete;
    CDkEmbeddedServer& operator=(const CDkEmbeddedServer&) = delete;

    void ServerLoop();
    void HandleClientSocket(uintptr_t client_socket);
    std::string BuildHttpResponse(const std::string& method, const std::string& path, const std::string& query, const std::string& body);

    std::string BuildJsonResponseOk(const std::string& body_json) const;
    std::string BuildJsonResponseError(int status_code, const std::string& code, const std::string& message) const;

    std::string HandleServerStatusRoute();
    std::string HandleServerStopRoute();
    std::string HandleTraceInfoRoute();
    std::string HandleModulesRoute();
    std::string HandleThreadsRoute();
    std::string HandleLifetimeEventsRoute();
    std::string HandleRegistersRoute(const std::string& query);
    std::string HandleCallstackRoute(const std::string& query);
    std::string HandlePageRoute(const std::string& query);
    std::string HandlePageSvgRoute(const std::string& query);
    std::string HandleFunctionCallsRoute(const std::string& query);
    std::string HandleCommandRoute(const std::string& query, const std::string& body);
    std::string HandleModelRoute(const std::string& query);
    std::string HandlePeRoute(const std::string& query);
    std::string HandleStringsRoute(const std::string& query);
    std::string HandleMemoryLayoutRoute(const std::string& query);
    std::string HandleEnvironmentRoute(const std::string& query);
    std::string HandleCapabilitiesRoute();

private:
    mutable std::mutex m_mutex;
    std::atomic<bool> m_running{ false };
    IDebugClient* m_preserved_client{ nullptr };

    uintptr_t m_listen_socket{ static_cast<uintptr_t>(~0ULL) };

    std::string m_bind_host{ "127.0.0.1" };
    uint16_t m_bind_port{ 8080 };
    uint64_t m_start_tick_ms{ 0 };
    std::atomic<uint64_t> m_request_counter{ 0 };
};
