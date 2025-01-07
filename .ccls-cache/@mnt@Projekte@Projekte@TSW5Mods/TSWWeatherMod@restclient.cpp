#include "restclient.hpp"
#include <iostream>
#include <vector>

RestClient::RestClient(const std::wstring &host, INTERNET_PORT port, bool useHttps)
    : host(host), port(port), useHttps(useHttps)
{
    hSession = WinHttpOpen(L"RESTClient/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                           WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);

    if (!hSession)
    {
        throw std::runtime_error("Failed to open WinHTTP session");
    }

    hConnect = WinHttpConnect(hSession, host.c_str(), port, 0);
    if (!hConnect)
    {
        WinHttpCloseHandle(hSession);
        throw std::runtime_error("Failed to connect to host");
    }
}

RestClient::~RestClient()
{
    if (hConnect)
        WinHttpCloseHandle(hConnect);
    if (hSession)
        WinHttpCloseHandle(hSession);
}

std::wstring RestClient::PerformRequest(const std::wstring &endpoint, const std::wstring &method, const std::wstring &body)
{
    HINTERNET hRequest = WinHttpOpenRequest(hConnect, method.c_str(), endpoint.c_str(),
                                            NULL, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            useHttps ? WINHTTP_FLAG_SECURE : 0);

    if (!hRequest)
    {
        throw std::runtime_error("Failed to open HTTP request");
    }

    if (!WinHttpSendRequest(hRequest,
                            L"Content-Type: application/json\r\n",
                            0,
                            (LPVOID)body.c_str(),
                            body.size() * sizeof(wchar_t),
                            body.size() * sizeof(wchar_t),
                            0))
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
    std::wstring response;

    do
    {
        if (!WinHttpQueryDataAvailable(hRequest, &size))
        {
            WinHttpCloseHandle(hRequest);
            throw std::runtime_error("Failed to query data");
        }

        if (size == 0)
            break;

        std::vector<wchar_t> buffer(size + 1);
        if (!WinHttpReadData(hRequest, buffer.data(), size, &downloaded))
        {
            WinHttpCloseHandle(hRequest);
            throw std::runtime_error("Failed to read data");
        }

        response.append(buffer.data(), downloaded / sizeof(wchar_t));
    } while (size > 0);

    WinHttpCloseHandle(hRequest);
    return response;
}

nlohmann::json RestClient::Get(const std::wstring &endpoint)
{
    std::wstring response = PerformRequest(endpoint, L"GET");
    return nlohmann::json::parse(std::string(response.begin(), response.end()));
}

nlohmann::json RestClient::Post(const std::wstring &endpoint, const nlohmann::json &body)
{
    std::string bodyStr = body.dump();
    std::wstring bodyWStr(bodyStr.begin(), bodyStr.end());
    std::wstring response = PerformRequest(endpoint, L"POST", bodyWStr);
    return nlohmann::json::parse(std::string(response.begin(), response.end()));
}