#pragma once
#include <windows.h>
#include <winhttp.h>
#include <functional>
#include <string>

namespace net {

using Sink = std::function<bool(const char* data, size_t n, unsigned long long total)>;

inline bool get(const std::string& url, const Sink& sink, std::string& error) {
    std::wstring wurl(url.begin(), url.end());
    URL_COMPONENTS parts{sizeof parts};
    wchar_t host[256] = {}, path[2048] = {};
    parts.lpszHostName = host, parts.dwHostNameLength = 256;
    parts.lpszUrlPath = path, parts.dwUrlPathLength = 2048;
    if (!WinHttpCrackUrl(wurl.c_str(), 0, 0, &parts) || parts.nScheme != INTERNET_SCHEME_HTTPS) return error = "bad address", false;
    HINTERNET session = WinHttpOpen(L"FaTrainer/" FATRAINER_VERSION, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) return error = "no network", false;
    const int TIMEOUT_MS = 8000;
    WinHttpSetTimeouts(session, TIMEOUT_MS, TIMEOUT_MS, TIMEOUT_MS, TIMEOUT_MS);
    HINTERNET connection = WinHttpConnect(session, host, parts.nPort, 0);
    HINTERNET request = connection ? WinHttpOpenRequest(connection, L"GET", path, nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE) : nullptr;
    bool ok = request && WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) &&
              WinHttpReceiveResponse(request, nullptr);
    DWORD status = 0, size = sizeof status;
    if (ok) WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);
    if (!ok) error = "could not reach GitHub (error " + std::to_string(GetLastError()) + ")";
    else if (status != 200) ok = false, error = "GitHub answered " + std::to_string(status);
    unsigned long long total = 0;
    wchar_t length[32];
    DWORD length_size = sizeof length;
    if (ok && WinHttpQueryHeaders(request, WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX, length, &length_size, WINHTTP_NO_HEADER_INDEX))
        total = _wcstoui64(length, nullptr, 10);
    char buffer[16384];
    for (DWORD got = 0; ok;) {
        if (!WinHttpReadData(request, buffer, sizeof buffer, &got)) { ok = false, error = "download interrupted"; break; }
        if (!got) break;
        if (!sink(buffer, got, total)) { ok = false, error = "cancelled"; break; }
    }
    if (request) WinHttpCloseHandle(request);
    if (connection) WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return ok;
}

inline bool get_text(const std::string& url, std::string& out, std::string& error) {
    const size_t LIMIT = 1 << 20;
    out.clear();
    return get(url, [&](const char* d, size_t n, unsigned long long) { out.append(d, n); return out.size() < LIMIT; }, error);
}

}
