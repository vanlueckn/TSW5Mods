#include "restclient.hpp"

TSWShared::RestClient::RestClient(const std::wstring& host, const INTERNET_PORT port,
                                  const bool use_https)
    : host_(host), port_(port), use_https_(use_https)
{
    h_session_ = WinHttpOpen(L"RESTClient/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                             WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

    if (!h_session_)
    {
        throw std::runtime_error("Failed to open WinHTTP session");
    }

    h_connect_ = WinHttpConnect(h_session_, host.c_str(), port, 0);
    if (!h_connect_)
    {
        WinHttpCloseHandle(h_session_);
        throw std::runtime_error("Failed to connect to host");
    }
}

TSWShared::RestClient::~RestClient()
{
    if (h_connect_)
        WinHttpCloseHandle(h_connect_);
    if (h_session_)
        WinHttpCloseHandle(h_session_);
}

std::string TSWShared::RestClient::perform_request(const std::wstring& endpoint,
                                                   const std::wstring& method,
                                                   const std::wstring& body) const
{
    HINTERNET hRequest = WinHttpOpenRequest(
        h_connect_, method.c_str(), endpoint.c_str(), nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, use_https_ ? WINHTTP_FLAG_SECURE : 0);

    if (!hRequest)
    {
        throw std::runtime_error("Failed to open HTTP request");
    }

    if (!WinHttpSendRequest(hRequest, L"Content-Type: application/json\r\n", 0,
                            (LPVOID)body.c_str(), body.size() * sizeof(wchar_t),
                            body.size() * sizeof(wchar_t), 0))
    {
        WinHttpCloseHandle(hRequest);
        throw std::runtime_error("Failed to send HTTP request");
    }

    if (!WinHttpReceiveResponse(hRequest, NULL))
    {
        WinHttpCloseHandle(hRequest);
        throw std::runtime_error("Failed to receive HTTP response");
    }

    DWORD size = 0;
    DWORD downloaded = 0;
    std::string response;

    do
    {
        if (!WinHttpQueryDataAvailable(hRequest, &size))
        {
            WinHttpCloseHandle(hRequest);
            throw std::runtime_error("Failed to query data");
        }

        if (size == 0)
            break;

        std::vector<char> buffer(size);
        if (!WinHttpReadData(hRequest, buffer.data(), size, &downloaded))
        {
            WinHttpCloseHandle(hRequest);
            throw std::runtime_error("Failed to read data");
        }

        response.append(buffer.data(), downloaded);
    }
    while (size > 0);

    WinHttpCloseHandle(hRequest);
    return response;
}

nlohmann::json TSWShared::RestClient::get(const std::wstring& endpoint)
{
    std::string response = perform_request(endpoint, L"GET");
    return nlohmann::json::parse(response);
}

nlohmann::json TSWShared::RestClient::post(const std::wstring& endpoint,
                                           const nlohmann::json& body)
{
    std::string body_str = body.dump();
    const std::wstring body_w_str(body_str.begin(), body_str.end());
    std::string response = perform_request(endpoint, L"POST", body_w_str);
    return nlohmann::json::parse(response);
}

std::wstring TSWShared::RestClient::convert_utf8_to_wide(const std::string& str)
{
    const int count = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), nullptr, 0);
    std::wstring wstr(count, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), str.length(), &wstr[0], count);
    return wstr;
}
