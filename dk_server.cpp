#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Unknwn.h>
#include <oaidl.h>
#include <winnt.h>

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
#include <unordered_set>
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
        std::string body;
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

        // Extract body: everything after the first \\r\\n\\r\\n
        size_t body_start = request.find("\\r\\n\\r\\n");
        std::string body;
        if (body_start != std::string::npos)
            body = request.substr(body_start + 4);

        return { method, path, query, body };
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

    std::string JoinLines(const std::vector<std::string>& lines)
    {
        std::ostringstream stream;
        for (size_t index = 0; index < lines.size(); ++index)
        {
            if (index > 0)
                stream << "\n";
            stream << lines[index];
        }

        return stream.str();
    }

    void ReplaceAll(std::string& value, const std::string& needle, const std::string& replacement)
    {
        if (needle.empty())
            return;

        size_t start = 0;
        while ((start = value.find(needle, start)) != std::string::npos)
        {
            value.replace(start, needle.size(), replacement);
            start += replacement.size();
        }
    }

    std::string TrimAsciiWhitespace(const std::string& value)
    {
        if (value.empty())
            return value;

        size_t start = 0;
        while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])) != 0)
            ++start;

        size_t end = value.size();
        while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])) != 0)
            --end;

        return value.substr(start, end - start);
    }

    std::string SanitizeCommandOutputLine(std::string line)
    {
        if (line.empty())
            return line;

        ReplaceAll(line, "\r", "");
        ReplaceAll(line, "</link>", "");
        ReplaceAll(line, "</col>", "");

        if (line.find("<link") != std::string::npos || line.find("cmd=\"") != std::string::npos)
        {
            size_t marker = line.rfind("\">");
            if (marker != std::string::npos)
                line = line.substr(marker + 2);
        }

        if (line.find("Debugger.Utility.Objects.Aggregate(") != std::string::npos)
        {
            size_t marker = line.rfind("\">");
            if (marker != std::string::npos)
                line = line.substr(marker + 2);
        }

        if (line.find("<col ") != std::string::npos)
        {
            std::string flattened;
            flattened.reserve(line.size());
            bool in_tag = false;
            for (char ch : line)
            {
                if (ch == '<')
                {
                    in_tag = true;
                    continue;
                }
                if (ch == '>')
                {
                    in_tag = false;
                    continue;
                }
                if (!in_tag)
                    flattened.push_back(ch);
            }
            line = flattened;
        }

        return TrimAsciiWhitespace(line);
    }

    std::vector<std::string> SanitizeCommandOutputLines(const std::vector<std::string>& lines)
    {
        std::vector<std::string> cleaned;
        cleaned.reserve(lines.size());

        for (const auto& line : lines)
        {
            cleaned.push_back(SanitizeCommandOutputLine(line));
        }

        return cleaned;
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

    bool SeekToQueryPosition(
        bool is_ttd,
        DK_MOBJ_PTR& proc,
        const std::map<std::string, std::string>& query_params,
        std::tuple<uint64_t, uint64_t>& original_pos,
        bool& should_restore_pos)
    {
        if (!is_ttd || proc == nullptr)
            return false;

        try
        {
            auto ttd = DK_MGET_POBJ(proc, "TTD");
            auto lifetime = ttd != nullptr ? DK_MGET_POBJ(ttd, "Lifetime") : DK_MOBJ_PTR{};
            if (lifetime == nullptr)
                return false;

            auto min_pos = DK_MGET_POS(lifetime, "MinPosition");
            auto max_pos = DK_MGET_POS(lifetime, "MaxPosition");

            uint64_t seq = 0;
            uint64_t step = 0;
            bool has_major = false;
            bool has_minor = false;

            auto it_major = query_params.find("major");
            if (it_major != query_params.end())
                has_major = TryParseU64(it_major->second, seq);

            auto it_minor = query_params.find("minor");
            if (it_minor != query_params.end())
                has_minor = TryParseU64(it_minor->second, step);

            if (has_major)
            {
                if (!has_minor)
                    step = 0;

                const uint64_t min_major = std::get<0>(min_pos);
                const uint64_t min_minor = std::get<1>(min_pos);
                const uint64_t max_major = std::get<0>(max_pos);
                const uint64_t max_minor = std::get<1>(max_pos);

                const bool below_min = (seq < min_major) || (seq == min_major && step < min_minor);
                const bool above_max = (seq > max_major) || (seq == max_major && step > max_minor);

                if (below_min)
                {
                    seq = min_major;
                    step = min_minor;
                }
                else if (above_max)
                {
                    seq = max_major;
                    step = max_minor;
                }

                original_pos = DK_GET_CURPOS();
                DK_SEEK_TO(seq, step);
                should_restore_pos = true;
                return true;
            }

            auto it_time = query_params.find("time");
            if (it_time == query_params.end())
                return false;

            double requested_time = 0.0;
            if (!TryParseDouble(it_time->second, requested_time))
                return false;

            double ratio = requested_time;
            if (requested_time > 1.0)
                ratio = requested_time / 10000.0;
            ratio = std::max(0.0, std::min(1.0, ratio));

            seq = std::get<0>(min_pos);
            if (std::get<0>(max_pos) > std::get<0>(min_pos))
            {
                double span = static_cast<double>(std::get<0>(max_pos) - std::get<0>(min_pos));
                seq = std::get<0>(min_pos) + static_cast<uint64_t>(span * ratio);
            }

            step = std::get<1>(min_pos);
            if (std::get<1>(max_pos) > std::get<1>(min_pos))
            {
                double span = static_cast<double>(std::get<1>(max_pos) - std::get<1>(min_pos));
                step = std::get<1>(min_pos) + static_cast<uint64_t>(span * ratio);
            }

            original_pos = DK_GET_CURPOS();
            DK_SEEK_TO(seq, step);
            should_restore_pos = true;
            return true;
        }
        catch (...)
        {
            return false;
        }
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
        const std::string& body = parsed.body;

        // Log request details with timestamp
        auto request_time = std::chrono::system_clock::now();
        auto time_t_c = std::chrono::system_clock::to_time_t(request_time);
        struct tm timeinfo;
        localtime_s(&timeinfo, &time_t_c);
        char timestamp_buf[32];
        strftime(timestamp_buf, sizeof(timestamp_buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        
        // Measure response generation time
        uint64_t start_ms = NowMs();
        std::string response = BuildHttpResponse(method, path, query, body);
        uint64_t duration_ms = NowMs() - start_ms;

        // Extract status code and Content-Length from response
        int status_code = 0;
        size_t content_length = 0;
        std::string content_type = "application/json";
        {
            size_t sp = response.find(' ');
            if (sp != std::string::npos && sp + 3 < response.size())
                status_code = std::stoi(response.substr(sp + 1, 3));
            
            // Extract Content-Length header
            size_t cl_pos = response.find("Content-Length: ");
            if (cl_pos != std::string::npos)
            {
                size_t cl_end = response.find("\r\n", cl_pos);
                if (cl_end != std::string::npos)
                {
                    try {
                        content_length = std::stoul(response.substr(cl_pos + 16, cl_end - (cl_pos + 16)));
                    } catch (...) { content_length = 0; }
                }
            }
            
            // Extract Content-Type header
            size_t ct_pos = response.find("Content-Type: ");
            if (ct_pos != std::string::npos)
            {
                size_t ct_end = response.find("\r\n", ct_pos);
                if (ct_end != std::string::npos)
                {
                    content_type = response.substr(ct_pos + 14, ct_end - (ct_pos + 14));
                }
            }
        }

        // Extract format short name from content-type
        std::string format_name = "JSON";
        if (content_type.find("svg") != std::string::npos) format_name = "SVG";
        else if (content_type.find("text/html") != std::string::npos) format_name = "HTML";
        else if (content_type.find("text/plain") != std::string::npos) format_name = "TXT";

        // Build neat log message with padding: [status] METHOD PATH | ==> +Xms FORMAT SIZE
        std::string path_and_query = path;
        if (!query.empty()) path_and_query += "?" + query;
        
        // Pad path to 40 chars for alignment
        while (path_and_query.length() < 40)
            path_and_query += " ";
        
        std::ostringstream log_msg;
        log_msg << "[dk_serve] [" << timestamp_buf << "] "
                << "[" << status_code << "] "
                << method << " " << path_and_query
                << "| ==> +" << duration_ms << "ms   "
                << std::left << std::setw(6) << format_name
                << content_length << "B";
        
        EXT_F_OUT("%s\n", log_msg.str().c_str());
        
        // Periodic status log every 60 seconds
        static uint64_t last_status_ms = NowMs();
        uint64_t now_ms = NowMs();
        if (now_ms - last_status_ms >= 60000)  // 60 seconds
        {
            last_status_ms = now_ms;
            EXT_F_OUT("[dk serve] [PERIODIC STATUS] Server running, last request: %s %s (status %d)\n",
                     method.c_str(), path.c_str(), status_code);
        }

        send(client_socket, response.c_str(), static_cast<int>(response.size()), 0);
    }
    catch (...)
    {
        std::string response = BuildJsonResponseError(500, "INTERNAL_ERROR", "Unhandled server exception");
        send(client_socket, response.c_str(), static_cast<int>(response.size()), 0);
    }

    closesocket(client_socket);
}

std::string CDkEmbeddedServer::BuildHttpResponse(const std::string& method, const std::string& path, const std::string& query, const std::string& body)
{
    if (method != "GET" && !(method == "POST" && path == "/api/command/execute"))
    {
        return BuildJsonResponseError(405, "METHOD_NOT_ALLOWED",
            "Only GET is supported (POST accepted for /api/command/execute)");
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

    if (path == "/api/function-calls")
        return BuildJsonResponseOk(HandleFunctionCallsRoute(query));

    if (path == "/api/command/execute")
        return BuildJsonResponseOk(HandleCommandRoute(query, body));

    if (path == "/api/model")
        return BuildJsonResponseOk(HandleModelRoute(query));

    if (path == "/api/pe")
        return BuildJsonResponseOk(HandlePeRoute(query));

    if (path == "/api/strings")
        return BuildJsonResponseOk(HandleStringsRoute(query));

    if (path == "/api/memory/layout")
        return BuildJsonResponseOk(HandleMemoryLayoutRoute(query));

    if (path == "/api/environment")
        return BuildJsonResponseOk(HandleEnvironmentRoute(query));

    if (path == "/api/capabilities")
        return BuildJsonResponseOk(HandleCapabilitiesRoute());

    if (path == "/api/ttd/mem-access")
        return BuildJsonResponseOk(HandleTtdMemAccessRoute(query));

    if (path == "/")
    {
        json root = {
            {"schemaVersion", "1.0"},
            {"name", "dk embedded server"},
            {"phase", 2},
            {"routes", json::array({"/api/server/status", "/api/server/stop", "/api/ttd/trace-info", "/api/ttd/modules", "/api/ttd/threads", "/api/ttd/events/lifetime", "/api/registers", "/api/callstack", "/api/page", "/api/page/svg", "/api/function-calls", "/api/command/execute", "/api/model", "/api/pe", "/api/strings", "/api/memory/layout", "/api/environment", "/api/capabilities", "/api/ttd/mem-access"})}
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
    (void)SeekToQueryPosition(is_ttd, proc, query_params, original_pos, should_restore_pos);

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
    (void)SeekToQueryPosition(is_ttd, proc, query_params, original_pos, should_restore_pos);

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

    uint64_t requested_address = 0;
    bool has_requested_address = false;
    bool address_param_supplied = false;
    {
        auto it_address = query_params.find("address");
        if (it_address == query_params.end()) it_address = query_params.find("addr");
        if (it_address == query_params.end()) it_address = query_params.find("va");
        if (it_address == query_params.end()) it_address = query_params.find("page_base");
        if (it_address == query_params.end()) it_address = query_params.find("page");

        if (it_address != query_params.end())
        {
            address_param_supplied = true;
            has_requested_address = TryParseU64(TrimAsciiWhitespace(it_address->second), requested_address);
        }
    }

    bool should_restore_pos = false;
    std::tuple<uint64_t, uint64_t> original_pos{ 0, 0 };
    (void)SeekToQueryPosition(is_ttd, proc, query_params, original_pos, should_restore_pos);

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

    if (address_param_supplied && !has_requested_address)
    {
        root["error"] = "Invalid address query parameter";
        if (should_restore_pos)
        {
            try { DK_SEEK_TO(std::get<0>(original_pos), std::get<1>(original_pos)); }
            catch (...) {}
        }
        return root.dump();
    }

    uint64_t effective_address = 0;
    if (has_requested_address && requested_address != 0)
    {
        effective_address = requested_address;
    }
    else if (has_rsp && rsp_value != 0)
    {
        effective_address = rsp_value;
    }

    if (effective_address == 0)
    {
        if (should_restore_pos)
        {
            try { DK_SEEK_TO(std::get<0>(original_pos), std::get<1>(original_pos)); }
            catch (...) {}
        }
        return root.dump();
    }

    uint64_t page_base = effective_address & 0xFFFFFFFFFFFFF000ULL;

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
    root["rsp"]       = FormatHexU64(has_rsp ? rsp_value : effective_address);
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

    // Parse optional explicit page address.
    uint64_t requested_address = 0;
    bool has_requested_address = false;
    {
        auto it_addr = query_params.find("address");
        if (it_addr == query_params.end()) it_addr = query_params.find("addr");
        if (it_addr == query_params.end()) it_addr = query_params.find("va");
        if (it_addr == query_params.end()) it_addr = query_params.find("page_base");
        if (it_addr == query_params.end()) it_addr = query_params.find("page");
        if (it_addr != query_params.end())
            has_requested_address = TryParseU64(TrimAsciiWhitespace(it_addr->second), requested_address);

        if (it_addr != query_params.end() && !has_requested_address)
            return BuildJsonResponseError(400, "BAD_ADDRESS", "Query parameter 'address' is invalid");
    }

    bool should_restore_pos = false;
    std::tuple<uint64_t, uint64_t> original_pos{ 0, 0 };
    (void)SeekToQueryPosition(is_ttd, proc, query_params, original_pos, should_restore_pos);

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

    // Extract RSP only when no explicit page address is provided.
    uint64_t rsp_value = 0;
    bool has_rsp = has_requested_address;
    if (!has_requested_address && selected_thread != nullptr)
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

    if (!has_rsp || (!has_requested_address && rsp_value == 0))
    {
        if (should_restore_pos)
        {
            try { DK_SEEK_TO(std::get<0>(original_pos), std::get<1>(original_pos)); }
            catch (...) {}
        }
        return BuildJsonResponseError(503, "NO_RSP", "Could not read RSP register");
    }

    uint64_t page_base = has_requested_address
        ? (requested_address & 0xFFFFFFFFFFFFF000ULL)
        : (rsp_value & 0xFFFFFFFFFFFFF000ULL);

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

std::string CDkEmbeddedServer::HandleCommandRoute(const std::string& query, const std::string& body)
{
    uint64_t request_id = ++m_request_counter;
    uint64_t start_time_ms = NowMs();

    auto params = ParseQueryParams(query);
    std::string command;

    // For POST requests, try to parse the JSON body for the command
    if (!body.empty())
    {
        try
        {
            auto body_json = json::parse(body);
            auto cmd_iter = body_json.find("command");
            if (cmd_iter != body_json.end() && cmd_iter->is_string())
                command = cmd_iter->get<std::string>();
        }
        catch (...)
        {
            // If JSON parsing fails, fall back to query params
        }
    }

    if (command.empty())
    {
        auto command_iter = params.find("command");
        if (command_iter != params.end())
            command = command_iter->second;
    }

    if (command.empty())
    {
        auto alias_iter = params.find("cmd");
        if (alias_iter != params.end())
            command = alias_iter->second;
    }

    // Parse optional timeoutMs (for documentation/client-side use;
    // the underlying execute_cmd is blocking and cannot be interrupted
    // from the same thread, so the server records the intent but the
    // actual timeout enforcement happens at the HTTP/MCP layer.)
    uint64_t timeout_ms = 0;
    {
        auto it = params.find("timeoutMs");
        if (it == params.end())
            it = params.find("timeout");
        if (it != params.end())
        {
            uint64_t parsed = 0;
            if (TryParseU64(it->second, parsed) && parsed > 0)
                timeout_ms = parsed;
        }
    }

    auto lines = command.empty()
        ? std::vector<std::string>{}
        : DK_MODEL_ACCESS->execute_cmd(command);
    auto sanitized_lines = SanitizeCommandOutputLines(lines);

    uint64_t elapsed_ms = NowMs() - start_time_ms;

    json root = {
        {"schemaVersion", "1.0"},
        {"requestId", std::to_string(request_id)},
        {"command", {
            {"input", command},
            {"lineCount", sanitized_lines.size()},
            {"lines", sanitized_lines},
            {"output", JoinLines(sanitized_lines)}
        }},
        {"timing", {
            {"elapsedMs", elapsed_ms}
        }}
    };

    if (timeout_ms > 0)
        root["timing"]["timeoutMs"] = timeout_ms;

    if (command.empty())
    {
        root["command"]["message"] = "Query parameter 'command' is required.";
    }

    return root.dump();
}

std::string CDkEmbeddedServer::HandleModelRoute(const std::string& query)
{
    uint64_t request_id = ++m_request_counter;

    auto params = ParseQueryParams(query);

    auto ReadParam = [&](const std::initializer_list<const char*>& keys, const std::string& fallback = "") -> std::string {
        for (const auto* key : keys)
        {
            auto it = params.find(key);
            if (it != params.end())
            {
                std::string value = TrimAsciiWhitespace(it->second);
                if (!value.empty())
                    return value;
            }
        }
        return fallback;
    };

    auto ToLower = [](std::string text) -> std::string {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char ch) {
            return static_cast<char>(std::tolower(ch));
        });
        return text;
    };

    auto ParseBool = [&](const std::string& value, bool fallback) -> bool {
        if (value.empty()) return fallback;
        const std::string lower = ToLower(value);
        if (lower == "1" || lower == "true" || lower == "yes" || lower == "on") return true;
        if (lower == "0" || lower == "false" || lower == "no" || lower == "off") return false;
        return fallback;
    };

    auto FormatHex = [](uint64_t value) -> std::string {
        std::ostringstream stream;
        stream << "0x" << std::hex << std::nouppercase << value;
        return stream.str();
    };

    auto VarTypeName = [](VARTYPE vt) -> std::string {
        switch (vt)
        {
        case VT_EMPTY: return "EMPTY";
        case VT_NULL: return "NULL";
        case VT_I1: return "I1";
        case VT_I2: return "I2";
        case VT_I4: return "I4";
        case VT_I8: return "I8";
        case VT_UI1: return "UI1";
        case VT_UI2: return "UI2";
        case VT_UI4: return "UI4";
        case VT_UI8: return "UI8";
        case VT_INT: return "INT";
        case VT_UINT: return "UINT";
        case VT_BOOL: return "BOOL";
        case VT_BSTR: return "BSTR";
        case VT_R4: return "R4";
        case VT_R8: return "R8";
        case VT_UNKNOWN: return "UNKNOWN";
        case VT_DISPATCH: return "DISPATCH";
        default:
            {
                std::ostringstream stream;
                stream << "VT(" << vt << ")";
                return stream.str();
            }
        }
    };

    auto VariantToString = [&](VARIANT& value) -> std::string {
        std::ostringstream stream;
        switch (value.vt)
        {
        case VT_EMPTY:
            return "";
        case VT_NULL:
            return "null";
        case VT_BOOL:
            return value.boolVal == VARIANT_TRUE ? "true" : "false";
        case VT_UI1:
            stream << static_cast<unsigned int>(value.bVal);
            return stream.str();
        case VT_UI2:
            stream << value.uiVal;
            return stream.str();
        case VT_UI4:
            stream << value.ulVal << " (" << FormatHex(value.ulVal) << ")";
            return stream.str();
        case VT_UI8:
            stream << value.ullVal << " (" << FormatHex(value.ullVal) << ")";
            return stream.str();
        case VT_I1:
            stream << static_cast<int>(value.cVal);
            return stream.str();
        case VT_I2:
            stream << value.iVal;
            return stream.str();
        case VT_I4:
        case VT_INT:
            stream << value.intVal;
            return stream.str();
        case VT_I8:
            stream << value.llVal;
            return stream.str();
        case VT_UINT:
            stream << value.uintVal;
            return stream.str();
        case VT_R4:
            stream << value.fltVal;
            return stream.str();
        case VT_R8:
            stream << value.dblVal;
            return stream.str();
        case VT_BSTR:
            return BSTR2str(value.bstrVal);
        default:
            return VarTypeName(value.vt);
        }
    };

    auto KindToString = [](ModelObjectKind kind) -> std::string {
        auto it = CModelAccess::s_map_kind_name.find(kind);
        if (it != CModelAccess::s_map_kind_name.end())
            return it->second;
        return "Unknown";
    };

    auto expr = ReadParam({ "expr", "expression", "path", "query" }, "@$cursession");
    auto trimmed_expr = TrimAsciiWhitespace(expr);

    uint64_t depth = 2;
    auto depth_text = ReadParam({ "depth", "r" }, "");
    if (!depth_text.empty())
    {
        uint64_t parsed = 0;
        if (TryParseU64(depth_text, parsed))
            depth = std::min<uint64_t>(8, parsed);
    }

    auto BuildPropertyPath = [](const std::string& parent, const std::string& child) -> std::string {
        if (parent.empty()) return child;
        if (child.empty()) return parent;
        return parent + "." + child;
    };

    auto SplitMethodPath = [](const std::string& fullPath) -> std::pair<std::string, std::string> {
        int bracket_depth = 0;
        for (int i = static_cast<int>(fullPath.size()) - 1; i >= 0; --i)
        {
            const char ch = fullPath[static_cast<size_t>(i)];
            if (ch == ']')
            {
                ++bracket_depth;
                continue;
            }
            if (ch == '[')
            {
                --bracket_depth;
                continue;
            }
            if (ch == '.' && bracket_depth == 0)
            {
                return { fullPath.substr(0, static_cast<size_t>(i)), fullPath.substr(static_cast<size_t>(i + 1)) };
            }
        }
        return { "", fullPath };
    };

    auto ParseArgList = [&](const std::string& text) -> std::vector<std::string> {
        std::vector<std::string> args;
        std::string current;
        bool in_quotes = false;
        char quote_char = '\0';

        auto PushCurrent = [&]() {
            const std::string trimmed = TrimAsciiWhitespace(current);
            if (!trimmed.empty())
                args.push_back(trimmed);
            current.clear();
        };

        for (size_t i = 0; i < text.size(); ++i)
        {
            const char ch = text[i];
            if ((ch == '"' || ch == '\'') && (i == 0 || text[i - 1] != '\\'))
            {
                if (!in_quotes)
                {
                    in_quotes = true;
                    quote_char = ch;
                    continue;
                }
                if (quote_char == ch)
                {
                    in_quotes = false;
                    quote_char = '\0';
                    continue;
                }
            }

            if (!in_quotes && ch == ',')
            {
                PushCurrent();
                continue;
            }

            current.push_back(ch);
        }

        PushCurrent();

        return args;
    };

    auto ParseInvocationExpression = [&](const std::string& expression, std::string& method_path, std::vector<std::string>& args) -> bool {
        method_path.clear();
        args.clear();

        const std::string trimmed = TrimAsciiWhitespace(expression);
        if (trimmed.size() < 3 || trimmed.back() != ')')
            return false;

        const size_t open_paren = trimmed.find_last_of('(');
        if (open_paren == std::string::npos || open_paren == 0)
            return false;

        method_path = TrimAsciiWhitespace(trimmed.substr(0, open_paren));
        if (method_path.empty())
            return false;

        const std::string arg_text = trimmed.substr(open_paren + 1, trimmed.size() - open_paren - 2);
        args = ParseArgList(arg_text);
        return true;
    };

    auto ResolveRelative = [&](DK_MOBJ_PTR root, const std::string& relativePath) -> DK_MOBJ_PTR {
        if (root == nullptr)
            return nullptr;
        if (relativePath.empty())
            return root;
        return DK_MODEL_ACCESS->get_pobj_tree(root, relativePath);
    };

    auto ResolveModelObject = [&](const std::string& expression) -> DK_MOBJ_PTR {
        const std::string trimmed = TrimAsciiWhitespace(expression);
        if (trimmed.empty())
            return nullptr;

        if (trimmed == "@$cursession")
            return DK_MODEL_ACCESS->get_current_session();
        if (trimmed.rfind("@$cursession.", 0) == 0)
            return ResolveRelative(DK_MODEL_ACCESS->get_current_session(), trimmed.substr(13));

        if (trimmed == "@$curprocess")
            return DK_MODEL_ACCESS->get_current_process();
        if (trimmed.rfind("@$curprocess.", 0) == 0)
            return ResolveRelative(DK_MODEL_ACCESS->get_current_process(), trimmed.substr(13));

        if (trimmed == "@$curthread")
            return DK_MODEL_ACCESS->get_current_thread();
        if (trimmed.rfind("@$curthread.", 0) == 0)
            return ResolveRelative(DK_MODEL_ACCESS->get_current_thread(), trimmed.substr(12));

        if (trimmed == "@$curstack")
            return DK_MODEL_ACCESS->get_current_stack();
        if (trimmed.rfind("@$curstack.", 0) == 0)
            return ResolveRelative(DK_MODEL_ACCESS->get_current_stack(), trimmed.substr(11));

        if (trimmed == "@$curframe")
            return DK_MODEL_ACCESS->get_current_frame();
        if (trimmed.rfind("@$curframe.", 0) == 0)
            return ResolveRelative(DK_MODEL_ACCESS->get_current_frame(), trimmed.substr(11));

        auto named = DK_MODEL_ACCESS->path2mobj(trimmed);
        if (named != nullptr)
            return named;

        return DK_MODEL_ACCESS->get_mobj_tree(trimmed);
    };

    auto SummarizeObject = [&](const std::string& path, const std::string& label, DK_MOBJ_PTR object) -> json {
        json summary = {
            {"label", label},
            {"path", path},
            {"kind", "Unavailable"},
            {"value", ""},
            {"valueType", ""},
            {"address", ""},
            {"display", "Unavailable"},
            {"browsable", object != nullptr}
        };

        if (object == nullptr)
        {
            summary["error"] = "Object unavailable";
            return summary;
        }

        summary["kind"] = KindToString(DK_MODEL_ACCESS->get_kind(object));

        Location location = {};
        if (SUCCEEDED(object->GetLocation(&location)) && location.Offset != 0)
            summary["address"] = FormatHex(location.Offset);

        VARIANT intrinsic = {};
        VariantInit(&intrinsic);
        if (SUCCEEDED(object->GetIntrinsicValue(&intrinsic)))
        {
            summary["valueType"] = VarTypeName(intrinsic.vt);
            summary["value"] = VariantToString(intrinsic);
            VariantClear(&intrinsic);
        }

        const std::string value = summary["value"].get<std::string>();
        const std::string address = summary["address"].get<std::string>();
        if (!value.empty())
            summary["display"] = value;
        else if (!address.empty())
            summary["display"] = address;
        else
            summary["display"] = summary["kind"];

        return summary;
    };

    json roots = json::array({
        "Debugger",
        "Debugger.Sessions",
        "@$cursession",
        "@$cursession.Processes",
        "@$curprocess",
        "@$curprocess.Threads",
        "@$curprocess.Modules",
        "@$curthread",
        "@$curstack",
        "@$curframe"
    });

    json snippets = json::array({
        { {"label", "List Sessions"}, {"expr", "Debugger.Sessions"}, {"mode", "browse"}, {"depth", 2} },
        { {"label", "Current Session"}, {"expr", "@$cursession"}, {"mode", "browse"}, {"depth", 2} },
        { {"label", "Session Processes"}, {"expr", "@$cursession.Processes"}, {"mode", "browse"}, {"depth", 2} },
        { {"label", "Current Process"}, {"expr", "@$curprocess"}, {"mode", "browse"}, {"depth", 2} },
        { {"label", "Current Thread"}, {"expr", "@$curthread"}, {"mode", "browse"}, {"depth", 2} },
        { {"label", "Current Process Threads"}, {"expr", "@$curprocess.Threads"}, {"mode", "browse"}, {"depth", 2} },
        { {"label", "Current Process Modules"}, {"expr", "@$curprocess.Modules"}, {"mode", "browse"}, {"depth", 2} },
        { {"label", "Current Stack"}, {"expr", "@$curstack"}, {"mode", "browse"}, {"depth", 1} },
        { {"label", "Current Frame"}, {"expr", "@$curframe"}, {"mode", "browse"}, {"depth", 1} },
        { {"label", "Process Memory"}, {"expr", "@$curprocess.Memory"}, {"mode", "browse"}, {"depth", 2} }
    });

    json notes = json::array({
        "Model Explorer uses direct model-object access and collection traversal. It no longer relies on dx text rendering.",
        "Prefer @$cursession / @$curprocess / @$curthread over Debugger.Sessions[0].Processes[0] style indexing. Indexed collections can be unavailable for some targets and may raise 0x8000000b.",
        "Invoke methods with expression syntax: <path.to.Method>() or <path.to.Method>(arg1, \"arg2\", true)."
    });

    const bool explicit_invoke = ParseBool(ReadParam({ "invoke" }, ""), false);
    const std::vector<std::string> explicit_args = ParseArgList(ReadParam({ "args" }, ""));

    json model = {
        {"expression", trimmed_expr},
        {"depth", depth},
        {"available", false},
        {"invocation", {
            {"requested", false},
            {"target", ""},
            {"args", json::array()},
            {"succeeded", false}
        }},
        {"roots", roots},
        {"snippets", snippets},
        {"notes", notes},
        {"summary", nullptr},
        {"properties", json::array()},
        {"items", json::array()},
        {"limits", {
            {"propertyLimit", 64},
            {"itemLimit", 64}
        }}
    };

    if (trimmed_expr.empty())
    {
        model["error"] = "Expression is required.";
    }
    else if (trimmed_expr.rfind("dx ", 0) == 0 || trimmed_expr.rfind("dx\t", 0) == 0)
    {
        model["error"] = "Model Explorer accepts model paths and aliases only, not raw dx commands.";
    }
    else
    {
        bool invocation_requested = false;
        std::string invocation_target = trimmed_expr;
        std::vector<std::string> invocation_args;

        if (ParseInvocationExpression(trimmed_expr, invocation_target, invocation_args))
        {
            invocation_requested = true;
        }
        else if (explicit_invoke)
        {
            invocation_requested = true;
            invocation_target = trimmed_expr;
            invocation_args = explicit_args;
        }

        model["invocation"]["requested"] = invocation_requested;
        model["invocation"]["target"] = invocation_target;
        model["invocation"]["args"] = invocation_args;

        DK_MOBJ_PTR object = nullptr;

        if (invocation_requested)
        {
            auto method_object = ResolveModelObject(invocation_target);
            if (method_object == nullptr)
            {
                model["error"] = "Unable to resolve method path in the current debugging context.";
            }
            else
            {
                std::vector<DK_MOBJ_PTR> arg_objects;
                bool args_ok = true;
                for (const auto& arg : invocation_args)
                {
                    const std::string raw_arg = TrimAsciiWhitespace(arg);

                    auto CreateStringArg = [&](std::string value) -> DK_MOBJ_PTR {
                        return DK_MODEL_ACCESS->create_str_intrinsic_obj(value);
                    };

                    auto ParseSignedI64 = [](const std::string& value, int64_t& out) -> bool {
                        if (value.empty())
                            return false;
                        char* end_ptr = nullptr;
                        const long long parsed = std::strtoll(value.c_str(), &end_ptr, 0);
                        if (end_ptr == value.c_str() || (end_ptr != nullptr && *end_ptr != '\0'))
                            return false;
                        out = static_cast<int64_t>(parsed);
                        return true;
                    };

                    DK_MOBJ_PTR arg_obj;
                    if (raw_arg.size() >= 2 && ((raw_arg.front() == '"' && raw_arg.back() == '"') || (raw_arg.front() == '\'' && raw_arg.back() == '\'')))
                    {
                        arg_obj = CreateStringArg(raw_arg.substr(1, raw_arg.size() - 2));
                    }
                    else
                    {
                        const std::string lower = ToLower(raw_arg);
                        if (lower == "true" || lower == "false")
                        {
                            arg_obj = DK_MODEL_ACCESS->create_bool_intrinsic_obj(lower == "true");
                        }
                        else
                        {
                            uint64_t unsigned_val = 0;
                            int64_t signed_val = 0;
                            if (TryParseU64(raw_arg, unsigned_val))
                            {
                                arg_obj = DK_MODEL_ACCESS->create_u64_intrinsic_obj(unsigned_val);
                            }
                            else if (ParseSignedI64(raw_arg, signed_val))
                            {
                                arg_obj = DK_MODEL_ACCESS->create_i64_intrinsic_obj(signed_val);
                            }
                            else
                            {
                                arg_obj = CreateStringArg(raw_arg);
                            }
                        }
                    }

                    if (arg_obj == nullptr)
                    {
                        args_ok = false;
                        break;
                    }
                    arg_objects.push_back(arg_obj);
                }

                if (!args_ok)
                {
                    model["error"] = "Failed to build invocation argument objects.";
                }
                else
                {
                    auto path_parts = SplitMethodPath(invocation_target);
                    DK_MOBJ_PTR context_object = path_parts.first.empty()
                        ? method_object
                        : ResolveModelObject(path_parts.first);
                    if (context_object == nullptr)
                        context_object = method_object;

                    auto call_result = DK_MODEL_ACCESS->call(method_object, context_object, arg_objects);
                    if (call_result == nullptr)
                    {
                        model["error"] = "Method invocation failed. Verify method path, context, and parameter types.";
                    }
                    else
                    {
                        object = call_result;
                        model["invocation"]["succeeded"] = true;
                        model["notes"].push_back("Method invoked through direct model-method call.");
                    }
                }
            }
        }
        else
        {
            object = ResolveModelObject(trimmed_expr);
        }

        if (object == nullptr)
        {
            if (!model.contains("error"))
                model["error"] = "Unable to resolve model path in the current debugging context.";
        }
        else
        {
            auto property_entries = DK_MODEL_ACCESS->enum_keys(object);
            auto item_entries = DK_MODEL_ACCESS->iterate(object);

            const std::string summary_path = invocation_requested
                ? (invocation_target + "()")
                : trimmed_expr;

            model["available"] = true;
            model["summary"] = SummarizeObject(summary_path, summary_path, object);
            model["summary"]["propertyCount"] = property_entries.size();
            model["summary"]["itemCount"] = item_entries.size();

            if (depth > 0)
            {
                const size_t property_limit = 64;
                const size_t item_limit = 64;

                size_t property_count = 0;
                for (auto& entry : property_entries)
                {
                    if (property_count++ >= property_limit)
                        break;

                    const std::string name = std::get<0>(entry);
                    const std::string path = BuildPropertyPath(summary_path, name);
                    json summary = SummarizeObject(path, name, std::get<2>(entry));
                    summary["declaredKind"] = std::get<1>(entry);
                    model["properties"].push_back(summary);
                }

                size_t item_count = 0;
                for (auto& entry : item_entries)
                {
                    if (item_count++ >= item_limit)
                        break;

                    const uint64_t index = std::get<0>(entry);
                    std::ostringstream label;
                    label << "[" << index << "]";
                    std::ostringstream path;
                    path << summary_path << "[" << index << "]";
                    json summary = SummarizeObject(path.str(), label.str(), std::get<1>(entry));
                    summary["index"] = index;
                    model["items"].push_back(summary);
                }

                if (property_entries.size() > property_limit)
                {
                    std::ostringstream message;
                    message << "Properties truncated to first " << property_limit << " entries.";
                    model["notes"].push_back(message.str());
                }

                if (item_entries.size() > item_limit)
                {
                    std::ostringstream message;
                    message << "Collection items truncated to first " << item_limit << " entries.";
                    model["notes"].push_back(message.str());
                }
            }
        }
    }

    json root = {
        {"schemaVersion", "1.0"},
        {"requestId", std::to_string(request_id)},
        {"model", model}
    };

    return root.dump();
}

std::string CDkEmbeddedServer::HandlePeRoute(const std::string& query)
{
    uint64_t request_id = ++m_request_counter;

    auto FormatHex = [](uint64_t value) -> std::string {
        std::ostringstream stream;
        stream << "0x" << std::hex << std::nouppercase << value;
        return stream.str();
    };

    auto FormatHex32 = [](uint32_t value) -> std::string {
        std::ostringstream stream;
        stream << "0x" << std::hex << std::nouppercase << value;
        return stream.str();
    };

    auto ReadVirtualExact = [](uint64_t address, void* buffer, size_t size) -> bool {
        ULONG bytes_read = 0;
        if (address == 0 || buffer == nullptr || size == 0)
            return false;
        if (EXT_D_IDebugDataSpaces == nullptr)
            return false;
        HRESULT hr = EXT_D_IDebugDataSpaces->ReadVirtual(address, buffer, static_cast<ULONG>(size), &bytes_read);
        return SUCCEEDED(hr) && bytes_read == size;
    };

    auto ReadString = [&](uint64_t address, size_t max_len = 260) -> std::string {
        std::string out;
        if (address == 0 || max_len == 0)
            return out;
        out.reserve(max_len);
        for (size_t i = 0; i < max_len; ++i)
        {
            char ch = 0;
            if (!ReadVirtualExact(address + i, &ch, sizeof(ch)))
                break;
            if (ch == '\0')
                break;
            out.push_back(ch);
        }
        return out;
    };

    const char* directory_names[IMAGE_NUMBEROF_DIRECTORY_ENTRIES] = {
        "EXPORT", "IMPORT", "RESOURCE", "EXCEPTION", "SECURITY", "BASERELOC", "DEBUG", "ARCHITECTURE",
        "GLOBALPTR", "TLS", "LOAD_CONFIG", "BOUND_IMPORT", "IAT", "DELAY_IMPORT", "COM_DESCRIPTOR", "RESERVED"
    };

    json root = {
        {"schemaVersion", "1.0"},
        {"requestId", std::to_string(request_id)},
        {"available", false},
        {"imageBase", ""},
        {"moduleName", ""},
        {"architecture", ""},
        {"headers", {
            {"dos", nullptr},
            {"nt", nullptr}
        }},
        {"sections", json::array()},
        {"dataDirectories", json::array()},
        {"imports", json::array()},
        {"exports", nullptr},
        {"relationships", json::array()},
        {"notes", json::array()}
    };

    uint64_t image_base = 0;
    auto params = ParseQueryParams(query);
    auto image_it = params.find("imageBase");
    if (image_it != params.end()) {
        TryParseU64(TrimAsciiWhitespace(image_it->second), image_base);
    }
    if (image_base == 0) {
        auto address_it = params.find("address");
        if (address_it != params.end()) {
            TryParseU64(TrimAsciiWhitespace(address_it->second), image_base);
        }
    }

    auto proc = DK_MODEL_ACCESS->get_current_process();
    if (image_base == 0 && proc != nullptr)
    {
        try { image_base = DK_MGET_PVAL<uint64_t, VT_UI8>(proc, "ImageBaseAddress"); } catch (...) {}
        if (image_base == 0)
        {
            try {
                auto env = DK_MGET_POBJ(proc, "Environment");
                if (env != nullptr) {
                    try { image_base = DK_MGET_PVAL<uint64_t, VT_UI8>(env, "ImageBaseAddress"); } catch (...) {}
                }
            } catch (...) {}
        }
        if (image_base == 0)
        {
            try {
                auto modules = DK_MGET_POBJ(proc, "Modules");
                if (modules != nullptr)
                {
                    auto first = DK_MODEL_ACCESS->at(modules, 0);
                    if (first != nullptr)
                    {
                        try { image_base = DK_MGET_PVAL<uint64_t, VT_UI8>(first, "BaseAddress"); } catch (...) {}
                        if (image_base == 0) {
                            try { image_base = DK_MGET_PVAL<uint64_t, VT_UI8>(first, "Address"); } catch (...) {}
                        }
                        try { root["moduleName"] = BSTR2str(DK_MGET_PVAL<BSTR, VT_BSTR>(first, "Name")); } catch (...) {}
                    }
                }
            } catch (...) {}
        }
    }

    if (image_base == 0)
    {
        root["notes"].push_back("Unable to resolve image base for current process.");
        return root.dump();
    }

    IMAGE_DOS_HEADER dos = {};
    if (!ReadVirtualExact(image_base, &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE)
    {
        root["notes"].push_back("Image base does not point to a valid IMAGE_DOS_HEADER.");
        return root.dump();
    }

    uint64_t nt_address = image_base + static_cast<uint32_t>(dos.e_lfanew);
    DWORD nt_signature = 0;
    if (!ReadVirtualExact(nt_address, &nt_signature, sizeof(nt_signature)) || nt_signature != IMAGE_NT_SIGNATURE)
    {
        root["notes"].push_back("IMAGE_NT_HEADERS signature is invalid.");
        return root.dump();
    }

    IMAGE_FILE_HEADER file_header = {};
    if (!ReadVirtualExact(nt_address + sizeof(DWORD), &file_header, sizeof(file_header)))
    {
        root["notes"].push_back("Unable to read IMAGE_FILE_HEADER.");
        return root.dump();
    }

    WORD optional_magic = 0;
    if (!ReadVirtualExact(nt_address + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER), &optional_magic, sizeof(optional_magic)))
    {
        root["notes"].push_back("Unable to read IMAGE_OPTIONAL_HEADER magic.");
        return root.dump();
    }

    bool is_64 = optional_magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    IMAGE_OPTIONAL_HEADER64 optional64 = {};
    IMAGE_OPTIONAL_HEADER32 optional32 = {};
    uint32_t size_of_headers = 0;
    uint32_t size_of_image = 0;
    uint32_t address_of_entry_point = 0;
    uint64_t image_base_from_optional = image_base;

    if (is_64)
    {
        IMAGE_NT_HEADERS64 nt64 = {};
        if (!ReadVirtualExact(nt_address, &nt64, sizeof(nt64)))
        {
            root["notes"].push_back("Unable to read IMAGE_NT_HEADERS64.");
            return root.dump();
        }
        optional64 = nt64.OptionalHeader;
        size_of_headers = nt64.OptionalHeader.SizeOfHeaders;
        size_of_image = nt64.OptionalHeader.SizeOfImage;
        address_of_entry_point = nt64.OptionalHeader.AddressOfEntryPoint;
        image_base_from_optional = nt64.OptionalHeader.ImageBase;
        root["architecture"] = "x64";
    }
    else
    {
        IMAGE_NT_HEADERS32 nt32 = {};
        if (!ReadVirtualExact(nt_address, &nt32, sizeof(nt32)))
        {
            root["notes"].push_back("Unable to read IMAGE_NT_HEADERS32.");
            return root.dump();
        }
        optional32 = nt32.OptionalHeader;
        size_of_headers = nt32.OptionalHeader.SizeOfHeaders;
        size_of_image = nt32.OptionalHeader.SizeOfImage;
        address_of_entry_point = nt32.OptionalHeader.AddressOfEntryPoint;
        image_base_from_optional = nt32.OptionalHeader.ImageBase;
        root["architecture"] = "x86";
    }

    root["available"] = true;
    root["imageBase"] = FormatHex(image_base);

    root["headers"]["dos"] = {
        {"id", "dos"},
        {"name", "IMAGE_DOS_HEADER"},
        {"address", FormatHex(image_base)},
        {"size", sizeof(IMAGE_DOS_HEADER)},
        {"fields", json::array({
            json{{"name", "e_magic"}, {"value", FormatHex32(dos.e_magic)}},
            json{{"name", "e_lfanew"}, {"value", FormatHex32(static_cast<uint32_t>(dos.e_lfanew))}},
            json{{"name", "ntHeaderAddress"}, {"value", FormatHex(nt_address)}}
        })}
    };

    root["headers"]["nt"] = {
        {"id", "nt"},
        {"name", is_64 ? "IMAGE_NT_HEADERS64" : "IMAGE_NT_HEADERS32"},
        {"address", FormatHex(nt_address)},
        {"size", is_64 ? sizeof(IMAGE_NT_HEADERS64) : sizeof(IMAGE_NT_HEADERS32)},
        {"fields", json::array({
            json{{"name", "Signature"}, {"value", FormatHex32(nt_signature)}},
            json{{"name", "Machine"}, {"value", FormatHex32(file_header.Machine)}},
            json{{"name", "NumberOfSections"}, {"value", std::to_string(file_header.NumberOfSections)}},
            json{{"name", "TimeDateStamp"}, {"value", FormatHex32(file_header.TimeDateStamp)}},
            json{{"name", "Characteristics"}, {"value", FormatHex32(file_header.Characteristics)}},
            json{{"name", "AddressOfEntryPoint"}, {"value", FormatHex32(address_of_entry_point)}},
            json{{"name", "ImageBase"}, {"value", FormatHex(image_base_from_optional)}},
            json{{"name", "SizeOfImage"}, {"value", FormatHex32(size_of_image)}},
            json{{"name", "SizeOfHeaders"}, {"value", FormatHex32(size_of_headers)}}
        })}
    };

    uint64_t section_address = nt_address + sizeof(DWORD) + sizeof(IMAGE_FILE_HEADER) + file_header.SizeOfOptionalHeader;
    for (uint32_t i = 0; i < file_header.NumberOfSections; ++i)
    {
        IMAGE_SECTION_HEADER section = {};
        if (!ReadVirtualExact(section_address + (i * sizeof(IMAGE_SECTION_HEADER)), &section, sizeof(section)))
            break;
        std::string name(reinterpret_cast<const char*>(section.Name), reinterpret_cast<const char*>(section.Name) + 8);
        size_t zero = name.find('\0');
        if (zero != std::string::npos) name.resize(zero);

        root["sections"].push_back({
            {"name", name},
            {"virtualAddress", FormatHex32(section.VirtualAddress)},
            {"virtualSize", FormatHex32(section.Misc.VirtualSize)},
            {"rawSize", FormatHex32(section.SizeOfRawData)},
            {"characteristics", FormatHex32(section.Characteristics)},
            {"address", FormatHex(image_base + section.VirtualAddress)}
        });
    }

    const IMAGE_DATA_DIRECTORY* directories = is_64 ? optional64.DataDirectory : optional32.DataDirectory;
    for (int i = 0; i < IMAGE_NUMBEROF_DIRECTORY_ENTRIES; ++i)
    {
        const auto& dir = directories[i];
        json entry = {
            {"id", std::string("dir-") + std::to_string(i)},
            {"index", i},
            {"name", directory_names[i]},
            {"virtualAddress", FormatHex32(dir.VirtualAddress)},
            {"size", FormatHex32(dir.Size)},
            {"address", dir.VirtualAddress != 0 ? FormatHex(image_base + dir.VirtualAddress) : ""}
        };
        root["dataDirectories"].push_back(entry);
    }

    auto import_dir = directories[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if (import_dir.VirtualAddress != 0 && import_dir.Size != 0)
    {
        const uint64_t import_base = image_base + import_dir.VirtualAddress;
        const uint64_t thunk_size = is_64 ? sizeof(uint64_t) : sizeof(uint32_t);
        for (size_t descriptor_index = 0; descriptor_index < 512; ++descriptor_index)
        {
            IMAGE_IMPORT_DESCRIPTOR descriptor = {};
            uint64_t descriptor_addr = import_base + descriptor_index * sizeof(IMAGE_IMPORT_DESCRIPTOR);
            if (!ReadVirtualExact(descriptor_addr, &descriptor, sizeof(descriptor)))
                break;
            if (descriptor.Name == 0 && descriptor.FirstThunk == 0 && descriptor.OriginalFirstThunk == 0)
                break;

            json import_json = {
                {"id", std::string("import-") + std::to_string(descriptor_index)},
                {"name", ReadString(image_base + descriptor.Name)},
                {"descriptorAddress", FormatHex(descriptor_addr)},
                {"nameRva", FormatHex32(descriptor.Name)},
                {"originalFirstThunk", FormatHex32(descriptor.OriginalFirstThunk)},
                {"firstThunk", FormatHex32(descriptor.FirstThunk)},
                {"timeDateStamp", FormatHex32(descriptor.TimeDateStamp)},
                {"forwarderChain", FormatHex32(descriptor.ForwarderChain)},
                {"functions", json::array()}
            };

            uint32_t thunk_rva = descriptor.OriginalFirstThunk != 0 ? descriptor.OriginalFirstThunk : descriptor.FirstThunk;
            for (size_t thunk_index = 0; thunk_index < 256 && thunk_rva != 0; ++thunk_index)
            {
                uint64_t thunk_value = 0;
                uint64_t thunk_addr = image_base + thunk_rva + thunk_index * thunk_size;
                if (is_64)
                {
                    uint64_t raw_thunk = 0;
                    if (!ReadVirtualExact(thunk_addr, &raw_thunk, sizeof(raw_thunk)) || raw_thunk == 0)
                        break;
                    thunk_value = raw_thunk;
                }
                else
                {
                    uint32_t raw_thunk = 0;
                    if (!ReadVirtualExact(thunk_addr, &raw_thunk, sizeof(raw_thunk)) || raw_thunk == 0)
                        break;
                    thunk_value = raw_thunk;
                }

                json function_json = {
                    {"index", thunk_index},
                    {"thunkAddress", FormatHex(thunk_addr)},
                    {"iatAddress", FormatHex(image_base + descriptor.FirstThunk + thunk_index * thunk_size)},
                    {"ordinal", nullptr},
                    {"hint", nullptr},
                    {"name", ""}
                };

                bool is_ordinal = is_64
                    ? ((thunk_value & IMAGE_ORDINAL_FLAG64) != 0)
                    : ((thunk_value & IMAGE_ORDINAL_FLAG32) != 0);

                if (is_ordinal)
                {
                    function_json["ordinal"] = static_cast<uint32_t>(thunk_value & 0xFFFF);
                }
                else
                {
                    uint64_t import_by_name_addr = image_base + static_cast<uint32_t>(thunk_value & 0xFFFFFFFF);
                    WORD hint = 0;
                    if (ReadVirtualExact(import_by_name_addr, &hint, sizeof(hint)))
                    {
                        function_json["hint"] = hint;
                        function_json["name"] = ReadString(import_by_name_addr + sizeof(WORD), 512);
                    }
                }
                import_json["functions"].push_back(function_json);
            }

            root["imports"].push_back(import_json);
        }
    }

    auto export_dir = directories[IMAGE_DIRECTORY_ENTRY_EXPORT];
    if (export_dir.VirtualAddress != 0 && export_dir.Size != 0)
    {
        IMAGE_EXPORT_DIRECTORY export_descriptor = {};
        uint64_t export_addr = image_base + export_dir.VirtualAddress;
        if (ReadVirtualExact(export_addr, &export_descriptor, sizeof(export_descriptor)))
        {
            std::map<uint32_t, std::string> names_by_ordinal;
            for (uint32_t i = 0; i < export_descriptor.NumberOfNames && i < 2048; ++i)
            {
                uint32_t name_rva = 0;
                uint16_t ordinal_index = 0;
                if (!ReadVirtualExact(image_base + export_descriptor.AddressOfNames + i * sizeof(uint32_t), &name_rva, sizeof(name_rva)))
                    break;
                if (!ReadVirtualExact(image_base + export_descriptor.AddressOfNameOrdinals + i * sizeof(uint16_t), &ordinal_index, sizeof(ordinal_index)))
                    break;
                names_by_ordinal[ordinal_index] = ReadString(image_base + name_rva, 512);
            }

            json functions = json::array();
            for (uint32_t i = 0; i < export_descriptor.NumberOfFunctions && i < 2048; ++i)
            {
                uint32_t func_rva = 0;
                if (!ReadVirtualExact(image_base + export_descriptor.AddressOfFunctions + i * sizeof(uint32_t), &func_rva, sizeof(func_rva)))
                    break;
                if (func_rva == 0)
                    continue;
                bool is_forwarder = func_rva >= export_dir.VirtualAddress && func_rva < export_dir.VirtualAddress + export_dir.Size;
                json func = {
                    {"ordinal", export_descriptor.Base + i},
                    {"rva", FormatHex32(func_rva)},
                    {"address", FormatHex(image_base + func_rva)},
                    {"name", names_by_ordinal.count(i) ? names_by_ordinal[i] : ""},
                    {"forwarder", is_forwarder ? ReadString(image_base + func_rva, 256) : ""}
                };
                functions.push_back(func);
            }

            root["exports"] = {
                {"id", "export"},
                {"name", "IMAGE_EXPORT_DIRECTORY"},
                {"descriptorAddress", FormatHex(export_addr)},
                {"dllName", ReadString(image_base + export_descriptor.Name)},
                {"ordinalBase", export_descriptor.Base},
                {"numberOfFunctions", export_descriptor.NumberOfFunctions},
                {"numberOfNames", export_descriptor.NumberOfNames},
                {"addressOfFunctions", FormatHex32(export_descriptor.AddressOfFunctions)},
                {"addressOfNames", FormatHex32(export_descriptor.AddressOfNames)},
                {"addressOfNameOrdinals", FormatHex32(export_descriptor.AddressOfNameOrdinals)},
                {"functions", functions}
            };
        }
    }

    root["relationships"] = json::array({
        json{{"from", "dos"}, {"to", "nt"}, {"label", "e_lfanew"}},
        json{{"from", "nt"}, {"to", "dir-0"}, {"label", "Export Directory"}},
        json{{"from", "nt"}, {"to", "dir-1"}, {"label", "Import Directory"}}
    });
    for (size_t i = 0; i < root["imports"].size(); ++i)
    {
        root["relationships"].push_back({{"from", "dir-1"}, {"to", root["imports"][i]["id"]}, {"label", "descriptor"}});
    }
    if (!root["exports"].is_null())
    {
        root["relationships"].push_back({{"from", "dir-0"}, {"to", "export"}, {"label", "descriptor"}});
    }

    if (root["imports"].size() == 0)
        root["notes"].push_back("No import descriptors found or import directory is empty.");
    if (root["exports"].is_null())
        root["notes"].push_back("No export descriptor found or export directory is empty.");

    return root.dump();
}

std::string CDkEmbeddedServer::HandleFunctionCallsRoute(const std::string& query)
{
    uint64_t request_id = ++m_request_counter;

    auto FormatHexU64 = [](uint64_t value) -> std::string
    {
        std::stringstream ss;
        ss << "0x" << std::hex << std::setw(16) << std::setfill('0') << value;
        return ss.str();
    };

    json root = {
        {"schemaVersion", "1.0"},
        {"requestId", std::to_string(request_id)},
        {"available", false},
        {"events", json::array()},
        {"returnedCount", 0},
        {"totalCount", 0},
        {"query", {
            {"input", ""},
            {"pattern", ""},
            {"resolvedSymbol", nullptr},
            {"mode", "symbol"}
        }}
    };

    auto params = ParseQueryParams(query);
    std::string target;

    auto target_iter = params.find("target");
    if (target_iter != params.end())
        target = TrimAsciiWhitespace(target_iter->second);

    if (target.empty())
    {
        auto query_iter = params.find("query");
        if (query_iter != params.end())
            target = TrimAsciiWhitespace(query_iter->second);
    }

    if (target.empty())
    {
        auto name_iter = params.find("name");
        if (name_iter != params.end())
            target = TrimAsciiWhitespace(name_iter->second);
    }

    root["query"]["input"] = target;

    uint64_t requested_limit = 200;
    auto limit_iter = params.find("limit");
    if (limit_iter != params.end())
    {
        uint64_t parsed_limit = 0;
        if (TryParseU64(limit_iter->second, parsed_limit) && parsed_limit > 0)
            requested_limit = std::min<uint64_t>(parsed_limit, 1000);
    }

    if (target.empty())
    {
        root["message"] = "Query parameter 'target' is required.";
        return root.dump();
    }

    if (!DK_MODEL_ACCESS->isTTD())
    {
        root["message"] = "Function-call search requires TTD mode.";
        return root.dump();
    }

    auto session = DK_MODEL_ACCESS->get_current_session();
    auto ttd = session != nullptr ? DK_MGET_POBJ(session, "TTD") : DK_MOBJ_PTR{};

    // Fallback only: some contexts may expose TTD under process.
    if (ttd == nullptr)
    {
        auto proc = DK_MODEL_ACCESS->get_current_process();
        if (proc != nullptr)
            ttd = DK_MGET_POBJ(proc, "TTD");
    }

    if (ttd == nullptr)
    {
        root["message"] = "TTD object unavailable.";
        return root.dump();
    }

    auto calls_pobj = DK_MGET_POBJ(ttd, "Calls");
    if (calls_pobj == nullptr)
    {
        root["message"] = "TTD call adapter unavailable.";
        return root.dump();
    }

    uint64_t target_address = 0;
    bool has_target_address = TryParseU64(target, target_address);
    std::string resolved_symbol;
    std::string call_pattern = target;

    if (has_target_address)
    {
        auto symbol_info = EXT_F_Addr2Sym(target_address);
        resolved_symbol = std::get<0>(symbol_info);
        if (!resolved_symbol.empty())
            call_pattern = resolved_symbol;

        root["query"]["mode"] = "address";
        root["query"]["address"] = FormatHexU64(target_address);
    }

    root["query"]["pattern"] = call_pattern;
    if (!resolved_symbol.empty())
        root["query"]["resolvedSymbol"] = resolved_symbol;

    auto arg = DK_MODEL_ACCESS->create_str_intrinsic_obj(call_pattern);
    std::vector<DK_MOBJ_PTR> vec_args;
    vec_args.push_back(arg);

    auto call_result = DK_MODEL_ACCESS->call(calls_pobj, ttd, vec_args);
    if (call_result == nullptr)
    {
        root["message"] = "TTD.Calls query returned no data.";
        return root.dump();
    }

    auto ExtractModuleName = [](const std::string& symbol_name) -> std::string
    {
        auto bang_pos = symbol_name.find('!');
        return bang_pos == std::string::npos ? "" : symbol_name.substr(0, bang_pos);
    };

    size_t total_count = 0;
    auto results = DK_MODEL_ACCESS->iterate(call_result);
    for (auto& result : results)
    {
        auto call_info = std::get<1>(result);
        if (call_info == nullptr)
            continue;

        std::tuple<uint64_t, uint64_t> start_pos{ 0, 0 };
        std::tuple<uint64_t, uint64_t> end_pos{ 0, 0 };
        std::string function;
        uint64_t function_address = 0;
        uint64_t return_address = 0;
        uint64_t return_value = 0;
        uint64_t thread_id = 0;
        uint64_t event_type = 0;

        try { start_pos = DK_MODEL_ACCESS->get_pos(call_info, "TimeStart"); } catch (...) {}
        try { end_pos = DK_MODEL_ACCESS->get_pos(call_info, "TimeEnd"); } catch (...) {}
        try { function = BSTR2str(DK_MODEL_ACCESS->get_pvalue<BSTR, VT_BSTR>(call_info, "Function")); } catch (...) {}
        try { function_address = DK_MODEL_ACCESS->get_pvalue<uint64_t, VT_UI8>(call_info, "FunctionAddress"); } catch (...) {}
        try { return_address = DK_MODEL_ACCESS->get_pvalue<uint64_t, VT_UI8>(call_info, "ReturnAddress"); } catch (...) {}
        try { return_value = DK_MODEL_ACCESS->get_pvalue<uint64_t, VT_UI8>(call_info, "ReturnValue"); } catch (...) {}
        try { thread_id = DK_MODEL_ACCESS->get_pvalue<uint64_t, VT_UI8>(call_info, "ThreadId"); } catch (...) {}
        try { event_type = DK_MODEL_ACCESS->get_pvalue<uint64_t, VT_UI8>(call_info, "EventType"); } catch (...) {}

        auto symbol_info = EXT_F_Addr2Sym(function_address);
        std::string symbol_name = std::get<0>(symbol_info);
        uint64_t displacement = std::get<1>(symbol_info);

        std::string display_name = !symbol_name.empty()
            ? symbol_name
            : (!function.empty() ? function : FormatHexU64(function_address));
        std::string module_name = ExtractModuleName(display_name);

        bool matches = true;
        if (has_target_address)
        {
            matches = function_address == target_address || return_address == target_address;
            if (!matches && !resolved_symbol.empty())
                matches = ContainsCaseInsensitive(display_name, resolved_symbol);
            if (!matches)
                matches = ContainsCaseInsensitive(display_name, target);
        }
        else
        {
            matches = ContainsCaseInsensitive(display_name, target)
                || ContainsCaseInsensitive(function, target)
                || ContainsCaseInsensitive(module_name, target);
        }

        if (!matches)
            continue;

        ++total_count;

        if (root["events"].size() >= requested_limit)
            continue;

        std::stringstream summary;
        summary << display_name
            << "  tid=" << thread_id
            << "  " << std::get<0>(start_pos) << ":" << std::get<1>(start_pos);

        json parameters = json::array();
        auto parameters_pobj = DK_MGET_POBJ(call_info, "Parameters");
        if (parameters_pobj != nullptr)
        {
            auto parameter_items = DK_MODEL_ACCESS->iterate(parameters_pobj);
            for (auto& parameter : parameter_items)
            {
                try
                {
                    uint64_t value = DK_MODEL_ACCESS->get_value<uint64_t, VT_UI8>(std::get<1>(parameter));
                    parameters.push_back(FormatHexU64(value));
                }
                catch (...)
                {
                }
            }
        }

        // Collect stack-based arguments (args 5-16) by seeking to TimeStart,
        // reading RSP, then reading [RSP+0x28], [RSP+0x30], ... (x64 calling convention:
        // RSP+0 = return address, RSP+8..+32 = shadow space, RSP+0x28 = arg5).
        if (std::get<0>(start_pos) != 0 && EXT_D_IDebugDataSpaces != nullptr)
        {
            try
            {
                std::tuple<uint64_t, uint64_t> original_pos{ 0, 0 };
                try { original_pos = DK_GET_CURPOS(); } catch (...) {}

                bool seeked = false;
                try
                {
                    DK_SEEK_TO(std::get<0>(start_pos), std::get<1>(start_pos));
                    seeked = true;
                }
                catch (...) {}

                if (seeked)
                {
                    // Read RSP from the calling thread's registers.
                    uint64_t rsp_value = 0;
                    bool has_rsp = false;

                    auto proc_obj = DK_MODEL_ACCESS->get_current_process();
                    if (proc_obj != nullptr)
                    {
                        auto threads_pobj = DK_MGET_POBJ(proc_obj, "Threads");
                        if (threads_pobj != nullptr)
                        {
                            for (auto& t : DK_MODEL_ACCESS->iterate(threads_pobj))
                            {
                                if (has_rsp) break;
                                auto t_obj = std::get<1>(t);
                                try
                                {
                                    uint64_t tid = DK_MGET_PVAL<uint64_t, VT_UI8>(t_obj, "Id");
                                    if (tid != thread_id) continue;
                                }
                                catch (...) { continue; }

                                auto regs_obj = DK_MGET_POBJ(t_obj, "Registers");
                                if (regs_obj == nullptr) continue;

                                std::vector<DK_MOBJ_PTR> reg_roots;
                                reg_roots.push_back(regs_obj);
                                auto user_obj2 = DK_MGET_POBJ(regs_obj, "User");
                                if (user_obj2 != nullptr) reg_roots.push_back(user_obj2);

                                for (auto& rr : reg_roots)
                                {
                                    if (has_rsp) break;
                                    for (const auto& rname : std::vector<std::string>{"rsp", "Rsp", "RSP"})
                                    {
                                        try
                                        {
                                            auto reg_obj = DK_MGET_POBJ(rr, rname);
                                            if (reg_obj != nullptr)
                                            {
                                                try
                                                {
                                                    rsp_value = DK_MODEL_ACCESS->get_value<uint64_t, VT_UI8>(reg_obj);
                                                    has_rsp = (rsp_value != 0);
                                                }
                                                catch (...) {}
                                            }
                                        }
                                        catch (...) {}
                                        if (has_rsp) break;
                                    }
                                }
                            }
                        }
                    }

                    if (has_rsp && rsp_value != 0)
                    {
                        // x64: arg5 starts at RSP+0x28 (return addr at RSP+0, then 4*8 shadow space).
                        // Read up to 12 additional args so the total can reach 16.
                        constexpr int kExtraArgs = 12;
                        for (int i = 0; i < kExtraArgs; ++i)
                        {
                            uint64_t addr = rsp_value + 0x28 + static_cast<uint64_t>(i) * 8;
                            uint64_t val = 0;
                            ULONG bytes_read = 0;
                            HRESULT hr = EXT_D_IDebugDataSpaces->ReadVirtual(
                                addr, reinterpret_cast<uint8_t*>(&val), sizeof(val), &bytes_read);
                            if (SUCCEEDED(hr) && bytes_read == sizeof(val))
                                parameters.push_back(FormatHexU64(val));
                            else
                                break; // stop at first unreadable address
                        }
                    }

                    // Restore original position.
                    try { DK_SEEK_TO(std::get<0>(original_pos), std::get<1>(original_pos)); } catch (...) {}
                }
            }
            catch (...) {}
        }

        std::stringstream event_id;
        event_id << std::get<0>(start_pos) << ":"
            << std::get<1>(start_pos) << ":"
            << thread_id << ":"
            << std::hex << function_address;

        root["events"].push_back({
            {"eventId", event_id.str()},
            {"summary", summary.str()},
            {"function", display_name},
            {"rawFunction", function},
            {"module", module_name},
            {"threadId", thread_id},
            {"eventType", event_type},
            {"functionAddress", FormatHexU64(function_address)},
            {"returnAddress", FormatHexU64(return_address)},
            {"returnValue", FormatHexU64(return_value)},
            {"symbolDisplacement", displacement},
            {"startPosition", PosToJson(start_pos)},
            {"endPosition", PosToJson(end_pos)},
            {"parameters", parameters}
        });
    }

    root["available"] = total_count > 0;
    root["returnedCount"] = root["events"].size();
    root["totalCount"] = total_count;

    if (total_count == 0)
        root["message"] = "No matching function-call events found.";

    return root.dump();
}

std::string CDkEmbeddedServer::HandleStringsRoute(const std::string& query)
{
        uint64_t request_id = ++m_request_counter;
        auto params = ParseQueryParams(query);

        auto ReadParam = [&](const std::initializer_list<const char*>& keys, const std::string& fallback = "") -> std::string {
            for (const auto* key : keys)
            {
                auto it = params.find(key);
                if (it != params.end())
                {
                    std::string value = TrimAsciiWhitespace(it->second);
                    if (!value.empty())
                        return value;
                }
            }
            return fallback;
        };

        auto FormatHex = [](uint64_t value) -> std::string {
            std::ostringstream stream;
            stream << "0x" << std::hex << std::nouppercase << value;
            return stream.str();
        };

        auto Utf8ToUtf16 = [](const std::string& text) -> std::wstring {
            if (text.empty()) return std::wstring();
            int required = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
            if (required <= 0)
            {
                std::wstring fallback;
                fallback.reserve(text.size());
                for (unsigned char ch : text)
                    fallback.push_back(static_cast<wchar_t>(ch));
                return fallback;
            }
            std::wstring wide(static_cast<size_t>(required), L'\0');
            MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), &wide[0], required);
            return wide;
        };

        json root = {
            {"schemaVersion", "1.0"},
            {"requestId", std::to_string(request_id)},
            {"available", false},
            {"query", { {"text", ""}, {"limit", 100}, {"rangeStart", "0x0"}, {"rangeEnd", "0x7fffffff0000"}, {"scope", "smart"} }},
            {"ascii", json::array()},
            {"unicode", json::array()},
            {"notes", json::array()}
        };

        if (!DK_MODEL_ACCESS->isUsermode())
        {
            root["notes"].push_back("Strings search is supported in user mode only.");
            return root.dump();
        }

        const std::string search_text = ReadParam({ "q", "query", "text", "pattern" });
        root["query"]["text"] = search_text;
        if (search_text.empty())
        {
            root["notes"].push_back("Query parameter 'q' is required.");
            return root.dump();
        }

        uint64_t limit = 100;
        uint64_t parsed_limit = 0;
        if (TryParseU64(ReadParam({ "limit" }, "100"), parsed_limit) && parsed_limit > 0)
            limit = std::min<uint64_t>(500, parsed_limit);
        root["query"]["limit"] = limit;

        uint64_t range_start = 0;
        uint64_t range_end = 0x7fffffff0000ULL;
        uint64_t parsed = 0;
        if (TryParseU64(ReadParam({ "start", "rangeStart" }, ""), parsed))
            range_start = parsed;
        if (TryParseU64(ReadParam({ "end", "rangeEnd" }, ""), parsed))
            range_end = parsed;
        if (range_end <= range_start)
            range_end = range_start + 0x8000000ULL;
        root["query"]["rangeStart"] = FormatHex(range_start);
        root["query"]["rangeEnd"] = FormatHex(range_end);

        const std::string scope = ReadParam({ "scope" }, "smart");
        const std::string writable_only_param = ReadParam({ "writableOnly", "writable" }, "");
        const bool writable_only =
            scope == "writable" || scope == "write" ||
            writable_only_param == "1" || writable_only_param == "true" || writable_only_param == "yes";
        const bool modules_only = scope == "modules" || scope == "images";
        const bool all_pages = scope == "all";
        root["query"]["scope"] = writable_only ? "writable" : (modules_only ? "modules" : (all_pages ? "all" : "smart"));

        auto proc = DK_MODEL_ACCESS->get_current_process();
        std::vector<std::pair<ULONG64, ULONG64>> module_ranges;
        if (proc != nullptr)
        {
            try
            {
                auto modules_obj = DK_MGET_POBJ(proc, "Modules");
                if (modules_obj != nullptr)
                {
                    auto module_entries = DK_MODEL_ACCESS->iterate(modules_obj);
                    for (auto& entry : module_entries)
                    {
                        try
                        {
                            auto mod = std::get<1>(entry);
                            auto info = DK_MGET_MODL(mod);
                            const ULONG64 base = std::get<0>(info);
                            const ULONG64 size = std::get<1>(info);
                            if (base == 0 || size == 0)
                                continue;

                            const ULONG64 mod_start = std::max<ULONG64>(base, range_start);
                            const ULONG64 mod_end = std::min<ULONG64>(base + size, range_end);
                            if (mod_end > mod_start)
                                module_ranges.push_back({ mod_start, mod_end });
                        }
                        catch (...) {}
                    }
                }
            }
            catch (...) {}
        }

        auto SearchPattern = [&](const void* pattern, ULONG pattern_size, ULONG pattern_granularity, uint64_t logical_length, const std::vector<std::pair<ULONG64, ULONG64>>& ranges, ULONG search_flags, json& out_results) {
            if (pattern == nullptr || pattern_size == 0)
                return;

            const ULONG64 chunk_length = 0x8000000ULL;
            std::unordered_set<ULONG64> seen_matches;

            for (const auto& range : ranges)
            {
                ULONG64 cursor = range.first;
                const ULONG64 range_limit = range.second;

                while (cursor < range_limit && out_results.size() < limit)
                {
                    ULONG64 match_offset = 0;
                    const ULONG64 current_length = std::min<ULONG64>(chunk_length, range_limit - cursor);
                    HRESULT hr = EXT_D_IDebugDataSpaces4->SearchVirtual2(
                        cursor,
                        current_length,
                        search_flags,
                        const_cast<void*>(pattern),
                        pattern_size,
                        pattern_granularity,
                        &match_offset);

                    if (hr == S_OK)
                    {
                        if (seen_matches.insert(match_offset).second)
                        {
                            out_results.push_back({
                                {"address", FormatHex(match_offset)},
                                {"page", FormatHex(match_offset & 0xFFFFFFFFFFFFF000ULL)},
                                {"offsetInPage", FormatHex(match_offset & 0xFFFULL)},
                                {"length", logical_length}
                            });
                        }
                        cursor = match_offset + std::max<ULONG64>(1, pattern_size);
                        continue;
                    }

                    if (hr == HRESULT_FROM_WIN32(ERROR_NOT_FOUND) || hr == static_cast<HRESULT>(0x9000001A))
                    {
                        cursor += current_length;
                        continue;
                    }

                    std::ostringstream note;
                    note << "SearchVirtual2 failed with HRESULT 0x" << std::hex << static_cast<uint32_t>(hr);
                    root["notes"].push_back(note.str());
                    return;
                }
            }
        };

        const std::vector<std::pair<ULONG64, ULONG64>> full_range = { { range_start, range_end } };

        auto ExecuteSearchPlan = [&](const void* pattern, ULONG pattern_size, ULONG pattern_granularity, uint64_t logical_length, json& out_results, const char* label) {
            if (writable_only)
            {
                root["notes"].push_back(std::string(label) + ": writable pages only.");
                SearchPattern(pattern, pattern_size, pattern_granularity, logical_length, full_range, DEBUG_VSEARCH_WRITABLE_ONLY, out_results);
                return;
            }

            if (modules_only)
            {
                root["notes"].push_back(std::string(label) + ": loaded module images only.");
                SearchPattern(pattern, pattern_size, pattern_granularity, logical_length, module_ranges, DEBUG_VSEARCH_DEFAULT, out_results);
                return;
            }

            if (all_pages)
            {
                root["notes"].push_back(std::string(label) + ": full user-mode scan. This is slower on large processes.");
                SearchPattern(pattern, pattern_size, pattern_granularity, logical_length, full_range, DEBUG_VSEARCH_DEFAULT, out_results);
                return;
            }

            root["notes"].push_back(std::string(label) + ": smart search uses writable pages first, then loaded module images if needed.");
            SearchPattern(pattern, pattern_size, pattern_granularity, logical_length, full_range, DEBUG_VSEARCH_WRITABLE_ONLY, out_results);
            if (out_results.empty() && !module_ranges.empty())
            {
                root["notes"].push_back(std::string(label) + ": no writable-page hits, falling back to loaded module images.");
                SearchPattern(pattern, pattern_size, pattern_granularity, logical_length, module_ranges, DEBUG_VSEARCH_DEFAULT, out_results);
            }
        };

        ExecuteSearchPlan(search_text.data(), static_cast<ULONG>(search_text.size()), 1, static_cast<uint64_t>(search_text.size()), root["ascii"], "ASCII");

        const std::wstring wide = Utf8ToUtf16(search_text);
        if (!wide.empty())
            ExecuteSearchPlan(wide.data(), static_cast<ULONG>(wide.size() * sizeof(wchar_t)), 1, static_cast<uint64_t>(wide.size()), root["unicode"], "UNICODE");

        root["available"] = true;
        if (root["ascii"].empty())
            root["notes"].push_back("No ASCII matches found.");
        if (root["unicode"].empty())
            root["notes"].push_back("No Unicode matches found.");

        return root.dump();
}

// ---------------------------------------------------------------------------
// /api/memory/layout
// Returns a snapshot of the target process virtual address space organised
// into typed regions for the multi-level Memory Layout visualisation in the
// front-end.  For each category (modules, threads/stacks, heaps, deterministic
// areas) the route returns the VA range plus category-specific detail fields.
// ---------------------------------------------------------------------------
std::string CDkEmbeddedServer::HandleMemoryLayoutRoute(const std::string& query)
{
    uint64_t request_id = ++m_request_counter;
    bool is_ttd = DK_MODEL_ACCESS->isTTD();

    auto FormatHex = [](uint64_t v) -> std::string {
        std::stringstream ss;
        ss << "0x" << std::hex << std::setw(16) << std::setfill('0') << v;
        return ss.str();
    };

    json root = {
        {"schemaVersion", "1.0"},
        {"requestId", std::to_string(request_id)},
        {"available", false},
        {"isUsermode", DK_MODEL_ACCESS->isUsermode()},
        {"isKernelmode", DK_MODEL_ACCESS->isKernelmode()},
        {"isTTD", is_ttd},
        // Deterministic VA landmarks for 64-bit Windows user-mode
        {"landmarks", json::array()},
        {"modules", json::array()},
        {"threads", json::array()},
        {"heaps", json::array()}
    };

    // -----------------------------------------------------------------------
    // Deterministic / well-known VA landmarks (user-mode 64-bit Windows)
    // -----------------------------------------------------------------------
    auto& landmarks = root["landmarks"];

    // Null / reserved
    landmarks.push_back({{"name","Null / Reserved"},  {"base","0x0000000000000000"}, {"end","0x0000000000010000"}, {"kind","reserved"}, {"color","#3a3a3a"}});
    // User-mode accessible range ends at ~8 TB on current Windows
    // (user-mode limit 0x00007FFFFFFFFFFF, though practical max is lower)
    landmarks.push_back({{"name","User Space"},        {"base","0x0000000000010000"}, {"end","0x00007FFFFFFFFFFF"}, {"kind","user"},     {"color","#1a2a1a"}});
    // Kernel-space starts (not accessible from user mode)
    landmarks.push_back({{"name","Kernel Space"},      {"base","0xFFFF000000000000"}, {"end","0xFFFFFFFFFFFFFFFF"}, {"kind","kernel"},   {"color","#2a1a2a"}});

    auto proc = DK_MODEL_ACCESS->get_current_process();
    if (proc == nullptr)
        return root.dump();

    root["available"] = true;

    // -----------------------------------------------------------------------
    // Modules  (base, size, name, path)
    // -----------------------------------------------------------------------
    try
    {
        auto modules_obj = DK_MGET_POBJ(proc, "Modules");
        if (modules_obj != nullptr)
        {
            auto module_entries = DK_MODEL_ACCESS->iterate(modules_obj);
            for (auto& entry : module_entries)
            {
                auto mod = std::get<1>(entry);
                try
                {
                    auto info = DK_MGET_MODL(mod);   // (base, size, name)
                    uint64_t base = std::get<0>(info);
                    uint64_t size = std::get<1>(info);
                    std::string name = std::get<2>(info);
                    if (base == 0 && size == 0) continue;

                    // Attempt to read sections count via execute_cmd to keep
                    // things non-intrusive; fall back to empty sections array.
                    root["modules"].push_back({
                        {"name",    Basename(name)},
                        {"path",    name},
                        {"base",    FormatHex(base)},
                        {"end",     FormatHex(base + size)},
                        {"size",    size},
                        {"kind",    "module"}
                    });
                }
                catch (...) {}
            }
        }
    }
    catch (...) {}

    // -----------------------------------------------------------------------
    // Threads - stack base/limit via Environment.StackBase / StackLimit or
    // Stack.Attributes.StackBase / StackLimit (Model path depends on target).
    // -----------------------------------------------------------------------
    try
    {
        auto threads_obj = DK_MGET_POBJ(proc, "Threads");
        if (threads_obj != nullptr)
        {
            auto thread_entries = DK_MODEL_ACCESS->iterate(threads_obj);
            for (auto& entry : thread_entries)
            {
                auto thread_obj = std::get<1>(entry);
                if (thread_obj == nullptr) continue;

                uint64_t tid = 0;
                try { tid = DK_MGET_PVAL<uint64_t, VT_UI8>(thread_obj, "Id"); }
                catch (...) {
                    try { tid = DK_MGET_PVAL<DWORD, VT_UI4>(thread_obj, "Id"); }
                    catch (...) {}
                }

                uint64_t stack_base  = 0;
                uint64_t stack_limit = 0;

                // --- Try Environment.StackBase / StackLimit (TTD model path) ---
                auto TryGetU64 = [](DK_MOBJ_PTR& parent, const char* key, uint64_t& out) -> bool {
                    try {
                        out = DK_MGET_PVAL<uint64_t, VT_UI8>(parent, key);
                        return out != 0;
                    } catch (...) {}
                    try {
                        out = static_cast<uint64_t>(DK_MGET_PVAL<DWORD, VT_UI4>(parent, key));
                        return out != 0;
                    } catch (...) {}
                    try {
                        auto child = DK_MGET_POBJ(parent, key);
                        if (child) {
                            try { out = DK_MGET_VAL<uint64_t, VT_UI8>(child); return out != 0; } catch (...) {}
                            try { out = static_cast<uint64_t>(DK_MGET_VAL<DWORD, VT_UI4>(child)); return out != 0; } catch (...) {}
                        }
                    } catch (...) {}
                    return false;
                };

                // Priority order:
                // 1. Environment.StackBase / .StackLimit
                // 2. Stack.Attributes.StackBase / .StackLimit
                // 3. Environment.TEB -> TEB.StackBase / .StackLimit (kernel path)
                bool found_stack = false;
                try {
                    auto env = DK_MGET_POBJ(thread_obj, "Environment");
                    if (env) {
                        found_stack = TryGetU64(env, "StackBase", stack_base) &&
                                      TryGetU64(env, "StackLimit", stack_limit);
                    }
                } catch (...) {}

                if (!found_stack) {
                    try {
                        auto stk = DK_MGET_POBJ(thread_obj, "Stack");
                        if (stk) {
                            auto attr = DK_MGET_POBJ(stk, "Attributes");
                            if (attr) {
                                found_stack = TryGetU64(attr, "StackBase", stack_base) &&
                                              TryGetU64(attr, "StackLimit", stack_limit);
                            }
                        }
                    } catch (...) {}
                }

                // Proc symbol for display
                std::string proc_symbol;
                try { proc_symbol = BSTR2str(DK_MGET_PVAL<BSTR, VT_BSTR>(thread_obj, "ThreadProcSymbol")); } catch (...) {}
                if (proc_symbol.empty()) {
                    try { proc_symbol = BSTR2str(DK_MGET_PVAL<BSTR, VT_BSTR>(thread_obj, "StartSymbol")); } catch (...) {}
                }

                // TEB address (try Environment.EnvironmentBlock as a proxy, or direct TEB key)
                uint64_t teb_addr = 0;
                try {
                    auto env = DK_MGET_POBJ(thread_obj, "Environment");
                    if (env) TryGetU64(env, "TEB", teb_addr);
                } catch (...) {}
                if (teb_addr == 0) {
                    try { TryGetU64(thread_obj, "Teb", teb_addr); } catch (...) {}
                }
                if (teb_addr == 0) {
                    try { TryGetU64(thread_obj, "TEB", teb_addr); } catch (...) {}
                }

                json t = {
                    {"threadId",    tid},
                    {"procSymbol",  proc_symbol},
                    {"kind",        "threadStack"},
                    {"tebAddress",  teb_addr != 0 ? FormatHex(teb_addr) : ""}
                };

                if (found_stack && stack_base != 0 && stack_limit != 0)
                {
                    // Windows convention: StackBase > StackLimit (stack grows down)
                    uint64_t lo = std::min(stack_base, stack_limit);
                    uint64_t hi = std::max(stack_base, stack_limit);
                    t["stackBase"]  = FormatHex(stack_base);
                    t["stackLimit"] = FormatHex(stack_limit);
                    t["base"]       = FormatHex(lo);
                    t["end"]        = FormatHex(hi);
                    t["size"]       = hi - lo;
                    t["hasRange"]   = true;
                }
                else
                {
                    t["stackBase"]  = "";
                    t["stackLimit"] = "";
                    t["base"]       = "";
                    t["end"]        = "";
                    t["size"]       = 0;
                    t["hasRange"]   = false;
                }

                root["threads"].push_back(t);
            }
        }
    }
    catch (...) {}

    // -----------------------------------------------------------------------
    // Heaps - use get_heap_memory() for TTD traces; also try model path
    // Process.Heaps[n].Address / CommittedSize for non-TTD.
    // -----------------------------------------------------------------------
    if (is_ttd)
    {
        try
        {
            // TTD heap events give allocation address + size
            auto heap_events = DK_MODEL_ACCESS->get_heap_memory();
            // Build per-heap-base summary
            std::map<uint64_t, std::pair<uint64_t, size_t>> heap_map; // base -> (max_end, alloc_count)
            for (auto& ev : heap_events)
            {
                if (ev.res_id == 0) continue;
                uint64_t base = ev.res_id;       // allocation base address
                uint64_t sz   = ev.size;
                auto& slot = heap_map[base];
                slot.second++;
                if (sz > 0)
                    slot.first = std::max(slot.first, base + sz);
            }
            for (auto& [base, info] : heap_map)
            {
                uint64_t end = info.first > base ? info.first : base + 0x1000;
                root["heaps"].push_back({
                    {"base",        FormatHex(base)},
                    {"end",         FormatHex(end)},
                    {"size",        end - base},
                    {"allocCount",  info.second},
                    {"kind",        "heap"},
                    {"source",      "ttd"}
                });
            }
        }
        catch (...) {}
    }
    else
    {
        try
        {
            // Non-TTD: query Process.Heaps model collection
            auto heaps_obj = DK_MGET_POBJ(proc, "Heaps");
            if (heaps_obj != nullptr)
            {
                auto heap_entries = DK_MODEL_ACCESS->iterate(heaps_obj);
                for (auto& entry : heap_entries)
                {
                    auto heap_obj = std::get<1>(entry);
                    if (heap_obj == nullptr) continue;

                    uint64_t addr = 0;
                    uint64_t committed = 0;
                    try { addr = DK_MGET_PVAL<uint64_t, VT_UI8>(heap_obj, "Address"); } catch (...) {}
                    try { committed = DK_MGET_PVAL<uint64_t, VT_UI8>(heap_obj, "CommittedSize"); } catch (...) {}
                    if (addr == 0) continue;

                    if (committed == 0) committed = 0x10000; // 64 KB default
                    root["heaps"].push_back({
                        {"base",        FormatHex(addr)},
                        {"end",         FormatHex(addr + committed)},
                        {"size",        committed},
                        {"allocCount",  0},
                        {"kind",        "heap"},
                        {"source",      "model"}
                    });
                }
            }
        }
        catch (...) {}
    }

    // -----------------------------------------------------------------------
    // Process-level addresses (PEB, image base from first/main module)
    // -----------------------------------------------------------------------
    try
    {
        uint64_t peb_addr = 0;
        try { peb_addr = DK_MGET_PVAL<uint64_t, VT_UI8>(proc, "Peb"); } catch (...) {}
        if (peb_addr == 0) {
            try { peb_addr = DK_MGET_PVAL<uint64_t, VT_UI8>(proc, "PEB"); } catch (...) {}
        }
        if (peb_addr == 0) {
            try {
                auto env = DK_MGET_POBJ(proc, "Environment");
                if (env) {
                    try { peb_addr = DK_MGET_PVAL<uint64_t, VT_UI8>(env, "PEB"); } catch (...) {}
                }
            } catch (...) {}
        }

        root["pebAddress"] = peb_addr != 0 ? FormatHex(peb_addr) : "";
    }
    catch (...) {
        root["pebAddress"] = "";
    }

    return root.dump();
}

std::string CDkEmbeddedServer::HandleEnvironmentRoute(const std::string& query)
{
    (void)query;
    uint64_t request_id = ++m_request_counter;
    bool is_ttd = DK_MODEL_ACCESS->isTTD();

    auto FormatHex = [](uint64_t v) -> std::string {
        std::stringstream ss;
        ss << "0x" << std::hex << std::setw(16) << std::setfill('0') << v;
        return ss.str();
    };

    auto EmptyList = []() -> json {
        return json::array();
    };

    json root = {
        {"schemaVersion", "1.0"},
        {"requestId", std::to_string(request_id)},
        {"available", false},
        {"isTTD", is_ttd},
        {"process", {
            {"environmentBlockAddress", ""},
            {"pebAddress", ""},
            {"ldrAddress", ""},
            {"processParametersAddress", ""},
            {"imageBaseAddress", ""}
        }},
        {"threads", json::array()},
        {"ldrLists", {
            {"inLoadOrder", EmptyList()},
            {"inMemoryOrder", EmptyList()},
            {"inInitializationOrder", EmptyList()}
        }},
        {"pebFields", json::array()},
        {"ldrFields", json::array()},
        {"processParametersFields", json::array()},
        {"structures", json::array()},
        {"notes", json::array()}
    };

    auto proc = DK_MODEL_ACCESS->get_current_process();
    if (proc == nullptr)
        return root.dump();

    root["available"] = true;

    auto TryParseAddressFromText = [](const std::string& text, uint64_t& out) -> bool {
        if (text.empty()) return false;

        // Fast path: whole string is parseable numeric text.
        if (TryParseU64(TrimAsciiWhitespace(text), out) && out != 0) {
            return true;
        }

        // Fallback: extract first 0x<hex> token from display text like "Address 0x7ff6abcd0000".
        const size_t n = text.size();
        for (size_t i = 0; i + 2 < n; ++i)
        {
            if (text[i] != '0' || (text[i + 1] != 'x' && text[i + 1] != 'X'))
                continue;

            size_t j = i + 2;
            while (j < n && std::isxdigit(static_cast<unsigned char>(text[j])) != 0)
                ++j;

            if (j == i + 2)
                continue;

            const std::string token = text.substr(i, j - i);
            if (TryParseU64(token, out) && out != 0)
                return true;
        }

        return false;
    };

    auto TryGetU64 = [&](DK_MOBJ_PTR& parent, const char* key, uint64_t& out) -> bool {
        auto TryParseAddressFromTextLocal = [&](const std::string& text) -> bool {
            return TryParseAddressFromText(text, out);
        };

        try {
            out = DK_MGET_PVAL<uint64_t, VT_UI8>(parent, key);
            return out != 0;
        } catch (...) {}
        try {
            out = static_cast<uint64_t>(DK_MGET_PVAL<DWORD, VT_UI4>(parent, key));
            return out != 0;
        } catch (...) {}
        try {
            auto child = DK_MGET_POBJ(parent, key);
            if (child != nullptr) {
                try { out = DK_MGET_VAL<uint64_t, VT_UI8>(child); return out != 0; } catch (...) {}
                try { out = static_cast<uint64_t>(DK_MGET_VAL<DWORD, VT_UI4>(child)); return out != 0; } catch (...) {}
            }
        } catch (...) {}
        try {
            const std::string text = BSTR2str(DK_MGET_PVAL<BSTR, VT_BSTR>(parent, key));
            if (TryParseAddressFromTextLocal(text)) return true;
        } catch (...) {}
        try {
            auto child = DK_MGET_POBJ(parent, key);
            if (child != nullptr) {
                try {
                    const std::string text = BSTR2str(DK_MGET_VAL<BSTR, VT_BSTR>(child));
                    if (TryParseAddressFromTextLocal(text)) return true;
                } catch (...) {}
            }
        } catch (...) {}
        return false;
    };

    auto TryGetString = [](DK_MOBJ_PTR& parent, const char* key, std::string& out) -> bool {
        try {
            out = BSTR2str(DK_MGET_PVAL<BSTR, VT_BSTR>(parent, key));
            return !out.empty();
        } catch (...) {}
        try {
            auto child = DK_MGET_POBJ(parent, key);
            if (child != nullptr) {
                try {
                    out = BSTR2str(DK_MGET_VAL<BSTR, VT_BSTR>(child));
                    return !out.empty();
                } catch (...) {}
            }
        } catch (...) {}
        return false;
    };

    auto TryGetObjU64 = [&](DK_MOBJ_PTR& obj, uint64_t& out) -> bool {
        if (obj == nullptr) return false;

        auto TryParseAddressFromTextLocal = [&](const std::string& text) -> bool {
            return TryParseAddressFromText(text, out);
        };

        try { out = DK_MGET_VAL<uint64_t, VT_UI8>(obj); return out != 0; } catch (...) {}
        try { out = static_cast<uint64_t>(DK_MGET_VAL<DWORD, VT_UI4>(obj)); return out != 0; } catch (...) {}
        try {
            const std::string text = BSTR2str(DK_MGET_VAL<BSTR, VT_BSTR>(obj));
            if (TryParseAddressFromTextLocal(text)) return true;
        } catch (...) {}

        // Some model objects expose location through metadata-like keys.
        const char* addr_keys[] = {
            "Address", "ObjectAddress", "TargetLocation", "Location", "Pointer", "Ptr", "Value"
        };
        for (const auto* key : addr_keys)
        {
            try {
                out = DK_MGET_PVAL<uint64_t, VT_UI8>(obj, key);
                if (out != 0) return true;
            } catch (...) {}
            try {
                const std::string text = BSTR2str(DK_MGET_PVAL<BSTR, VT_BSTR>(obj, key));
                if (TryParseAddressFromTextLocal(text)) return true;
            } catch (...) {}
            try {
                auto child = DK_MGET_POBJ(obj, key);
                if (child != nullptr) {
                    try { out = DK_MGET_VAL<uint64_t, VT_UI8>(child); if (out != 0) return true; } catch (...) {}
                    try { out = static_cast<uint64_t>(DK_MGET_VAL<DWORD, VT_UI4>(child)); if (out != 0) return true; } catch (...) {}
                    try {
                        const std::string text = BSTR2str(DK_MGET_VAL<BSTR, VT_BSTR>(child));
                        if (TryParseAddressFromTextLocal(text)) return true;
                    } catch (...) {}
                }
            } catch (...) {}
        }

        return false;
    };

    auto VariantTypeName = [](VARTYPE vt) -> std::string {
        switch (vt)
        {
        case VT_EMPTY: return "VT_EMPTY";
        case VT_NULL: return "VT_NULL";
        case VT_I1: return "VT_I1";
        case VT_UI1: return "VT_UI1";
        case VT_I2: return "VT_I2";
        case VT_UI2: return "VT_UI2";
        case VT_I4: return "VT_I4";
        case VT_UI4: return "VT_UI4";
        case VT_I8: return "VT_I8";
        case VT_UI8: return "VT_UI8";
        case VT_INT: return "VT_INT";
        case VT_UINT: return "VT_UINT";
        case VT_BOOL: return "VT_BOOL";
        case VT_BSTR: return "VT_BSTR";
        case VT_UNKNOWN: return "VT_UNKNOWN";
        case VT_DISPATCH: return "VT_DISPATCH";
        default: {
            std::stringstream ss;
            ss << "VT_" << std::hex << vt;
            return ss.str();
        }
        }
    };

    auto VariantToDisplay = [&](const VARIANT& v) -> json {
        json result = {
            {"value", ""},
            {"valueType", ""}
        };
        result["valueType"] = VariantTypeName(v.vt);

        std::stringstream ss;
        switch (v.vt)
        {
        case VT_BSTR:
            result["value"] = BSTR2str(v.bstrVal);
            break;
        case VT_BOOL:
            result["value"] = (v.boolVal == VARIANT_TRUE) ? "true" : "false";
            break;
        case VT_UI1:
            ss << "0x" << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(v.bVal);
            result["value"] = ss.str();
            result["u64"] = static_cast<uint64_t>(v.bVal);
            break;
        case VT_UI2:
            ss << "0x" << std::hex << std::setw(4) << std::setfill('0') << v.uiVal;
            result["value"] = ss.str();
            result["u64"] = static_cast<uint64_t>(v.uiVal);
            break;
        case VT_UI4:
            ss << "0x" << std::hex << std::setw(8) << std::setfill('0') << v.ulVal;
            result["value"] = ss.str();
            result["u64"] = static_cast<uint64_t>(v.ulVal);
            break;
        case VT_UI8:
            ss << "0x" << std::hex << std::setw(16) << std::setfill('0') << v.ullVal;
            result["value"] = ss.str();
            result["u64"] = static_cast<uint64_t>(v.ullVal);
            break;
        case VT_I1:
            ss << static_cast<int>(v.cVal);
            result["value"] = ss.str();
            result["i64"] = static_cast<int64_t>(v.cVal);
            break;
        case VT_I2:
            ss << v.iVal;
            result["value"] = ss.str();
            result["i64"] = static_cast<int64_t>(v.iVal);
            break;
        case VT_I4:
        case VT_INT:
            ss << v.lVal;
            result["value"] = ss.str();
            result["i64"] = static_cast<int64_t>(v.lVal);
            break;
        case VT_I8:
            ss << v.llVal;
            result["value"] = ss.str();
            result["i64"] = static_cast<int64_t>(v.llVal);
            break;
        default:
            if (v.vt == VT_EMPTY) {
                result["value"] = "(empty)";
            } else if (v.vt == VT_NULL) {
                result["value"] = "(null)";
            } else {
                ss << "<" << std::string(result["valueType"]) << ">";
                result["value"] = ss.str();
            }
            break;
        }

        return result;
    };

    auto ExtractObjectFields = [&](DK_MOBJ_PTR& obj, json& out_fields, size_t max_fields = 768) {
        if (obj == nullptr) return;
        std::vector<std::tuple<std::string, std::string, DK_MOBJ_PTR>> kvs;
        try { kvs = DK_MODEL_ACCESS->enum_keyvalues(obj); } catch (...) {}
        size_t emitted = 0;
        for (auto& kv : kvs)
        {
            if (emitted >= max_fields) {
                out_fields.push_back({
                    {"name", "..."},
                    {"kind", "Synthetic"},
                    {"value", "(truncated)"},
                    {"valueType", "note"}
                });
                break;
            }

            const auto& key_name = std::get<0>(kv);
            const auto& key_kind = std::get<1>(kv);
            auto key_obj = std::get<2>(kv);

            json field = {
                {"name", key_name},
                {"kind", key_kind},
                {"value", ""},
                {"valueType", ""}
            };

            if (key_obj != nullptr && key_kind == "Intrinsic")
            {
                VARIANT v;
                VariantInit(&v);
                HRESULT hr = key_obj->GetIntrinsicValue(&v);
                if (SUCCEEDED(hr)) {
                    auto value_json = VariantToDisplay(v);
                    field["value"] = value_json["value"];
                    field["valueType"] = value_json["valueType"];
                    if (value_json.contains("u64")) field["u64"] = value_json["u64"];
                    if (value_json.contains("i64")) field["i64"] = value_json["i64"];
                }
                VariantClear(&v);
            }
            else
            {
                field["value"] = "(" + key_kind + ")";
                field["valueType"] = key_kind;
            }

            out_fields.push_back(field);
            emitted++;
        }
    };

    struct ModuleInfo
    {
        std::string name;
        std::string path;
        uint64_t base{ 0 };
        uint64_t size{ 0 };
    };

    std::vector<ModuleInfo> module_infos;
    std::map<uint64_t, ModuleInfo> modules_by_base;
    try
    {
        auto modules_obj = DK_MGET_POBJ(proc, "Modules");
        if (modules_obj != nullptr)
        {
            auto module_entries = DK_MODEL_ACCESS->iterate(modules_obj);
            for (auto& entry : module_entries)
            {
                auto mod = std::get<1>(entry);
                if (mod == nullptr) continue;
                try
                {
                    auto info = DK_MGET_MODL(mod);
                    ModuleInfo item;
                    item.base = std::get<0>(info);
                    item.size = std::get<1>(info);
                    item.path = std::get<2>(info);
                    item.name = Basename(item.path);
                    if (item.base == 0) continue;
                    module_infos.push_back(item);
                    modules_by_base[item.base] = item;
                }
                catch (...) {}
            }
        }
    }
    catch (...) {}

    auto DecorateListLinks = [](json& arr) {
        if (!arr.is_array()) return;
        for (size_t i = 0; i < arr.size(); ++i)
        {
            arr[i]["orderIndex"] = i;
            arr[i]["prevName"] = i > 0 ? arr[i - 1]["name"] : "";
            arr[i]["nextName"] = (i + 1) < arr.size() ? arr[i + 1]["name"] : "";
        }
    };

    auto MakeModuleJson = [&](const std::string& list_name, const std::string& source, size_t index, const ModuleInfo& mod) -> json {
        return json{
            {"listName", list_name},
            {"source", source},
            {"orderIndex", index},
            {"name", mod.name},
            {"path", mod.path},
            {"base", mod.base != 0 ? FormatHex(mod.base) : ""},
            {"end", (mod.base != 0 && mod.size != 0) ? FormatHex(mod.base + mod.size) : ""},
            {"size", mod.size}
        };
    };

    DK_MOBJ_PTR proc_env;
    DK_MOBJ_PTR peb_obj;
    DK_MOBJ_PTR ldr_obj;
    DK_MOBJ_PTR proc_params_obj;
    uint64_t peb_addr = 0;
    uint64_t ldr_addr = 0;
    uint64_t proc_params_addr = 0;
    uint64_t image_base = 0;

    try { proc_env = DK_MGET_POBJ(proc, "Environment"); } catch (...) {}
    if (proc_env != nullptr)
    {
        TryGetU64(proc_env, "EnvironmentBlock", peb_addr);
        if (peb_addr == 0) TryGetU64(proc_env, "PEB", peb_addr);
        if (peb_addr == 0) TryGetU64(proc_env, "Peb", peb_addr);
        if (peb_addr == 0) TryGetU64(proc_env, "EnvironmentBlockAddress", peb_addr);
        TryGetU64(proc_env, "ImageBaseAddress", image_base);
        try { peb_obj = DK_MGET_POBJ(proc_env, "EnvironmentBlock"); } catch (...) {}
        if (peb_obj == nullptr) {
            try { peb_obj = DK_MGET_POBJ(proc_env, "PEB"); } catch (...) {}
        }
        if (peb_obj == nullptr) {
            try { peb_obj = DK_MGET_POBJ(proc_env, "Peb"); } catch (...) {}
        }

        if (peb_addr == 0 && peb_obj != nullptr) {
            TryGetObjU64(peb_obj, peb_addr);
        }
    }

    if (peb_addr == 0) {
        try { TryGetU64(proc, "Peb", peb_addr); } catch (...) {}
    }
    if (peb_addr == 0) {
        try { TryGetU64(proc, "PEB", peb_addr); } catch (...) {}
    }
    if (peb_addr == 0) {
        try { TryGetU64(proc, "EnvironmentBlock", peb_addr); } catch (...) {}
    }
    if (peb_addr == 0 && peb_obj != nullptr) {
        TryGetObjU64(peb_obj, peb_addr);
    }
    if (image_base == 0 && !module_infos.empty()) {
        image_base = module_infos.front().base;
    }

    if (peb_obj != nullptr)
    {
        TryGetU64(peb_obj, "Ldr", ldr_addr);
        if (ldr_addr == 0) TryGetU64(peb_obj, "LoaderData", ldr_addr);
        TryGetU64(peb_obj, "ProcessParameters", proc_params_addr);

        try { ldr_obj = DK_MGET_POBJ(peb_obj, "Ldr"); } catch (...) {}
        if (ldr_obj == nullptr) {
            try { ldr_obj = DK_MGET_POBJ(peb_obj, "LoaderData"); } catch (...) {}
        }
        try { proc_params_obj = DK_MGET_POBJ(peb_obj, "ProcessParameters"); } catch (...) {}
    }

    root["process"]["environmentBlockAddress"] = peb_addr != 0 ? FormatHex(peb_addr) : "";
    root["process"]["pebAddress"] = peb_addr != 0 ? FormatHex(peb_addr) : "";
    root["process"]["ldrAddress"] = ldr_addr != 0 ? FormatHex(ldr_addr) : "";
    root["process"]["processParametersAddress"] = proc_params_addr != 0 ? FormatHex(proc_params_addr) : "";
    root["process"]["imageBaseAddress"] = image_base != 0 ? FormatHex(image_base) : "";

    if (peb_addr != 0) {
        root["structures"].push_back({{"type", "PEB"}, {"address", FormatHex(peb_addr)}, {"source", "process.environment.environmentblock"}});
    }
    if (ldr_addr != 0) {
        root["structures"].push_back({{"type", "LDR"}, {"address", FormatHex(ldr_addr)}, {"source", "peb.ldr"}});
    }
    if (proc_params_addr != 0) {
        root["structures"].push_back({{"type", "ProcessParameters"}, {"address", FormatHex(proc_params_addr)}, {"source", "peb.processparameters"}});
    }

    if (peb_obj != nullptr) {
        ExtractObjectFields(peb_obj, root["pebFields"]);
    }
    if (ldr_obj != nullptr) {
        ExtractObjectFields(ldr_obj, root["ldrFields"]);
    }
    if (proc_params_obj != nullptr) {
        ExtractObjectFields(proc_params_obj, root["processParametersFields"]);
    }

    try
    {
        auto threads_obj = DK_MGET_POBJ(proc, "Threads");
        if (threads_obj != nullptr)
        {
            auto thread_entries = DK_MODEL_ACCESS->iterate(threads_obj);
            for (auto& entry : thread_entries)
            {
                auto thread_obj = std::get<1>(entry);
                if (thread_obj == nullptr) continue;

                uint64_t tid = 0;
                uint64_t teb_addr = 0;
                uint64_t stack_base = 0;
                uint64_t stack_limit = 0;
                DK_MOBJ_PTR thread_env;
                DK_MOBJ_PTR teb_obj;

                try { tid = DK_MGET_PVAL<uint64_t, VT_UI8>(thread_obj, "Id"); } catch (...) {
                    try { tid = DK_MGET_PVAL<DWORD, VT_UI4>(thread_obj, "Id"); } catch (...) {}
                }

                try { thread_env = DK_MGET_POBJ(thread_obj, "Environment"); } catch (...) {}
                if (thread_env != nullptr)
                {
                    TryGetU64(thread_env, "EnvironmentBlock", teb_addr);
                    if (teb_addr == 0) TryGetU64(thread_env, "TEB", teb_addr);
                    TryGetU64(thread_env, "StackBase", stack_base);
                    TryGetU64(thread_env, "StackLimit", stack_limit);
                    try { teb_obj = DK_MGET_POBJ(thread_env, "EnvironmentBlock"); } catch (...) {}
                }
                if (teb_addr == 0) {
                    TryGetU64(thread_obj, "Teb", teb_addr);
                }
                if (teb_addr == 0) {
                    TryGetU64(thread_obj, "TEB", teb_addr);
                }

                json thread_json = {
                    {"threadId", tid},
                    {"environmentBlockAddress", teb_addr != 0 ? FormatHex(teb_addr) : ""},
                    {"tebAddress", teb_addr != 0 ? FormatHex(teb_addr) : ""},
                    {"stackBase", stack_base != 0 ? FormatHex(stack_base) : ""},
                    {"stackLimit", stack_limit != 0 ? FormatHex(stack_limit) : ""},
                    {"environmentFields", json::array()},
                    {"tebFields", json::array()}
                };

                if (thread_env != nullptr) {
                    ExtractObjectFields(thread_env, thread_json["environmentFields"], 256);
                }
                if (teb_obj != nullptr) {
                    ExtractObjectFields(teb_obj, thread_json["tebFields"], 512);
                }

                root["threads"].push_back(thread_json);
                if (teb_addr != 0) {
                    root["structures"].push_back({{"type", "TEB"}, {"threadId", tid}, {"address", FormatHex(teb_addr)}, {"source", "thread.environment.environmentblock"}});
                }
            }
        }
    }
    catch (...) {}

    auto TryExtractLdrList = [&](DK_MOBJ_PTR& parent, const std::vector<std::string>& keys, const std::string& list_name, json& target) -> bool {
        for (const auto& key : keys)
        {
            DK_MOBJ_PTR list_obj;
            try { list_obj = DK_MGET_POBJ(parent, key); } catch (...) {}
            if (list_obj == nullptr) continue;

            auto entries = DK_MODEL_ACCESS->iterate(list_obj);
            if (entries.empty()) continue;

            for (size_t index = 0; index < entries.size(); ++index)
            {
                auto module_obj = std::get<1>(entries[index]);
                if (module_obj == nullptr) continue;

                ModuleInfo mod;
                TryGetU64(module_obj, "DllBase", mod.base);
                if (mod.base == 0) TryGetU64(module_obj, "BaseAddress", mod.base);
                if (mod.base == 0) TryGetU64(module_obj, "Base", mod.base);
                TryGetU64(module_obj, "SizeOfImage", mod.size);
                if (mod.size == 0) TryGetU64(module_obj, "Size", mod.size);
                TryGetString(module_obj, "BaseDllName", mod.name);
                if (mod.name.empty()) TryGetString(module_obj, "Name", mod.name);
                TryGetString(module_obj, "FullDllName", mod.path);
                if (mod.path.empty()) TryGetString(module_obj, "Path", mod.path);
                if (mod.name.empty() && mod.base != 0)
                {
                    auto found = modules_by_base.find(mod.base);
                    if (found != modules_by_base.end()) {
                        mod.name = found->second.name;
                        mod.path = found->second.path;
                        if (mod.size == 0) mod.size = found->second.size;
                    }
                }
                if (mod.name.empty() && mod.base == 0) continue;
                target.push_back(MakeModuleJson(list_name, "ldr", index, mod));
            }

            if (!target.empty()) {
                DecorateListLinks(target);
                return true;
            }
        }

        return false;
    };

    bool have_load_order = false;
    bool have_memory_order = false;
    bool have_init_order = false;
    if (ldr_obj != nullptr)
    {
        have_load_order = TryExtractLdrList(ldr_obj,
            {"InLoadOrderModuleList", "LoadOrder", "InLoadOrder"},
            "InLoadOrderModuleList",
            root["ldrLists"]["inLoadOrder"]);
        have_memory_order = TryExtractLdrList(ldr_obj,
            {"InMemoryOrderModuleList", "MemoryOrder", "InMemoryOrder"},
            "InMemoryOrderModuleList",
            root["ldrLists"]["inMemoryOrder"]);
        have_init_order = TryExtractLdrList(ldr_obj,
            {"InInitializationOrderModuleList", "InitializationOrder", "InInitializationOrder"},
            "InInitializationOrderModuleList",
            root["ldrLists"]["inInitializationOrder"]);
    }

    if (!have_load_order && !module_infos.empty())
    {
        for (size_t i = 0; i < module_infos.size(); ++i)
            root["ldrLists"]["inLoadOrder"].push_back(MakeModuleJson("InLoadOrderModuleList", "fallback-modules", i, module_infos[i]));
        DecorateListLinks(root["ldrLists"]["inLoadOrder"]);
        root["notes"].push_back("InLoadOrderModuleList fell back to Process.Modules enumeration.");
    }

    if (!have_memory_order && !module_infos.empty())
    {
        auto sorted = module_infos;
        std::sort(sorted.begin(), sorted.end(), [](const ModuleInfo& lhs, const ModuleInfo& rhs) {
            return lhs.base < rhs.base;
        });
        for (size_t i = 0; i < sorted.size(); ++i)
            root["ldrLists"]["inMemoryOrder"].push_back(MakeModuleJson("InMemoryOrderModuleList", "fallback-modules", i, sorted[i]));
        DecorateListLinks(root["ldrLists"]["inMemoryOrder"]);
        root["notes"].push_back("InMemoryOrderModuleList fell back to base-address ordering.");
    }

    if (!have_init_order && !module_infos.empty())
    {
        for (size_t i = 0; i < module_infos.size(); ++i)
            root["ldrLists"]["inInitializationOrder"].push_back(MakeModuleJson("InInitializationOrderModuleList", "fallback-modules", i, module_infos[i]));
        DecorateListLinks(root["ldrLists"]["inInitializationOrder"]);
        root["notes"].push_back("InInitializationOrderModuleList fell back to Process.Modules enumeration.");
    }

    return root.dump();
}

// ---------------------------------------------------------------------------
// /api/ttd/mem-access
// Queries TTD memory access events for a given address range and access mode.
// Parameters:
//   start_addr  - hex start address (required)
//   end_addr    - hex end address (required)
//   mode        - access type filter: R (read), W (write), E (execute), C (all) (default: W)
//   maxResults  - maximum results to return (default: 500, max: 5000)
//   timeoutMs   - soft timeout in milliseconds (default: 30000, max: 120000)
//
// Returns a structured JSON array of ttd_mem_access records with timing
// metadata. Sets timedOut=true when the soft timeout expires before all
// results are collected.
// ---------------------------------------------------------------------------
std::string CDkEmbeddedServer::HandleTtdMemAccessRoute(const std::string& query)
{
    uint64_t request_id = ++m_request_counter;
    uint64_t start_time_ms = NowMs();

    auto FormatHexU64 = [](uint64_t value) -> std::string {
        std::stringstream ss;
        ss << "0x" << std::hex << std::setw(16) << std::setfill('0') << value;
        return ss.str();
    };

    auto FormatHexCompact = [](uint64_t value) -> std::string {
        std::stringstream ss;
        ss << "0x" << std::hex << value;
        return ss.str();
    };

    json root = {
        {"schemaVersion", "1.0"},
        {"requestId", std::to_string(request_id)},
        {"available", false},
        {"accesses", json::array()},
        {"collectedCount", 0},
        {"timedOut", false},
        {"query", {
            {"startAddr", "0x0"},
            {"endAddr", "0x0"},
            {"mode", "W"}
        }},
        {"timing", {
            {"elapsedMs", 0},
            {"timeoutMs", 30000}
        }}
    };

    auto params = ParseQueryParams(query);

    // --- Parse required: start_addr ---
    std::string start_addr_str;
    {
        auto it = params.find("start_addr");
        if (it != params.end())
            start_addr_str = TrimAsciiWhitespace(it->second);
    }
    if (start_addr_str.empty())
    {
        auto it = params.find("start");
        if (it != params.end())
            start_addr_str = TrimAsciiWhitespace(it->second);
    }

    uint64_t start_addr = 0;
    if (start_addr_str.empty() || !TryParseU64(start_addr_str, start_addr))
    {
        root["message"] = "Query parameter 'start_addr' is required.";
        root["timing"]["elapsedMs"] = NowMs() - start_time_ms;
        return root.dump();
    }
    root["query"]["startAddr"] = FormatHexU64(start_addr);

    // --- Parse required: end_addr ---
    std::string end_addr_str;
    {
        auto it = params.find("end_addr");
        if (it != params.end())
            end_addr_str = TrimAsciiWhitespace(it->second);
    }
    if (end_addr_str.empty())
    {
        auto it = params.find("end");
        if (it != params.end())
            end_addr_str = TrimAsciiWhitespace(it->second);
    }

    uint64_t end_addr = 0;
    if (end_addr_str.empty() || !TryParseU64(end_addr_str, end_addr))
    {
        root["message"] = "Query parameter 'end_addr' is required.";
        root["timing"]["elapsedMs"] = NowMs() - start_time_ms;
        return root.dump();
    }
    root["query"]["endAddr"] = FormatHexU64(end_addr);

    if (end_addr <= start_addr)
    {
        root["message"] = "end_addr must be greater than start_addr.";
        root["timing"]["elapsedMs"] = NowMs() - start_time_ms;
        return root.dump();
    }

    // --- Parse optional: mode ---
    std::string mode = "W";
    {
        auto it = params.find("mode");
        if (it != params.end())
        {
            std::string raw = ToLowerAscii(TrimAsciiWhitespace(it->second));
            if (raw == "r" || raw == "read")
                mode = "R";
            else if (raw == "w" || raw == "write")
                mode = "W";
            else if (raw == "e" || raw == "exec" || raw == "execute")
                mode = "E";
            else if (raw == "c" || raw == "all")
                mode = "C";
            else if (raw == "rw")
                mode = "RW";
        }
    }
    root["query"]["mode"] = mode;

    // --- Parse optional: maxResults ---
    uint64_t max_results = 500;
    {
        auto it = params.find("maxResults");
        if (it == params.end())
            it = params.find("limit");
        if (it != params.end())
        {
            uint64_t parsed = 0;
            if (TryParseU64(it->second, parsed) && parsed > 0)
                max_results = std::min<uint64_t>(parsed, 5000);
        }
    }
    root["query"]["maxResults"] = max_results;

    // --- Parse optional: timeoutMs ---
    uint64_t timeout_ms = 30000;
    {
        auto it = params.find("timeoutMs");
        if (it == params.end())
            it = params.find("timeout");
        if (it != params.end())
        {
            uint64_t parsed = 0;
            if (TryParseU64(it->second, parsed) && parsed > 0)
                timeout_ms = std::min<uint64_t>(parsed, 120000);
        }
    }
    root["timing"]["timeoutMs"] = timeout_ms;

    // --- Parse optional: time range (major:minor position bounds) ---
    uint64_t ts_major = 0, ts_minor = 0, te_major = 0, te_minor = 0;
    {
        auto it = params.find("timeStartMajor");
        if (it != params.end()) TryParseU64(it->second, ts_major);
        it = params.find("timeStartMinor");
        if (it != params.end()) TryParseU64(it->second, ts_minor);
        it = params.find("timeEndMajor");
        if (it != params.end()) TryParseU64(it->second, te_major);
        it = params.find("timeEndMinor");
        if (it != params.end()) TryParseU64(it->second, te_minor);
    }
    bool has_time_range = (ts_major != 0 || ts_minor != 0 || te_major != 0 || te_minor != 0);
    if (has_time_range) {
        root["query"]["timeStart"] = { {"major", std::to_string(ts_major)}, {"minor", ts_minor} };
        root["query"]["timeEnd"]   = { {"major", std::to_string(te_major)}, {"minor", te_minor} };
    }

    // --- TTD guard ---
    if (!DK_MODEL_ACCESS->isTTD())
    {
        root["message"] = "TTD memory access query requires TTD mode.";
        root["timing"]["elapsedMs"] = NowMs() - start_time_ms;
        return root.dump();
    }

    // --- Execute query ---
    auto accesses = has_time_range
        ? DK_MODEL_ACCESS->get_mem_access(start_addr, end_addr, mode,
                                          ts_major, ts_minor, te_major, te_minor)
        : DK_MODEL_ACCESS->get_mem_access(start_addr, end_addr, mode);

    size_t collected = 0;
    bool timed_out = false;

    for (auto& access : accesses)
    {
        // --- Timeout check ---
        uint64_t elapsed = NowMs() - start_time_ms;
        if (elapsed > timeout_ms)
        {
            timed_out = true;
            break;
        }

        // --- Max results check ---
        if (collected >= max_results)
            break;

        // --- Resolve symbol for IP ---
        std::string ip_symbol;
        uint64_t ip_displacement = 0;
        try
        {
            auto sym_info = EXT_F_Addr2Sym(access.ip_addr);
            ip_symbol = std::get<0>(sym_info);
            ip_displacement = std::get<1>(sym_info);
        }
        catch (...) {}

        std::string display_ip = ip_symbol.empty()
            ? FormatHexU64(access.ip_addr)
            : (ip_symbol + "+0x" + 
               ([](uint64_t v){ std::stringstream s; s << std::hex << v; return s.str(); })(ip_displacement));

        root["accesses"].push_back({
            {"startPos", {
                {"major", std::to_string(std::get<0>(access.start_pos))},
                {"minor", std::get<1>(access.start_pos)}
            }},
            {"endPos", {
                {"major", std::to_string(std::get<0>(access.end_pos))},
                {"minor", std::get<1>(access.end_pos)}
            }},
            {"accessType", access.access_type},
            {"ip", FormatHexU64(access.ip_addr)},
            {"ipSymbol", display_ip},
            {"address", FormatHexU64(access.addr)},
            {"size", access.size},
            {"value", FormatHexCompact(access.value)},
            {"overwrittenValue", FormatHexCompact(access.overwritten_value)},
            {"threadId", access.thread_id},
            {"eventType", access.event_type}
        });

        ++collected;
    }

    uint64_t total_elapsed = NowMs() - start_time_ms;

    root["available"] = collected > 0;
    root["collectedCount"] = collected;
    root["totalCount"] = accesses.size();
    root["timedOut"] = timed_out;
    root["timing"]["elapsedMs"] = total_elapsed;

    if (accesses.empty())
        root["message"] = "No memory access events found in the given range.";
    else if (timed_out)
        root["message"] = "Soft timeout reached; " + std::to_string(collected) +
                          " of " + std::to_string(accesses.size()) +
                          " results returned.";
    else if (collected < accesses.size())
        root["message"] = "maxResults limit reached; " + std::to_string(collected) +
                          " of " + std::to_string(accesses.size()) +
                          " results returned. Increase maxResults (max 5000) or narrow the address range.";

    return root.dump();
}

std::string CDkEmbeddedServer::HandleCapabilitiesRoute()
{
    json root;

    // Server metadata
    root["server"] = {
        {"schemaVersion", "1.0"},
        {"name", "dk embedded server"},
        {"phase", 2},
        {"uptimeMs", std::to_string(GetUptimeMs())},
        {"requestCount", std::to_string(m_request_counter.load())},
    };

    // Session information
    json session;
    session["isTTD"] = DK_MODEL_ACCESS->isTTD();
    session["isUsermode"] = DK_MODEL_ACCESS->isUsermode();
    session["isKernelmode"] = DK_MODEL_ACCESS->isKernelmode();
    session["isDump"] = DK_MODEL_ACCESS->isDump();
    session["isLive"] = DK_MODEL_ACCESS->isLive();
    session["isNT"] = DK_MODEL_ACCESS->isNT();

    // Architecture information.
    // sizeof(size_t) reflects the host debugger's pointer size:
    //   4 on 32-bit (x86) WinDbg, 8 on 64-bit (x64/ARM64) WinDbg.
    // This is the correct value for pointer-size-sensitive dispatch
    // in WinDbg extensions, which must match the debugger's bitness.
    ULONG ptr_size = static_cast<ULONG>(sizeof(size_t));
    session["pointerSize"] = static_cast<int>(ptr_size);
    session["is64Bit"] = (ptr_size == 8);

    root["session"] = session;

    // Available routes
    json routes = json::array();
    routes.push_back("/api/server/status");
    routes.push_back("/api/server/stop");
    routes.push_back("/api/ttd/trace-info");
    routes.push_back("/api/ttd/modules");
    routes.push_back("/api/ttd/threads");
    routes.push_back("/api/ttd/events/lifetime");
    routes.push_back("/api/registers");
    routes.push_back("/api/callstack");
    routes.push_back("/api/page");
    routes.push_back("/api/page/svg");
    routes.push_back("/api/function-calls");
    routes.push_back("/api/command/execute");
    routes.push_back("/api/model");
    routes.push_back("/api/pe");
    routes.push_back("/api/strings");
    routes.push_back("/api/memory/layout");
    routes.push_back("/api/environment");
    routes.push_back("/api/capabilities");
    root["routes"] = routes;

    return root.dump();
}
