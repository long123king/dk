#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Unknwn.h>
#include <oaidl.h>

#include "model.h"
#include "dk_server.h"

#include "CmdExt.h"
#include "MemoryAnalyzer.h"
#include "memory.h"
#include "json/json.hpp"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <iomanip>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#pragma comment(lib, "Ws2_32.lib")

// Prevent Windows min/max macros from conflicting with std::min / std::max
#undef min
#undef max

using json = nlohmann::json;

namespace
{
    struct ParsedRequestLine
    {
        std::string method;
        std::string path;
        std::string query;
    };

    uint64_t NowMs()
    {
        auto now = std::chrono::steady_clock::now().time_since_epoch();
        return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
    }

    ParsedRequestLine ParseRequestLine(const std::string& request)
    {
        auto first_line_end = request.find("\r\n");
        auto first_line = first_line_end == std::string::npos ? request : request.substr(0, first_line_end);

        std::stringstream ss(first_line);
        std::string method;
        std::string path;
        std::string version;
        ss >> method >> path >> version;

        if (method.empty() || path.empty())
            return { "", "", "" };

        std::string query;
        auto q_pos = path.find('?');
        if (q_pos != std::string::npos)
        {
            query = path.substr(q_pos + 1);
            path = path.substr(0, q_pos);
        }

        return { method, path, query };
    }

    int HexDigitValue(char c)
    {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return 10 + (c - 'a');
        if (c >= 'A' && c <= 'F') return 10 + (c - 'A');
        return -1;
    }

    std::string UrlDecode(const std::string& s)
    {
        std::string out;
        out.reserve(s.size());
        for (size_t i = 0; i < s.size(); ++i)
        {
            if (s[i] == '%' && i + 2 < s.size())
            {
                int hi = HexDigitValue(s[i + 1]);
                int lo = HexDigitValue(s[i + 2]);
                if (hi >= 0 && lo >= 0)
                {
                    out.push_back(static_cast<char>((hi << 4) | lo));
                    i += 2;
                    continue;
                }
            }

            if (s[i] == '+')
                out.push_back(' ');
            else
                out.push_back(s[i]);
        }
        return out;
    }

    std::map<std::string, std::string> ParseQueryParams(const std::string& query)
    {
        std::map<std::string, std::string> params;
        if (query.empty())
            return params;

        size_t start = 0;
        while (start <= query.size())
        {
            size_t amp = query.find('&', start);
            std::string token = (amp == std::string::npos)
                ? query.substr(start)
                : query.substr(start, amp - start);

            if (!token.empty())
            {
                size_t eq = token.find('=');
                std::string key = UrlDecode(eq == std::string::npos ? token : token.substr(0, eq));
                std::string value = UrlDecode(eq == std::string::npos ? "" : token.substr(eq + 1));
                if (!key.empty())
                    params[key] = value;
            }

            if (amp == std::string::npos)
                break;
            start = amp + 1;
        }

        return params;
    }

    bool TryParseU64(const std::string& value, uint64_t& out)
    {
        if (value.empty())
            return false;

        char* end_ptr = nullptr;
        unsigned long long parsed = std::strtoull(value.c_str(), &end_ptr, 0);
        if (end_ptr == value.c_str() || (end_ptr != nullptr && *end_ptr != '\0'))
            return false;

        out = static_cast<uint64_t>(parsed);
        return true;
    }

    bool TryParseDouble(const std::string& value, double& out)
    {
        if (value.empty())
            return false;

        char* end_ptr = nullptr;
        double parsed = std::strtod(value.c_str(), &end_ptr);
        if (end_ptr == value.c_str() || (end_ptr != nullptr && *end_ptr != '\0'))
            return false;

        out = parsed;
        return true;
    }

    std::string ToLowerAscii(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(),
                       [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        return value;
    }

    std::string Basename(const std::string& path)
    {
        if (path.empty())
            return "";

        auto pos = path.find_last_of("/\\");
        if (pos == std::string::npos)
            return path;
        if (pos + 1 >= path.size())
            return "";
        return path.substr(pos + 1);
    }

    std::string BuildModuleId(uint64_t base_address, uint64_t image_size, const std::string& module_name)
    {
        std::stringstream ss;
        ss << std::hex << base_address << ":" << image_size << ":" << module_name;
        return ss.str();
    }

    bool IsZeroPos(const std::tuple<uint64_t, uint64_t>& pos)
    {
        return std::get<0>(pos) == 0 && std::get<1>(pos) == 0;
    }

    bool ContainsCaseInsensitive(const std::string& value, const std::string& token)
    {
        std::string lowered_value = ToLowerAscii(value);
        std::string lowered_token = ToLowerAscii(token);
        return lowered_value.find(lowered_token) != std::string::npos;
    }

    std::string ClassifyLifetimeEventType(const std::string& event_type)
    {
        bool is_module_event = ContainsCaseInsensitive(event_type, "module");
        bool is_thread_event = ContainsCaseInsensitive(event_type, "thread");

        if (is_module_event)
        {
            if (ContainsCaseInsensitive(event_type, "unload") || ContainsCaseInsensitive(event_type, "unloaded"))
                return "moduleUnload";
            if (ContainsCaseInsensitive(event_type, "load") || ContainsCaseInsensitive(event_type, "loaded"))
                return "moduleLoad";
        }

        if (is_thread_event)
        {
            if (ContainsCaseInsensitive(event_type, "terminate") || ContainsCaseInsensitive(event_type, "terminated") || ContainsCaseInsensitive(event_type, "exit"))
                return "threadTerminate";
            if (ContainsCaseInsensitive(event_type, "create") || ContainsCaseInsensitive(event_type, "created") || ContainsCaseInsensitive(event_type, "start"))
                return "threadCreate";
        }

        return "";
    }

    json PosToJson(const std::tuple<uint64_t, uint64_t>& pos)
    {
        return {
            {"major", std::to_string(std::get<0>(pos))},
            {"minor", std::get<1>(pos)}
        };
    }
}

CDkEmbeddedServer* CDkEmbeddedServer::Instance()
{
    static CDkEmbeddedServer s_instance;
    return &s_instance;
}

CDkEmbeddedServer::~CDkEmbeddedServer()
{
    Stop();
}

bool CDkEmbeddedServer::Start(const std::string& host, uint16_t port, std::string& error_message)
{
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        if (m_running)
        {
            error_message = "server is already running";
            return false;
        }
    }

    WSADATA wsa_data;
    int wsa_result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (wsa_result != 0)
    {
        error_message = "WSAStartup failed with code " + std::to_string(wsa_result);
        return false;
    }

    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == INVALID_SOCKET)
    {
        error_message = "socket() failed";
        WSACleanup();
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    std::string resolved_host = host.empty() ? "127.0.0.1" : host;
    if (resolved_host == "localhost")
        resolved_host = "127.0.0.1";

    int pton_result = inet_pton(AF_INET, resolved_host.c_str(), &addr.sin_addr);
    if (pton_result != 1)
    {
        closesocket(listen_socket);
        WSACleanup();
        error_message = "Invalid IPv4 host: " + resolved_host;
        return false;
    }

    int reuse_addr = 1;
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&reuse_addr), sizeof(reuse_addr));

    int bind_result = bind(listen_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
    if (bind_result == SOCKET_ERROR)
    {
        error_message = "bind() failed";
        closesocket(listen_socket);
        WSACleanup();
        return false;
    }

    int listen_result = listen(listen_socket, SOMAXCONN);
    if (listen_result == SOCKET_ERROR)
    {
        error_message = "listen() failed";
        closesocket(listen_socket);
        WSACleanup();
        return false;
    }

    {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_bind_host = resolved_host;
        m_bind_port = port;
        m_listen_socket = static_cast<uintptr_t>(listen_socket);
        m_start_tick_ms = NowMs();
        m_running = true;
    }

    ServerLoop();

    {
        std::lock_guard<std::mutex> guard(m_mutex);
        if (m_listen_socket != static_cast<uintptr_t>(~0ULL))
        {
            SOCKET active_socket = static_cast<SOCKET>(m_listen_socket);
            closesocket(active_socket);
            m_listen_socket = static_cast<uintptr_t>(~0ULL);
        }
        m_running = false;
    }

    WSACleanup();

    if (m_preserved_client)
    {
        m_preserved_client->Release();
        m_preserved_client = nullptr;
    }

    return true;
}

void CDkEmbeddedServer::Stop()
{
    bool was_running = false;
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        was_running = m_running;

        m_running = false;

        if (m_listen_socket != static_cast<uintptr_t>(~0ULL))
        {
            SOCKET listen_socket = static_cast<SOCKET>(m_listen_socket);
            closesocket(listen_socket);
            m_listen_socket = static_cast<uintptr_t>(~0ULL);
        }
    }

    if (was_running)
        WSACleanup();

    // Always release preserved client on Stop(), including failure paths where
    // Start() returned before entering serving loop.
    if (m_preserved_client)
    {
        m_preserved_client->Release();
        m_preserved_client = nullptr;
    }
}

void CDkEmbeddedServer::SetPreservedClient(IDebugClient* client)
{
    // The caller obtained this pointer via QueryInterface (already AddRef'd).
    // We take ownership here; Release() is called in Stop().
    if (m_preserved_client != nullptr && m_preserved_client != client)
    {
        m_preserved_client->Release();
    }
    m_preserved_client = client;
}

bool CDkEmbeddedServer::IsRunning() const
{
    return m_running.load();
}

std::string CDkEmbeddedServer::GetBindHost() const
{
    std::lock_guard<std::mutex> guard(m_mutex);
    return m_bind_host;
}

uint16_t CDkEmbeddedServer::GetBindPort() const
{
    std::lock_guard<std::mutex> guard(m_mutex);
    return m_bind_port;
}

uint64_t CDkEmbeddedServer::GetUptimeMs() const
{
    if (!m_running)
        return 0;

    uint64_t start = 0;
    {
        std::lock_guard<std::mutex> guard(m_mutex);
        start = m_start_tick_ms;
    }
    return start == 0 ? 0 : (NowMs() - start);
}

void CDkEmbeddedServer::ServerLoop()
{
    HRESULT coinit_hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    bool should_uninit = SUCCEEDED(coinit_hr);

    while (m_running)
    {
        SOCKET listen_socket = INVALID_SOCKET;
        {
            std::lock_guard<std::mutex> guard(m_mutex);
            if (m_listen_socket != static_cast<uintptr_t>(~0ULL))
                listen_socket = static_cast<SOCKET>(m_listen_socket);
        }

        if (listen_socket == INVALID_SOCKET)
            break;

        sockaddr_in client_addr{};
        int client_len = sizeof(client_addr);
        SOCKET client_socket = accept(listen_socket, reinterpret_cast<sockaddr*>(&client_addr), &client_len);

        if (client_socket == INVALID_SOCKET)
        {
            if (m_running)
                continue;
            break;
        }

        // Prevent a slow/silent client from blocking Stop() indefinitely.
        DWORD recv_timeout_ms = 5000;
        setsockopt(client_socket, SOL_SOCKET, SO_RCVTIMEO,
                   reinterpret_cast<const char*>(&recv_timeout_ms), sizeof(recv_timeout_ms));

        HandleClientSocket(static_cast<uintptr_t>(client_socket));
    }

    if (should_uninit)
        CoUninitialize();
}

void CDkEmbeddedServer::HandleClientSocket(uintptr_t client_socket_value)
{
    SOCKET client_socket = static_cast<SOCKET>(client_socket_value);

    std::string request;
    request.resize(16384);
    int received = recv(client_socket, request.data(), static_cast<int>(request.size()), 0);
    if (received <= 0)
    {
        closesocket(client_socket);
        return;
    }

    request.resize(static_cast<size_t>(received));

    DWORD send_timeout_ms = 5000;
    setsockopt(client_socket, SOL_SOCKET, SO_SNDTIMEO,
               reinterpret_cast<const char*>(&send_timeout_ms), sizeof(send_timeout_ms));

    try
    {
        auto parsed = ParseRequestLine(request);
        const std::string& method = parsed.method;
        const std::string& path = parsed.path;
        const std::string& query = parsed.query;

        std::string response = BuildHttpResponse(method, path, query);

        // Extract status code from "HTTP/1.1 NNN ..."
        int status_code = 0;
        {
            size_t sp = response.find(' ');
            if (sp != std::string::npos && sp + 3 < response.size())
                status_code = std::stoi(response.substr(sp + 1, 3));
        }

        if (method.empty() || path.empty())
            EXT_F_OUT("[dk serve] [%d] UNKNOWN <malformed request>\n", status_code);
        else
            EXT_F_OUT("[dk serve] [%d] %s %s\n", status_code, method.c_str(), path.c_str());

        send(client_socket, response.c_str(), static_cast<int>(response.size()), 0);
    }
    catch (...)
    {
        std::string response = BuildJsonResponseError(500, "INTERNAL_ERROR", "Unhandled server exception");
        send(client_socket, response.c_str(), static_cast<int>(response.size()), 0);
    }

    closesocket(client_socket);
}

std::string CDkEmbeddedServer::BuildHttpResponse(const std::string& method, const std::string& path, const std::string& query)
{
    if (method != "GET")
    {
        return BuildJsonResponseError(405, "METHOD_NOT_ALLOWED", "Only GET is supported in phase 2");
    }

    if (path == "/api/server/status")
        return BuildJsonResponseOk(HandleServerStatusRoute());

    if (path == "/api/server/stop")
        return BuildJsonResponseOk(HandleServerStopRoute());

    if (path == "/api/ttd/trace-info")
        return BuildJsonResponseOk(HandleTraceInfoRoute());

    if (path == "/api/ttd/modules")
        return BuildJsonResponseOk(HandleModulesRoute());

    if (path == "/api/ttd/threads")
        return BuildJsonResponseOk(HandleThreadsRoute());

    if (path == "/api/ttd/events/lifetime")
        return BuildJsonResponseOk(HandleLifetimeEventsRoute());

    if (path == "/api/registers")
        return BuildJsonResponseOk(HandleRegistersRoute(query));

    if (path == "/api/callstack")
        return BuildJsonResponseOk(HandleCallstackRoute(query));

    if (path == "/api/page")
        return BuildJsonResponseOk(HandlePageRoute(query));

    if (path == "/api/page/svg")
        return HandlePageSvgRoute(query);

    if (path == "/")
    {
        json root = {
            {"schemaVersion", "1.0"},
            {"name", "dk embedded server"},
            {"phase", 2},
            {"routes", json::array({"/api/server/status", "/api/server/stop", "/api/ttd/trace-info", "/api/ttd/modules", "/api/ttd/threads", "/api/ttd/events/lifetime", "/api/registers", "/api/callstack", "/api/page", "/api/page/svg"})}
        };
        return BuildJsonResponseOk(root.dump());
    }

    return BuildJsonResponseError(404, "NOT_FOUND", "Route not found");
}

std::string CDkEmbeddedServer::BuildJsonResponseOk(const std::string& body_json) const
{
    std::stringstream ss;
    ss << "HTTP/1.1 200 OK\r\n"
       << "Content-Type: application/json; charset=utf-8\r\n"
       << "Connection: close\r\n"
       << "Cache-Control: no-store\r\n"
       << "Content-Length: " << body_json.size() << "\r\n\r\n"
       << body_json;

    return ss.str();
}

std::string CDkEmbeddedServer::BuildJsonResponseError(int status_code, const std::string& code, const std::string& message) const
{
    json err = {
        {"schemaVersion", "1.0"},
        {"error", {
            {"code", code},
            {"message", message},
            {"status", status_code}
        }}
    };

    std::string body = err.dump();

    std::stringstream ss;
    ss << "HTTP/1.1 " << status_code << " ERROR\r\n"
       << "Content-Type: application/json; charset=utf-8\r\n"
       << "Connection: close\r\n"
       << "Cache-Control: no-store\r\n"
       << "Content-Length: " << body.size() << "\r\n\r\n"
       << body;

    return ss.str();
}

std::string CDkEmbeddedServer::HandleServerStatusRoute()
{
    uint64_t request_id = ++m_request_counter;
    std::string host = GetBindHost();
    uint16_t port = GetBindPort();

    json root = {
        {"schemaVersion", "1.0"},
        {"requestId", std::to_string(request_id)},
        {"server", {
            {"running", IsRunning()},
            {"host", host},
            {"port", port},
            {"uptimeMs", GetUptimeMs()}
        }}
    };

    return root.dump();
}

std::string CDkEmbeddedServer::HandleServerStopRoute()
{
    uint64_t request_id = ++m_request_counter;

    {
        std::lock_guard<std::mutex> guard(m_mutex);
        m_running = false;
    }

    json root = {
        {"schemaVersion", "1.0"},
        {"requestId", std::to_string(request_id)},
        {"server", {
            {"running", false},
            {"stopping", true},
            {"message", "Stop requested. Server will exit and return control to WinDbg."}
        }}
    };

    return root.dump();
}

std::string CDkEmbeddedServer::HandleTraceInfoRoute()
{
    uint64_t request_id = ++m_request_counter;

    json root = {
        {"schemaVersion", "1.0"},
        {"requestId", std::to_string(request_id)},
        {"trace", {
            {"available", false},
            {"isTTD", DK_MODEL_ACCESS->isTTD()},
            {"dumpFile", DK_GET_DUMP_FILENAME()}
        }}
    };

    if (!DK_MODEL_ACCESS->isTTD())
        return root.dump();

    auto proc = DK_MODEL_ACCESS->get_current_process();
    if (proc == nullptr)
        return root.dump();

    auto ttd = DK_MGET_POBJ(proc, "TTD");
    if (ttd == nullptr)
        return root.dump();

    auto lifetime = DK_MGET_POBJ(ttd, "Lifetime");
    if (lifetime == nullptr)
        return root.dump();

    auto min_pos = DK_MGET_POS(lifetime, "MinPosition");
    auto max_pos = DK_MGET_POS(lifetime, "MaxPosition");
    auto cur_pos = DK_GET_CURPOS();

    root["trace"]["available"] = true;
    root["trace"]["firstPos"] = {
        {"major", std::to_string(get<0>(min_pos))},
        {"minor", get<1>(min_pos)}
    };
    root["trace"]["lastPos"] = {
        {"major", std::to_string(get<0>(max_pos))},
        {"minor", get<1>(max_pos)}
    };
    root["trace"]["currentPos"] = {
        {"major", std::to_string(get<0>(cur_pos))},
        {"minor", get<1>(cur_pos)}
    };

    return root.dump();
}

std::string CDkEmbeddedServer::HandleModulesRoute()
{
    uint64_t request_id = ++m_request_counter;
    bool is_ttd = DK_MODEL_ACCESS->isTTD();

    json root = {
        {"schemaVersion", "1.0"},
        {"requestId", std::to_string(request_id)},
        {"trace", {
            {"isTTD", is_ttd}
        }},
        {"modules", json::array()}
    };

    auto proc = DK_MODEL_ACCESS->get_current_process();
    if (proc == nullptr)
    {
        root["moduleCount"] = 0;
        return root.dump();
    }

    std::tuple<uint64_t, uint64_t> original_pos{ 0, 0 };
    std::tuple<uint64_t, uint64_t> max_pos{ 0, 0 };
    bool should_restore_pos = false;
    DK_MOBJ_PTR ttd_obj;

    if (is_ttd)
    {
        try
        {
            ttd_obj = DK_MGET_POBJ(proc, "TTD");
            if (ttd_obj != nullptr)
            {
                auto lifetime = DK_MGET_POBJ(ttd_obj, "Lifetime");
                if (lifetime != nullptr)
                {
                    original_pos = DK_GET_CURPOS();
                    max_pos = DK_MGET_POS(lifetime, "MaxPosition");
                    DK_SEEK_TO(get<0>(max_pos), get<1>(max_pos));
                    should_restore_pos = true;
                }
            }
        }
        catch (...)
        {
            should_restore_pos = false;
        }
    }

    std::string process_name;
    try
    {
        process_name = BSTR2str(DK_MGET_PVAL<BSTR, VT_BSTR>(proc, "Name"));
    }
    catch (...)
    {
        process_name.clear();
    }

    struct ModuleRouteEntry
    {
        uint64_t base_address{ 0 };
        uint64_t image_size{ 0 };
        std::string module_name;
        std::tuple<uint64_t, uint64_t> load_pos{ 0, 0 };
        std::tuple<uint64_t, uint64_t> unload_pos{ 0, 0 };
    };

    std::vector<ModuleRouteEntry> modules;

    struct ModuleLifetimePair
    {
        std::tuple<uint64_t, uint64_t> load_pos{ 0, 0 };
        std::tuple<uint64_t, uint64_t> unload_pos{ 0, 0 };
        bool has_load{ false };
        bool has_unload{ false };
    };
    std::map<std::string, std::vector<ModuleLifetimePair>> lifetimes_by_module;

    auto modules_obj = DK_MGET_POBJ(proc, "Modules");
    auto module_entries = DK_MODEL_ACCESS->iterate(modules_obj);
    modules.reserve(module_entries.size());

    for (auto& module_entry : module_entries)
    {
        auto module_obj = std::get<1>(module_entry);
        auto module_info = DK_MGET_MODL(module_obj);

        ModuleRouteEntry route_entry;
        route_entry.base_address = std::get<0>(module_info);
        route_entry.image_size = std::get<1>(module_info);
        route_entry.module_name = std::get<2>(module_info);

        // Initialize to zero; will be populated from TTD events only
        route_entry.load_pos = { 0, 0 };
        route_entry.unload_pos = { 0, 0 };

        modules.push_back(std::move(route_entry));
    }

    if (is_ttd && ttd_obj != nullptr)
    {
        auto events_obj = DK_MGET_POBJ(ttd_obj, "Events");
        if (events_obj != nullptr)
        {
            auto events = DK_MODEL_ACCESS->iterate(events_obj);
            for (auto& event_entry : events)
            {
                auto event_obj = std::get<1>(event_entry);

                std::string event_type;
                try
                {
                    event_type = BSTR2str(DK_MGET_PVAL<BSTR, VT_BSTR>(event_obj, "Type"));
                }
                catch (...)
                {
                    event_type.clear();
                }

                std::string event_kind = ClassifyLifetimeEventType(event_type);
                if (event_kind != "moduleLoad" && event_kind != "moduleUnload")
                    continue;

                auto module_obj = DK_MGET_POBJ(event_obj, "Module");
                if (module_obj == nullptr)
                    continue;

                auto event_pos = DK_MGET_POS(event_obj, "Position");
                auto module_info = DK_MGET_MODL(module_obj);
                std::string module_id = BuildModuleId(std::get<0>(module_info), std::get<1>(module_info), std::get<2>(module_info));

                auto& pairs = lifetimes_by_module[module_id];
                if (event_kind == "moduleLoad")
                {
                    ModuleLifetimePair pair;
                    pair.load_pos = event_pos;
                    pair.has_load = true;
                    pairs.push_back(std::move(pair));
                }
                else
                {
                    bool assigned = false;
                    for (auto rit = pairs.rbegin(); rit != pairs.rend(); ++rit)
                    {
                        if (rit->has_load && !rit->has_unload)
                        {
                            rit->unload_pos = event_pos;
                            rit->has_unload = true;
                            assigned = true;
                            break;
                        }
                    }

                    if (!assigned)
                    {
                        ModuleLifetimePair pair;
                        pair.unload_pos = event_pos;
                        pair.has_unload = true;
                        pairs.push_back(std::move(pair));
                    }
                }
            }
        }
    }

    std::sort(modules.begin(), modules.end(),
              [](const ModuleRouteEntry& lhs, const ModuleRouteEntry& rhs)
              {
                  if (lhs.base_address != rhs.base_address)
                      return lhs.base_address < rhs.base_address;
                  return lhs.module_name < rhs.module_name;
              });

    std::string process_base = ToLowerAscii(Basename(process_name));
    int main_index = -1;
    for (size_t i = 0; i < modules.size(); ++i)
    {
        const auto& module_name = modules[i].module_name;
        if (ToLowerAscii(Basename(module_name)) == process_base)
        {
            main_index = static_cast<int>(i);
            break;
        }
    }

    if (main_index < 0 && !modules.empty())
        main_index = 0;

    for (size_t i = 0; i < modules.size(); ++i)
    {
        uint64_t base = modules[i].base_address;
        uint64_t size = modules[i].image_size;
        const std::string& module_name = modules[i].module_name;
        auto load_pos = modules[i].load_pos;
        auto unload_pos = modules[i].unload_pos;

        bool has_load_pos = (std::get<0>(load_pos) != 0 || std::get<1>(load_pos) != 0);
        bool has_unload_pos = (std::get<0>(unload_pos) != 0 || std::get<1>(unload_pos) != 0);
        std::string module_id = BuildModuleId(base, size, module_name);

        json lifetimes = json::array();
        auto lifetime_it = lifetimes_by_module.find(module_id);
        if (lifetime_it != lifetimes_by_module.end())
        {
            for (const auto& pair : lifetime_it->second)
            {
                lifetimes.push_back({
                    {"loadPosition", {
                        {"available", pair.has_load},
                        {"major", std::to_string(std::get<0>(pair.load_pos))},
                        {"minor", std::get<1>(pair.load_pos)}
                    }},
                    {"unloadPosition", {
                        {"available", pair.has_unload},
                        {"major", std::to_string(std::get<0>(pair.unload_pos))},
                        {"minor", std::get<1>(pair.unload_pos)}
                    }}
                });
            }

            if (!lifetime_it->second.empty())
            {
                const auto& first_pair = lifetime_it->second.front();
                const auto& last_pair = lifetime_it->second.back();
                if (first_pair.has_load)
                {
                    load_pos = first_pair.load_pos;
                    has_load_pos = true;
                }
                if (last_pair.has_unload)
                {
                    unload_pos = last_pair.unload_pos;
                    has_unload_pos = true;
                }
            }
        }
        else if (has_load_pos || has_unload_pos)
        {
            lifetimes.push_back({
                {"loadPosition", {
                    {"available", has_load_pos},
                    {"major", std::to_string(std::get<0>(load_pos))},
                    {"minor", std::get<1>(load_pos)}
                }},
                {"unloadPosition", {
                    {"available", has_unload_pos},
                    {"major", std::to_string(std::get<0>(unload_pos))},
                    {"minor", std::get<1>(unload_pos)}
                }}
            });
        }

        bool is_main = (static_cast<int>(i) == main_index);
        uint64_t lane_order_hint = is_main ? 0 : (static_cast<int>(i) < main_index ? i + 1 : i);

        root["modules"].push_back({
            {"moduleId", module_id},
            {"name", Basename(module_name)},
            {"path", module_name},
            {"baseAddress", base},
            {"imageSize", size},
            {"loadPosition", {
                {"available", has_load_pos},
                {"major", std::to_string(std::get<0>(load_pos))},
                {"minor", std::get<1>(load_pos)}
            }},
            {"unloadPosition", {
                {"available", has_unload_pos},
                {"major", std::to_string(std::get<0>(unload_pos))},
                {"minor", std::get<1>(unload_pos)}
            }},
            {"lifetimes", lifetimes},
            {"isMain", is_main},
            {"laneOrderHint", lane_order_hint}
        });
    }

    if (should_restore_pos)
    {
        try
        {
            DK_SEEK_TO(get<0>(original_pos), get<1>(original_pos));
        }
        catch (...)
        {
            // Best effort restore for non-intrusive serve-mode requests.
        }
    }

    root["moduleCount"] = modules.size();
    return root.dump();
}

std::string CDkEmbeddedServer::HandleThreadsRoute()
{
    uint64_t request_id = ++m_request_counter;
    bool is_ttd = DK_MODEL_ACCESS->isTTD();

    json root = {
        {"schemaVersion", "1.0"},
        {"requestId", std::to_string(request_id)},
        {"trace", {
            {"isTTD", is_ttd}
        }},
        {"threads", json::array()}
    };

    auto proc = DK_MODEL_ACCESS->get_current_process();
    if (proc == nullptr)
    {
        root["threadCount"] = 0;
        return root.dump();
    }

    std::tuple<uint64_t, uint64_t> original_pos{ 0, 0 };
    bool should_restore_pos = false;
    DK_MOBJ_PTR ttd_obj;

    if (is_ttd)
    {
        try
        {
            ttd_obj = DK_MGET_POBJ(proc, "TTD");
            if (ttd_obj != nullptr)
            {
                auto lifetime = DK_MGET_POBJ(ttd_obj, "Lifetime");
                if (lifetime != nullptr)
                {
                    original_pos = DK_GET_CURPOS();
                    auto max_pos = DK_MGET_POS(lifetime, "MaxPosition");
                    DK_SEEK_TO(std::get<0>(max_pos), std::get<1>(max_pos));
                    should_restore_pos = true;
                }
            }
        }
        catch (...)
        {
            should_restore_pos = false;
        }
    }

    struct ThreadLifetimePair
    {
        std::tuple<uint64_t, uint64_t> create_pos{ 0, 0 };
        std::tuple<uint64_t, uint64_t> terminate_pos{ 0, 0 };
        bool has_create{ false };
        bool has_terminate{ false };
    };

    std::map<uint64_t, std::vector<ThreadLifetimePair>> lifetimes_by_thread;
    if (is_ttd && ttd_obj != nullptr)
    {
        auto events_obj = DK_MGET_POBJ(ttd_obj, "Events");
        if (events_obj != nullptr)
        {
            auto events = DK_MODEL_ACCESS->iterate(events_obj);
            for (auto& event_entry : events)
            {
                auto event_obj = std::get<1>(event_entry);

                std::string event_type;
                try
                {
                    event_type = BSTR2str(DK_MGET_PVAL<BSTR, VT_BSTR>(event_obj, "Type"));
                }
                catch (...)
                {
                    event_type.clear();
                }

                std::string event_kind = ClassifyLifetimeEventType(event_type);
                if (event_kind != "threadCreate" && event_kind != "threadTerminate")
                    continue;

                auto event_thread = DK_MGET_POBJ(event_obj, "Thread");
                if (event_thread == nullptr)
                    continue;

                uint64_t thread_id = 0;
                try
                {
                    thread_id = DK_MGET_PVAL<uint64_t, VT_UI8>(event_thread, "Id");
                }
                catch (...)
                {
                    thread_id = 0;
                }

                std::tuple<uint64_t, uint64_t> event_pos{ 0, 0 };
                try
                {
                    event_pos = DK_MGET_POS(event_obj, "Position");
                }
                catch (...)
                {
                    event_pos = { 0, 0 };
                }

                auto& pairs = lifetimes_by_thread[thread_id];
                if (event_kind == "threadCreate")
                {
                    ThreadLifetimePair pair;
                    pair.create_pos = event_pos;
                    pair.has_create = true;
                    pairs.push_back(std::move(pair));
                }
                else
                {
                    bool assigned = false;
                    for (auto rit = pairs.rbegin(); rit != pairs.rend(); ++rit)
                    {
                        if (rit->has_create && !rit->has_terminate)
                        {
                            rit->terminate_pos = event_pos;
                            rit->has_terminate = true;
                            assigned = true;
                            break;
                        }
                    }

                    if (!assigned)
                    {
                        ThreadLifetimePair pair;
                        pair.terminate_pos = event_pos;
                        pair.has_terminate = true;
                        pairs.push_back(std::move(pair));
                    }
                }
            }
        }
    }

    auto TryReadU64 = [](DK_MOBJ_PTR& obj, const char* key, uint64_t& out_val) -> bool
    {
        try
        {
            out_val = DK_MGET_PVAL<uint64_t, VT_UI8>(obj, key);
            return true;
        }
        catch (...)
        {
        }

        try
        {
            out_val = DK_MGET_PVAL<DWORD, VT_UI4>(obj, key);
            return true;
        }
        catch (...)
        {
        }

        return false;
    };

    auto ResolveThreadProcAddress = [&](DK_MOBJ_PTR& thread_obj) -> uint64_t
    {
        uint64_t addr = 0;
        const char* keys[] = {
            "ThreadProc",
            "StartAddress",
            "Win32StartAddress",
            "UserStartAddress",
            "InitialIP",
            "InstructionOffset"
        };

        for (auto key : keys)
        {
            if (TryReadU64(thread_obj, key, addr))
                return addr;
        }

        auto thread_ttd = DK_MGET_POBJ(thread_obj, "TTD");
        if (thread_ttd != nullptr)
        {
            for (auto key : keys)
            {
                if (TryReadU64(thread_ttd, key, addr))
                    return addr;
            }
        }

        return 0;
    };

    auto TryReadString = [](DK_MOBJ_PTR& obj, const char* key, std::string& out_val) -> bool
    {
        try
        {
            out_val = BSTR2str(DK_MGET_PVAL<BSTR, VT_BSTR>(obj, key));
            return !out_val.empty();
        }
        catch (...)
        {
        }

        return false;
    };

    auto ResolveThreadProcSymbol = [&](DK_MOBJ_PTR& thread_obj) -> std::string
    {
        std::string symbol;
        const char* keys[] = {
            "ThreadProcSymbol",
            "StartSymbol",
            "Win32StartSymbol",
            "UserStartSymbol"
        };

        for (auto key : keys)
        {
            if (TryReadString(thread_obj, key, symbol))
                return symbol;
        }

        auto thread_ttd = DK_MGET_POBJ(thread_obj, "TTD");
        if (thread_ttd != nullptr)
        {
            for (auto key : keys)
            {
                if (TryReadString(thread_ttd, key, symbol))
                    return symbol;
            }
        }

        return "";
    };

    auto BuildLifetimesJson = [](const std::vector<ThreadLifetimePair>& pairs) -> json
    {
        json lifetimes = json::array();
        for (const auto& pair : pairs)
        {
            lifetimes.push_back({
                {"createPosition", {
                    {"available", pair.has_create},
                    {"major", std::to_string(std::get<0>(pair.create_pos))},
                    {"minor", std::get<1>(pair.create_pos)}
                }},
                {"terminatePosition", {
                    {"available", pair.has_terminate},
                    {"major", std::to_string(std::get<0>(pair.terminate_pos))},
                    {"minor", std::get<1>(pair.terminate_pos)}
                }}
            });
        }
        return lifetimes;
    };

    std::set<uint64_t> emitted_thread_ids;
    auto threads_obj = DK_MGET_POBJ(proc, "Threads");
    if (threads_obj != nullptr)
    {
        auto threads = DK_MODEL_ACCESS->iterate(threads_obj);
        for (auto& thread_entry : threads)
        {
            auto thread_obj = std::get<1>(thread_entry);

            uint64_t thread_id = 0;
            (void)TryReadU64(thread_obj, "Id", thread_id);
            emitted_thread_ids.insert(thread_id);

            uint64_t proc_addr = ResolveThreadProcAddress(thread_obj);
            std::string symbol_name = ResolveThreadProcSymbol(thread_obj);
            uint64_t symbol_offset = 0;
            bool has_symbol = !symbol_name.empty();

            auto pair_it = lifetimes_by_thread.find(thread_id);
            json lifetimes = pair_it != lifetimes_by_thread.end() ? BuildLifetimesJson(pair_it->second) : json::array();

            bool has_create_pos = false;
            bool has_terminate_pos = false;
            std::tuple<uint64_t, uint64_t> create_pos{ 0, 0 };
            std::tuple<uint64_t, uint64_t> terminate_pos{ 0, 0 };
            if (pair_it != lifetimes_by_thread.end() && !pair_it->second.empty())
            {
                const auto& first_pair = pair_it->second.front();
                const auto& last_pair = pair_it->second.back();
                has_create_pos = first_pair.has_create;
                has_terminate_pos = last_pair.has_terminate;
                create_pos = first_pair.create_pos;
                terminate_pos = last_pair.terminate_pos;
            }

            root["threads"].push_back({
                {"threadId", thread_id},
                {"procAddress", proc_addr},
                {"procSymbol", {
                    {"available", has_symbol},
                    {"name", symbol_name},
                    {"displacement", symbol_offset}
                }},
                {"createPosition", {
                    {"available", has_create_pos},
                    {"major", std::to_string(std::get<0>(create_pos))},
                    {"minor", std::get<1>(create_pos)}
                }},
                {"terminatePosition", {
                    {"available", has_terminate_pos},
                    {"major", std::to_string(std::get<0>(terminate_pos))},
                    {"minor", std::get<1>(terminate_pos)}
                }},
                {"lifetimes", lifetimes}
            });
        }
    }

    for (const auto& kv : lifetimes_by_thread)
    {
        if (emitted_thread_ids.find(kv.first) != emitted_thread_ids.end())
            continue;

        root["threads"].push_back({
            {"threadId", kv.first},
            {"procAddress", 0},
            {"procSymbol", {
                {"available", false},
                {"name", ""},
                {"displacement", 0}
            }},
            {"createPosition", {
                {"available", false},
                {"major", "0"},
                {"minor", 0}
            }},
            {"terminatePosition", {
                {"available", false},
                {"major", "0"},
                {"minor", 0}
            }},
            {"lifetimes", BuildLifetimesJson(kv.second)}
        });
    }

    if (should_restore_pos)
    {
        try
        {
            DK_SEEK_TO(std::get<0>(original_pos), std::get<1>(original_pos));
        }
        catch (...)
        {
            // Best effort restore for non-intrusive serve-mode requests.
        }
    }

    root["threadCount"] = root["threads"].size();
    return root.dump();
}

std::string CDkEmbeddedServer::HandleLifetimeEventsRoute()
{
    uint64_t request_id = ++m_request_counter;
    bool is_ttd = DK_MODEL_ACCESS->isTTD();

    json root = {
        {"schemaVersion", "1.0"},
        {"requestId", std::to_string(request_id)},
        {"trace", {
            {"isTTD", is_ttd}
        }},
        {"moduleLifetimeEvents", json::array()},
        {"threadLifetimeEvents", json::array()},
        {"moduleLifetimes", json::array()},
        {"threadLifetimes", json::array()}
    };

    if (!is_ttd)
    {
        root["moduleLifetimeEventCount"] = 0;
        root["threadLifetimeEventCount"] = 0;
        return root.dump();
    }

    auto proc = DK_MODEL_ACCESS->get_current_process();
    if (proc == nullptr)
    {
        root["moduleLifetimeEventCount"] = 0;
        root["threadLifetimeEventCount"] = 0;
        return root.dump();
    }

    auto ttd = DK_MGET_POBJ(proc, "TTD");
    if (ttd == nullptr)
    {
        root["moduleLifetimeEventCount"] = 0;
        root["threadLifetimeEventCount"] = 0;
        return root.dump();
    }

    std::tuple<uint64_t, uint64_t> original_pos{ 0, 0 };
    bool should_restore_pos = false;
    try
    {
        auto lifetime = DK_MGET_POBJ(ttd, "Lifetime");
        if (lifetime != nullptr)
        {
            original_pos = DK_GET_CURPOS();
            auto max_pos = DK_MGET_POS(lifetime, "MaxPosition");
            DK_SEEK_TO(std::get<0>(max_pos), std::get<1>(max_pos));
            should_restore_pos = true;
        }
    }
    catch (...)
    {
        should_restore_pos = false;
    }

    struct ModuleLifetimeSummary
    {
        std::string module_id;
        std::string name;
        std::string path;
        uint64_t base_address{ 0 };
        uint64_t image_size{ 0 };
        std::tuple<uint64_t, uint64_t> first_event_pos{ 0, 0 };
        std::tuple<uint64_t, uint64_t> last_event_pos{ 0, 0 };
        std::tuple<uint64_t, uint64_t> load_pos{ 0, 0 };
        std::tuple<uint64_t, uint64_t> unload_pos{ 0, 0 };
    };

    struct ThreadLifetimeSummary
    {
        uint64_t thread_id{ 0 };
        std::tuple<uint64_t, uint64_t> first_event_pos{ 0, 0 };
        std::tuple<uint64_t, uint64_t> last_event_pos{ 0, 0 };
        std::tuple<uint64_t, uint64_t> create_pos{ 0, 0 };
        std::tuple<uint64_t, uint64_t> terminate_pos{ 0, 0 };
    };

    std::map<std::string, ModuleLifetimeSummary> module_lifetime_map;
    std::map<uint64_t, ThreadLifetimeSummary> thread_lifetime_map;

    auto events_obj = DK_MGET_POBJ(ttd, "Events");
    if (events_obj != nullptr)
    {
        auto events = DK_MODEL_ACCESS->iterate(events_obj);
        for (auto& event_entry : events)
        {
            auto event_obj = std::get<1>(event_entry);

            std::string event_type;
            try
            {
                event_type = BSTR2str(DK_MGET_PVAL<BSTR, VT_BSTR>(event_obj, "Type"));
            }
            catch (...)
            {
                event_type.clear();
            }

            std::string event_kind = ClassifyLifetimeEventType(event_type);
            if (event_kind.empty())
                continue;

            std::tuple<uint64_t, uint64_t> event_pos{ 0, 0 };
            try
            {
                event_pos = DK_MGET_POS(event_obj, "Position");
            }
            catch (...)
            {
                event_pos = { 0, 0 };
            }

            if (event_kind == "moduleLoad" || event_kind == "moduleUnload")
            {
                auto module_obj = DK_MGET_POBJ(event_obj, "Module");
                if (module_obj != nullptr)
                {
                    auto module_info = DK_MGET_MODL(module_obj);
                    uint64_t base = std::get<0>(module_info);
                    uint64_t size = std::get<1>(module_info);
                    std::string module_path = std::get<2>(module_info);
                    std::string module_name = Basename(module_path);
                    std::string module_id = BuildModuleId(base, size, module_path);

                    root["moduleLifetimeEvents"].push_back({
                        {"eventType", event_type},
                        {"kind", event_kind},
                        {"position", PosToJson(event_pos)},
                        {"module", {
                            {"moduleId", module_id},
                            {"name", module_name},
                            {"path", module_path},
                            {"baseAddress", base},
                            {"imageSize", size}
                        }}
                    });

                    auto& summary = module_lifetime_map[module_id];
                    if (summary.module_id.empty())
                    {
                        summary.module_id = module_id;
                        summary.name = module_name;
                        summary.path = module_path;
                        summary.base_address = base;
                        summary.image_size = size;
                    }

                    if (IsZeroPos(summary.first_event_pos) || event_pos < summary.first_event_pos)
                        summary.first_event_pos = event_pos;
                    if (IsZeroPos(summary.last_event_pos) || summary.last_event_pos < event_pos)
                        summary.last_event_pos = event_pos;

                    if (event_kind == "moduleLoad" && (IsZeroPos(summary.load_pos) || event_pos < summary.load_pos))
                        summary.load_pos = event_pos;
                    if (event_kind == "moduleUnload" && (IsZeroPos(summary.unload_pos) || summary.unload_pos < event_pos))
                        summary.unload_pos = event_pos;
                }
            }
            else if (event_kind == "threadCreate" || event_kind == "threadTerminate")
            {
                auto thread_obj = DK_MGET_POBJ(event_obj, "Thread");
                if (thread_obj != nullptr)
                {
                    uint64_t thread_id = 0;
                    try
                    {
                        thread_id = DK_MGET_PVAL<uint64_t, VT_UI8>(thread_obj, "Id");
                    }
                    catch (...)
                    {
                        thread_id = 0;
                    }

                    root["threadLifetimeEvents"].push_back({
                        {"eventType", event_type},
                        {"kind", event_kind},
                        {"position", PosToJson(event_pos)},
                        {"thread", {
                            {"id", thread_id}
                        }}
                    });

                    auto& summary = thread_lifetime_map[thread_id];
                    summary.thread_id = thread_id;

                    if (IsZeroPos(summary.first_event_pos) || event_pos < summary.first_event_pos)
                        summary.first_event_pos = event_pos;
                    if (IsZeroPos(summary.last_event_pos) || summary.last_event_pos < event_pos)
                        summary.last_event_pos = event_pos;

                    if (event_kind == "threadCreate" && (IsZeroPos(summary.create_pos) || event_pos < summary.create_pos))
                        summary.create_pos = event_pos;
                    if (event_kind == "threadTerminate" && (IsZeroPos(summary.terminate_pos) || summary.terminate_pos < event_pos))
                        summary.terminate_pos = event_pos;
                }
            }
        }
    }

    for (const auto& kv : module_lifetime_map)
    {
        const auto& summary = kv.second;
        root["moduleLifetimes"].push_back({
            {"moduleId", summary.module_id},
            {"name", summary.name},
            {"path", summary.path},
            {"baseAddress", summary.base_address},
            {"imageSize", summary.image_size},
            {"firstEventPosition", PosToJson(summary.first_event_pos)},
            {"lastEventPosition", PosToJson(summary.last_event_pos)},
            {"loadPosition", {
                {"available", !IsZeroPos(summary.load_pos)},
                {"major", std::to_string(std::get<0>(summary.load_pos))},
                {"minor", std::get<1>(summary.load_pos)}
            }},
            {"unloadPosition", {
                {"available", !IsZeroPos(summary.unload_pos)},
                {"major", std::to_string(std::get<0>(summary.unload_pos))},
                {"minor", std::get<1>(summary.unload_pos)}
            }}
        });
    }

    for (const auto& kv : thread_lifetime_map)
    {
        const auto& summary = kv.second;
        root["threadLifetimes"].push_back({
            {"threadId", summary.thread_id},
            {"firstEventPosition", PosToJson(summary.first_event_pos)},
            {"lastEventPosition", PosToJson(summary.last_event_pos)},
            {"createPosition", {
                {"available", !IsZeroPos(summary.create_pos)},
                {"major", std::to_string(std::get<0>(summary.create_pos))},
                {"minor", std::get<1>(summary.create_pos)}
            }},
            {"terminatePosition", {
                {"available", !IsZeroPos(summary.terminate_pos)},
                {"major", std::to_string(std::get<0>(summary.terminate_pos))},
                {"minor", std::get<1>(summary.terminate_pos)}
            }}
        });
    }

    if (should_restore_pos)
    {
        try
        {
            DK_SEEK_TO(std::get<0>(original_pos), std::get<1>(original_pos));
        }
        catch (...)
        {
            // Best effort restore for non-intrusive serve-mode requests.
        }
    }

    root["moduleLifetimeEventCount"] = root["moduleLifetimeEvents"].size();
    root["threadLifetimeEventCount"] = root["threadLifetimeEvents"].size();

    return root.dump();
}

std::string CDkEmbeddedServer::HandleRegistersRoute(const std::string& query)
{
    uint64_t request_id = ++m_request_counter;
    bool is_ttd = DK_MODEL_ACCESS->isTTD();

    json root = {
        {"schemaVersion", "1.0"},
        {"requestId", std::to_string(request_id)},
        {"trace", {{"isTTD", is_ttd}}},
        {"registers", json::object()},
        {"available", false}
    };

    auto proc = DK_MODEL_ACCESS->get_current_process();
    if (proc == nullptr)
        return root.dump();

    auto query_params = ParseQueryParams(query);

    uint64_t requested_thread_id = 0;
    bool has_requested_thread = false;
    auto it_thread = query_params.find("thread_id");
    if (it_thread == query_params.end())
        it_thread = query_params.find("threadId");
    if (it_thread != query_params.end())
        has_requested_thread = TryParseU64(it_thread->second, requested_thread_id);

    bool should_restore_pos = false;
    std::tuple<uint64_t, uint64_t> original_pos{ 0, 0 };
    if (is_ttd)
    {
        auto it_time = query_params.find("time");
        if (it_time != query_params.end())
        {
            double requested_time = 0.0;
            if (TryParseDouble(it_time->second, requested_time))
            {
                try
                {
                    auto ttd = DK_MGET_POBJ(proc, "TTD");
                    auto lifetime = ttd != nullptr ? DK_MGET_POBJ(ttd, "Lifetime") : DK_MOBJ_PTR{};
                    if (lifetime != nullptr)
                    {
                        original_pos = DK_GET_CURPOS();
                        auto min_pos = DK_MGET_POS(lifetime, "MinPosition");
                        auto max_pos = DK_MGET_POS(lifetime, "MaxPosition");

                        double ratio = requested_time;
                        if (requested_time > 1.0)
                            ratio = requested_time / 10000.0;
                        ratio = std::max(0.0, std::min(1.0, ratio));

                        uint64_t seq = std::get<0>(min_pos);
                        if (std::get<0>(max_pos) > std::get<0>(min_pos))
                        {
                            double span = static_cast<double>(std::get<0>(max_pos) - std::get<0>(min_pos));
                            seq = std::get<0>(min_pos) + static_cast<uint64_t>(span * ratio);
                        }

                        uint64_t step = std::get<1>(min_pos);
                        if (std::get<1>(max_pos) > std::get<1>(min_pos))
                        {
                            double span = static_cast<double>(std::get<1>(max_pos) - std::get<1>(min_pos));
                            step = std::get<1>(min_pos) + static_cast<uint64_t>(span * ratio);
                        }

                        DK_SEEK_TO(seq, step);
                        should_restore_pos = true;
                    }
                }
                catch (...)
                {
                }
            }
        }
    }

    DK_MOBJ_PTR selected_thread;
    auto threads_obj = DK_MGET_POBJ(proc, "Threads");
    if (threads_obj != nullptr)
    {
        auto threads = DK_MODEL_ACCESS->iterate(threads_obj);
        for (auto& thread_entry : threads)
        {
            auto thread_obj = std::get<1>(thread_entry);
            uint64_t tid = 0;
            try
            {
                tid = DK_MGET_PVAL<uint64_t, VT_UI8>(thread_obj, "Id");
            }
            catch (...)
            {
                continue;
            }

            if (!has_requested_thread || tid == requested_thread_id)
            {
                selected_thread = thread_obj;
                requested_thread_id = tid;
                has_requested_thread = true;
                break;
            }
        }
    }

    if (selected_thread == nullptr)
        selected_thread = DK_MODEL_ACCESS->get_current_thread();

    auto FormatHexU64 = [](uint64_t v) -> std::string
    {
        std::stringstream ss;
        ss << "0x" << std::hex << std::setw(16) << std::setfill('0') << v;
        return ss.str();
    };

    auto TryReadObjU64 = [](DK_MOBJ_PTR& obj, uint64_t& out) -> bool
    {
        try { out = DK_MGET_VAL<uint64_t, VT_UI8>(obj); return true; } catch (...) {}
        try { out = DK_MGET_VAL<DWORD, VT_UI4>(obj); return true; } catch (...) {}
        try { out = static_cast<uint64_t>(DK_MGET_VAL<int64_t, VT_I8>(obj)); return true; } catch (...) {}
        return false;
    };

    auto TryReadRegister = [&](const std::vector<DK_MOBJ_PTR>& roots, const std::vector<std::string>& names, uint64_t& out_val) -> bool
    {
        for (auto& root_obj : roots)
        {
            if (root_obj == nullptr)
                continue;

            for (const auto& name : names)
            {
                try
                {
                    auto reg_obj = DK_MGET_POBJ(const_cast<DK_MOBJ_PTR&>(root_obj), name);
                    if (reg_obj != nullptr && TryReadObjU64(reg_obj, out_val))
                        return true;
                }
                catch (...)
                {
                }
            }
        }
        return false;
    };

    if (selected_thread != nullptr)
    {
        root["threadId"] = requested_thread_id;

        std::vector<DK_MOBJ_PTR> reg_roots;
        auto regs_obj = DK_MGET_POBJ(selected_thread, "Registers");
        if (regs_obj != nullptr)
        {
            reg_roots.push_back(regs_obj);
            auto user_obj = DK_MGET_POBJ(regs_obj, "User");
            if (user_obj != nullptr) reg_roots.push_back(user_obj);
            auto kernel_obj = DK_MGET_POBJ(regs_obj, "Kernel");
            if (kernel_obj != nullptr) reg_roots.push_back(kernel_obj);
        }

        std::map<std::string, std::vector<std::string>> reg_aliases = {
            {"rax", {"rax", "Rax", "RAX"}},
            {"rbx", {"rbx", "Rbx", "RBX"}},
            {"rcx", {"rcx", "Rcx", "RCX"}},
            {"rdx", {"rdx", "Rdx", "RDX"}},
            {"rsi", {"rsi", "Rsi", "RSI"}},
            {"rdi", {"rdi", "Rdi", "RDI"}},
            {"rbp", {"rbp", "Rbp", "RBP"}},
            {"rsp", {"rsp", "Rsp", "RSP"}},
            {"rip", {"rip", "Rip", "RIP", "pc", "Pc", "IP"}},
            {"r8", {"r8", "R8"}},
            {"r9", {"r9", "R9"}},
            {"r10", {"r10", "R10"}},
            {"r11", {"r11", "R11"}},
            {"r12", {"r12", "R12"}},
            {"r13", {"r13", "R13"}},
            {"r14", {"r14", "R14"}},
            {"r15", {"r15", "R15"}}
        };

        for (const auto& kv : reg_aliases)
        {
            uint64_t reg_value = 0;
            if (TryReadRegister(reg_roots, kv.second, reg_value))
                root["registers"][kv.first] = FormatHexU64(reg_value);
        }

        // Fallback RIP/RSP from top frame attributes when direct register access is unavailable.
        auto stack_obj = DK_MGET_POBJ(selected_thread, "Stack");
        if (stack_obj != nullptr)
        {
            auto frames_obj = DK_MGET_POBJ(stack_obj, "Frames");
            if (frames_obj != nullptr)
            {
                auto frames = DK_MODEL_ACCESS->iterate(frames_obj);
                if (!frames.empty())
                {
                    auto frame_attrs = DK_MGET_POBJ(std::get<1>(frames.front()), "Attributes");
                    if (frame_attrs != nullptr)
                    {
                        try
                        {
                            if (!root["registers"].contains("rip"))
                            {
                                uint64_t rip = DK_MGET_PVAL<uint64_t, VT_UI8>(frame_attrs, "InstructionOffset");
                                root["registers"]["rip"] = FormatHexU64(rip);
                            }
                        }
                        catch (...) {}

                        try
                        {
                            if (!root["registers"].contains("rsp"))
                            {
                                uint64_t rsp = DK_MGET_PVAL<uint64_t, VT_UI8>(frame_attrs, "StackOffset");
                                root["registers"]["rsp"] = FormatHexU64(rsp);
                            }
                        }
                        catch (...) {}
                    }
                }
            }
        }

        root["available"] = !root["registers"].empty();
    }

    if (should_restore_pos)
    {
        try
        {
            DK_SEEK_TO(std::get<0>(original_pos), std::get<1>(original_pos));
        }
        catch (...)
        {
        }
    }

    return root.dump();
}

std::string CDkEmbeddedServer::HandleCallstackRoute(const std::string& query)
{
    uint64_t request_id = ++m_request_counter;
    bool is_ttd = DK_MODEL_ACCESS->isTTD();

    json root = {
        {"schemaVersion", "1.0"},
        {"requestId", std::to_string(request_id)},
        {"trace", {{"isTTD", is_ttd}}},
        {"frames", json::array()},
        {"available", false}
    };

    auto proc = DK_MODEL_ACCESS->get_current_process();
    if (proc == nullptr)
    {
        root["frameCount"] = 0;
        return root.dump();
    }

    auto query_params = ParseQueryParams(query);

    uint64_t requested_thread_id = 0;
    bool has_requested_thread = false;
    auto it_thread = query_params.find("thread_id");
    if (it_thread == query_params.end())
        it_thread = query_params.find("threadId");
    if (it_thread != query_params.end())
        has_requested_thread = TryParseU64(it_thread->second, requested_thread_id);

    bool should_restore_pos = false;
    std::tuple<uint64_t, uint64_t> original_pos{ 0, 0 };
    if (is_ttd)
    {
        auto it_time = query_params.find("time");
        if (it_time != query_params.end())
        {
            double requested_time = 0.0;
            if (TryParseDouble(it_time->second, requested_time))
            {
                try
                {
                    auto ttd = DK_MGET_POBJ(proc, "TTD");
                    auto lifetime = ttd != nullptr ? DK_MGET_POBJ(ttd, "Lifetime") : DK_MOBJ_PTR{};
                    if (lifetime != nullptr)
                    {
                        original_pos = DK_GET_CURPOS();
                        auto min_pos = DK_MGET_POS(lifetime, "MinPosition");
                        auto max_pos = DK_MGET_POS(lifetime, "MaxPosition");

                        double ratio = requested_time;
                        if (requested_time > 1.0)
                            ratio = requested_time / 10000.0;
                        ratio = std::max(0.0, std::min(1.0, ratio));

                        uint64_t seq = std::get<0>(min_pos);
                        if (std::get<0>(max_pos) > std::get<0>(min_pos))
                        {
                            double span = static_cast<double>(std::get<0>(max_pos) - std::get<0>(min_pos));
                            seq = std::get<0>(min_pos) + static_cast<uint64_t>(span * ratio);
                        }

                        uint64_t step = std::get<1>(min_pos);
                        if (std::get<1>(max_pos) > std::get<1>(min_pos))
                        {
                            double span = static_cast<double>(std::get<1>(max_pos) - std::get<1>(min_pos));
                            step = std::get<1>(min_pos) + static_cast<uint64_t>(span * ratio);
                        }

                        DK_SEEK_TO(seq, step);
                        should_restore_pos = true;
                    }
                }
                catch (...)
                {
                }
            }
        }
    }

    DK_MOBJ_PTR selected_thread;
    auto threads_obj = DK_MGET_POBJ(proc, "Threads");
    if (threads_obj != nullptr)
    {
        auto threads = DK_MODEL_ACCESS->iterate(threads_obj);
        for (auto& thread_entry : threads)
        {
            auto thread_obj = std::get<1>(thread_entry);
            uint64_t tid = 0;
            try
            {
                tid = DK_MGET_PVAL<uint64_t, VT_UI8>(thread_obj, "Id");
            }
            catch (...)
            {
                continue;
            }

            if (!has_requested_thread || tid == requested_thread_id)
            {
                selected_thread = thread_obj;
                requested_thread_id = tid;
                has_requested_thread = true;
                break;
            }
        }
    }

    if (selected_thread == nullptr)
        selected_thread = DK_MODEL_ACCESS->get_current_thread();

    if (selected_thread != nullptr)
    {
        root["threadId"] = requested_thread_id;

        auto stack_obj = DK_MGET_POBJ(selected_thread, "Stack");
        auto frames_obj = stack_obj != nullptr ? DK_MGET_POBJ(stack_obj, "Frames") : DK_MOBJ_PTR{};
        if (frames_obj != nullptr)
        {
            auto frames = DK_MODEL_ACCESS->iterate(frames_obj);
            for (auto& frame_entry : frames)
            {
                auto frame_obj = std::get<1>(frame_entry);
                auto frame_attrs = DK_MGET_POBJ(frame_obj, "Attributes");
                if (frame_attrs == nullptr)
                    continue;

                uint64_t frame_number = 0;
                uint64_t insn_ptr = 0;
                uint64_t return_ptr = 0;
                uint64_t frame_ptr = 0;
                uint64_t stack_ptr = 0;

                try { frame_number = DK_MGET_PVAL<DWORD, VT_UI4>(frame_attrs, "FrameNumber"); } catch (...) {}
                try { insn_ptr = DK_MGET_PVAL<uint64_t, VT_UI8>(frame_attrs, "InstructionOffset"); } catch (...) {}
                try { return_ptr = DK_MGET_PVAL<uint64_t, VT_UI8>(frame_attrs, "ReturnOffset"); } catch (...) {}
                try { frame_ptr = DK_MGET_PVAL<uint64_t, VT_UI8>(frame_attrs, "FrameOffset"); } catch (...) {}
                try { stack_ptr = DK_MGET_PVAL<uint64_t, VT_UI8>(frame_attrs, "StackOffset"); } catch (...) {}

                auto sym = EXT_F_Addr2Sym(insn_ptr);
                root["frames"].push_back({
                    {"frameNumber", frame_number},
                    {"instructionOffset", insn_ptr},
                    {"returnOffset", return_ptr},
                    {"frameOffset", frame_ptr},
                    {"stackOffset", stack_ptr},
                    {"function", std::get<0>(sym)},
                    {"displacement", std::get<1>(sym)}
                });
            }
        }

        root["frameCount"] = root["frames"].size();
        root["available"] = root["frameCount"].get<size_t>() > 0;
    }
    else
    {
        root["frameCount"] = 0;
    }

    if (should_restore_pos)
    {
        try
        {
            DK_SEEK_TO(std::get<0>(original_pos), std::get<1>(original_pos));
        }
        catch (...)
        {
        }
    }

    return root.dump();
}

std::string CDkEmbeddedServer::HandlePageRoute(const std::string& query)
{
    uint64_t request_id = ++m_request_counter;
    bool is_ttd = DK_MODEL_ACCESS->isTTD();

    json root = {
        {"schemaVersion", "1.0"},
        {"requestId", std::to_string(request_id)},
        {"available",  false},
        {"pageAddr",   ""},
        {"rsp",        ""},
        {"bytes",      ""},
        {"ptr2sym",    json::array()},
        {"ptr2local",  json::array()},
        {"ptr2heap",   json::array()},
        {"ptr2astr",   json::array()},
        {"ptr2ustr",   json::array()}
    };

    auto proc = DK_MODEL_ACCESS->get_current_process();
    if (proc == nullptr)
        return root.dump();

    auto query_params = ParseQueryParams(query);

    uint64_t requested_thread_id = 0;
    bool has_requested_thread = false;
    auto it_thread = query_params.find("thread_id");
    if (it_thread == query_params.end())
        it_thread = query_params.find("threadId");
    if (it_thread != query_params.end())
        has_requested_thread = TryParseU64(it_thread->second, requested_thread_id);

    bool should_restore_pos = false;
    std::tuple<uint64_t, uint64_t> original_pos{ 0, 0 };
    if (is_ttd)
    {
        auto it_time = query_params.find("time");
        if (it_time != query_params.end())
        {
            double requested_time = 0.0;
            if (TryParseDouble(it_time->second, requested_time))
            {
                try
                {
                    auto ttd = DK_MGET_POBJ(proc, "TTD");
                    auto lifetime = ttd != nullptr ? DK_MGET_POBJ(ttd, "Lifetime") : DK_MOBJ_PTR{};
                    if (lifetime != nullptr)
                    {
                        original_pos = DK_GET_CURPOS();
                        auto min_pos = DK_MGET_POS(lifetime, "MinPosition");
                        auto max_pos = DK_MGET_POS(lifetime, "MaxPosition");

                        double ratio = requested_time;
                        if (requested_time > 1.0)
                            ratio = requested_time / 10000.0;
                        ratio = std::max(0.0, std::min(1.0, ratio));

                        uint64_t seq = std::get<0>(min_pos);
                        if (std::get<0>(max_pos) > std::get<0>(min_pos))
                        {
                            double span = static_cast<double>(std::get<0>(max_pos) - std::get<0>(min_pos));
                            seq = std::get<0>(min_pos) + static_cast<uint64_t>(span * ratio);
                        }

                        uint64_t step = std::get<1>(min_pos);
                        if (std::get<1>(max_pos) > std::get<1>(min_pos))
                        {
                            double span = static_cast<double>(std::get<1>(max_pos) - std::get<1>(min_pos));
                            step = std::get<1>(min_pos) + static_cast<uint64_t>(span * ratio);
                        }

                        DK_SEEK_TO(seq, step);
                        should_restore_pos = true;
                    }
                }
                catch (...) {}
            }
        }
    }

    // Select thread
    DK_MOBJ_PTR selected_thread;
    auto threads_obj = DK_MGET_POBJ(proc, "Threads");
    if (threads_obj != nullptr)
    {
        auto threads = DK_MODEL_ACCESS->iterate(threads_obj);
        for (auto& thread_entry : threads)
        {
            auto thread_obj = std::get<1>(thread_entry);
            uint64_t tid = 0;
            try { tid = DK_MGET_PVAL<uint64_t, VT_UI8>(thread_obj, "Id"); }
            catch (...) { continue; }

            if (!has_requested_thread || tid == requested_thread_id)
            {
                selected_thread = thread_obj;
                has_requested_thread = true;
                break;
            }
        }
    }
    if (selected_thread == nullptr)
        selected_thread = DK_MODEL_ACCESS->get_current_thread();

    auto FormatHexU64 = [](uint64_t v) -> std::string
    {
        std::stringstream ss;
        ss << "0x" << std::hex << std::setw(16) << std::setfill('0') << v;
        return ss.str();
    };

    auto TryReadObjU64 = [](DK_MOBJ_PTR& obj, uint64_t& out) -> bool
    {
        try { out = DK_MGET_VAL<uint64_t, VT_UI8>(obj); return true; } catch (...) {}
        try { out = DK_MGET_VAL<DWORD, VT_UI4>(obj); return true; } catch (...) {}
        try { out = static_cast<uint64_t>(DK_MGET_VAL<int64_t, VT_I8>(obj)); return true; } catch (...) {}
        return false;
    };

    // Extract RSP from thread registers
    uint64_t rsp_value = 0;
    bool has_rsp = false;
    if (selected_thread != nullptr)
    {
        auto regs_obj = DK_MGET_POBJ(selected_thread, "Registers");
        std::vector<DK_MOBJ_PTR> reg_roots;
        if (regs_obj != nullptr)
        {
            reg_roots.push_back(regs_obj);
            auto user_obj = DK_MGET_POBJ(regs_obj, "User");
            if (user_obj != nullptr) reg_roots.push_back(user_obj);
        }

        for (auto& reg_root : reg_roots)
        {
            for (const auto& name : std::vector<std::string>{"rsp", "Rsp", "RSP"})
            {
                try
                {
                    auto reg_obj = DK_MGET_POBJ(reg_root, name);
                    if (reg_obj != nullptr && TryReadObjU64(reg_obj, rsp_value))
                    {
                        has_rsp = (rsp_value != 0);
                        break;
                    }
                }
                catch (...) {}
            }
            if (has_rsp) break;
        }

        // Fallback: StackOffset from top frame attributes
        if (!has_rsp)
        {
            try
            {
                auto stack_obj = DK_MGET_POBJ(selected_thread, "Stack");
                if (stack_obj != nullptr)
                {
                    auto frames_obj = DK_MGET_POBJ(stack_obj, "Frames");
                    if (frames_obj != nullptr)
                    {
                        auto frames = DK_MODEL_ACCESS->iterate(frames_obj);
                        if (!frames.empty())
                        {
                            auto frame_attrs = DK_MGET_POBJ(std::get<1>(frames.front()), "Attributes");
                            if (frame_attrs != nullptr)
                            {
                                rsp_value = DK_MGET_PVAL<uint64_t, VT_UI8>(frame_attrs, "StackOffset");
                                has_rsp = (rsp_value != 0);
                            }
                        }
                    }
                }
            }
            catch (...) {}
        }
    }

    if (!has_rsp || rsp_value == 0)
    {
        if (should_restore_pos)
        {
            try { DK_SEEK_TO(std::get<0>(original_pos), std::get<1>(original_pos)); }
            catch (...) {}
        }
        return root.dump();
    }

    uint64_t page_base = rsp_value & 0xFFFFFFFFFFFFF000ULL;

    // Read 4096 bytes of the stack page
    std::string page(0x1000, '\0');
    {
        ULONG bytes_read = 0;
        HRESULT hr = EXT_D_IDebugDataSpaces->ReadVirtual(page_base, (uint8_t*)page.data(), 0x1000, &bytes_read);
        if (hr != S_OK || bytes_read != 0x1000)
        {
            // Byte-by-byte fallback
            for (size_t i = 0; i < 0x1000; i++)
            {
                try
                {
                    ULONG br = 0;
                    EXT_D_IDebugDataSpaces->ReadVirtual(page_base + i, (uint8_t*)page.data() + i, 1, &br);
                }
                catch (...) {}
            }
        }
    }

    // Encode bytes as lowercase hex string (8192 chars for 4096 bytes)
    {
        std::stringstream hex_ss;
        hex_ss << std::hex << std::setfill('0');
        for (unsigned char b : page)
            hex_ss << std::setw(2) << static_cast<unsigned int>(b);
        root["bytes"] = hex_ss.str();
    }

    root["pageAddr"]  = FormatHexU64(page_base);
    root["rsp"]       = FormatHexU64(rsp_value);
    root["available"] = true;

    // Analyze the page with CMemoryAnalyzer (mirrors page_2_svg logic)
    try
    {
        CMemoryAnalyzer manalyzer(page, page_base, 0x1000, page_base, page_base + 0x1000);
        manalyzer.analyze();

        for (auto& kv : manalyzer.get_ptr2sym())
        {
            root["ptr2sym"].push_back({
                {"offset",     static_cast<uint64_t>(kv.first)},
                {"symbol",     std::get<0>(kv.second)},
                {"targetAddr", FormatHexU64(std::get<1>(kv.second))}
            });
        }

        for (auto& kv : manalyzer.get_ptr2local())
        {
            root["ptr2local"].push_back({
                {"fromOffset", static_cast<uint64_t>(kv.first)},
                {"toOffset",   static_cast<uint64_t>(kv.second)}
            });
        }

        for (auto& kv : manalyzer.get_ptr2heap())
        {
            root["ptr2heap"].push_back({
                {"offset",     static_cast<uint64_t>(kv.first)},
                {"targetAddr", FormatHexU64(std::get<0>(kv.second))},
                {"heapBase",   FormatHexU64(std::get<1>(kv.second))},
                {"heapSize",   static_cast<uint64_t>(std::get<2>(kv.second))}
            });
        }

        for (auto& kv : manalyzer.get_ptr2astr())
        {
            uint64_t off = (kv.first >= page_base) ? static_cast<uint64_t>(kv.first - page_base) : 0;
            root["ptr2astr"].push_back({{"offset", off}, {"text", kv.second}});
        }

        for (auto& kv : manalyzer.get_ptr2ustr())
        {
            uint64_t off = (kv.first >= page_base) ? static_cast<uint64_t>(kv.first - page_base) : 0;
            root["ptr2ustr"].push_back({{"offset", off}, {"text", kv.second}});
        }
    }
    catch (...) {}

    if (should_restore_pos)
    {
        try { DK_SEEK_TO(std::get<0>(original_pos), std::get<1>(original_pos)); }
        catch (...) {}
    }

    return root.dump();
}

std::string CDkEmbeddedServer::HandlePageSvgRoute(const std::string& query)
{
    bool is_ttd = DK_MODEL_ACCESS->isTTD();

    auto proc = DK_MODEL_ACCESS->get_current_process();
    if (proc == nullptr)
        return BuildJsonResponseError(503, "NO_PROCESS", "No active process");

    auto query_params = ParseQueryParams(query);

    // Parse dark theme flag
    bool dark_theme = false;
    {
        auto it = query_params.find("dark");
        if (it != query_params.end())
            dark_theme = (it->second == "1" || ToLowerAscii(it->second) == "true");
    }

    // Parse thread_id
    uint64_t requested_thread_id = 0;
    bool has_requested_thread = false;
    {
        auto it_thread = query_params.find("thread_id");
        if (it_thread == query_params.end())
            it_thread = query_params.find("threadId");
        if (it_thread != query_params.end())
            has_requested_thread = TryParseU64(it_thread->second, requested_thread_id);
    }

    bool should_restore_pos = false;
    std::tuple<uint64_t, uint64_t> original_pos{ 0, 0 };
    if (is_ttd)
    {
        auto it_time = query_params.find("time");
        if (it_time != query_params.end())
        {
            double requested_time = 0.0;
            if (TryParseDouble(it_time->second, requested_time))
            {
                try
                {
                    auto ttd = DK_MGET_POBJ(proc, "TTD");
                    auto lifetime = ttd != nullptr ? DK_MGET_POBJ(ttd, "Lifetime") : DK_MOBJ_PTR{};
                    if (lifetime != nullptr)
                    {
                        original_pos = DK_GET_CURPOS();
                        auto min_pos = DK_MGET_POS(lifetime, "MinPosition");
                        auto max_pos = DK_MGET_POS(lifetime, "MaxPosition");

                        double ratio = requested_time;
                        if (requested_time > 1.0)
                            ratio = requested_time / 10000.0;
                        ratio = std::max(0.0, std::min(1.0, ratio));

                        uint64_t seq = std::get<0>(min_pos);
                        if (std::get<0>(max_pos) > std::get<0>(min_pos))
                            seq = std::get<0>(min_pos) + static_cast<uint64_t>((std::get<0>(max_pos) - std::get<0>(min_pos)) * ratio);

                        uint64_t step = std::get<1>(min_pos);
                        if (std::get<1>(max_pos) > std::get<1>(min_pos))
                            step = std::get<1>(min_pos) + static_cast<uint64_t>((std::get<1>(max_pos) - std::get<1>(min_pos)) * ratio);

                        DK_SEEK_TO(seq, step);
                        should_restore_pos = true;
                    }
                }
                catch (...) {}
            }
        }
    }

    // Select thread
    DK_MOBJ_PTR selected_thread;
    auto threads_obj = DK_MGET_POBJ(proc, "Threads");
    if (threads_obj != nullptr)
    {
        auto threads = DK_MODEL_ACCESS->iterate(threads_obj);
        for (auto& thread_entry : threads)
        {
            auto thread_obj = std::get<1>(thread_entry);
            uint64_t tid = 0;
            try { tid = DK_MGET_PVAL<uint64_t, VT_UI8>(thread_obj, "Id"); }
            catch (...) { continue; }

            if (!has_requested_thread || tid == requested_thread_id)
            {
                selected_thread = thread_obj;
                break;
            }
        }
    }
    if (selected_thread == nullptr)
        selected_thread = DK_MODEL_ACCESS->get_current_thread();

    auto TryReadObjU64 = [](DK_MOBJ_PTR& obj, uint64_t& out) -> bool
    {
        try { out = DK_MGET_VAL<uint64_t, VT_UI8>(obj); return true; } catch (...) {}
        try { out = DK_MGET_VAL<DWORD, VT_UI4>(obj); return true; } catch (...) {}
        try { out = static_cast<uint64_t>(DK_MGET_VAL<int64_t, VT_I8>(obj)); return true; } catch (...) {}
        return false;
    };

    // Extract RSP
    uint64_t rsp_value = 0;
    bool has_rsp = false;
    if (selected_thread != nullptr)
    {
        auto regs_obj = DK_MGET_POBJ(selected_thread, "Registers");
        std::vector<DK_MOBJ_PTR> reg_roots;
        if (regs_obj != nullptr)
        {
            reg_roots.push_back(regs_obj);
            auto user_obj = DK_MGET_POBJ(regs_obj, "User");
            if (user_obj != nullptr) reg_roots.push_back(user_obj);
        }

        for (auto& reg_root : reg_roots)
        {
            for (const auto& name : std::vector<std::string>{"rsp", "Rsp", "RSP"})
            {
                try
                {
                    auto reg_obj = DK_MGET_POBJ(reg_root, name);
                    if (reg_obj != nullptr && TryReadObjU64(reg_obj, rsp_value))
                    {
                        has_rsp = (rsp_value != 0);
                        break;
                    }
                }
                catch (...) {}
            }
            if (has_rsp) break;
        }

        if (!has_rsp)
        {
            try
            {
                auto stack_obj = DK_MGET_POBJ(selected_thread, "Stack");
                if (stack_obj != nullptr)
                {
                    auto frames_obj = DK_MGET_POBJ(stack_obj, "Frames");
                    if (frames_obj != nullptr)
                    {
                        auto frames = DK_MODEL_ACCESS->iterate(frames_obj);
                        if (!frames.empty())
                        {
                            auto frame_attrs = DK_MGET_POBJ(std::get<1>(frames.front()), "Attributes");
                            if (frame_attrs != nullptr)
                            {
                                rsp_value = DK_MGET_PVAL<uint64_t, VT_UI8>(frame_attrs, "StackOffset");
                                has_rsp = (rsp_value != 0);
                            }
                        }
                    }
                }
            }
            catch (...) {}
        }
    }

    if (!has_rsp || rsp_value == 0)
    {
        if (should_restore_pos)
        {
            try { DK_SEEK_TO(std::get<0>(original_pos), std::get<1>(original_pos)); }
            catch (...) {}
        }
        return BuildJsonResponseError(503, "NO_RSP", "Could not read RSP register");
    }

    uint64_t page_base = rsp_value & 0xFFFFFFFFFFFFF000ULL;

    std::string svg;
    try
    {
        svg = page_to_svg_string(page_base, page_base, page_base + 0x1000, dark_theme);
    }
    catch (...)
    {
        if (should_restore_pos)
        {
            try { DK_SEEK_TO(std::get<0>(original_pos), std::get<1>(original_pos)); }
            catch (...) {}
        }
        return BuildJsonResponseError(500, "SVG_FAILED", "page_to_svg_string threw an exception");
    }

    if (should_restore_pos)
    {
        try { DK_SEEK_TO(std::get<0>(original_pos), std::get<1>(original_pos)); }
        catch (...) {}
    }

    std::stringstream ss;
    ss << "HTTP/1.1 200 OK\r\n"
       << "Content-Type: image/svg+xml; charset=utf-8\r\n"
       << "Connection: close\r\n"
       << "Cache-Control: no-store\r\n"
       << "Content-Length: " << svg.size() << "\r\n\r\n"
       << svg;

    return ss.str();
}
