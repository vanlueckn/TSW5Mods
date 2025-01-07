#ifndef RESTCLIENT_H
#define RESTCLIENT_H

#include <string>
#include <windows.h>
#include <winhttp.h>
#include "json.hpp"

#pragma comment(lib, "winhttp.lib")

class RestClient {
public:
    RestClient(const std::wstring& host, INTERNET_PORT port, bool useHttps = true);
    ~RestClient();

    nlohmann::json Get(const std::wstring& endpoint);
    nlohmann::json Post(const std::wstring& endpoint, const nlohmann::json& body);

private:
    HINTERNET hSession;
    HINTERNET hConnect;
    bool useHttps;
    std::wstring host;
    INTERNET_PORT port;

    std::wstring PerformRequest(const std::wstring& endpoint, const std::wstring& method, const std::wstring& body = L"");
};

#endif // RESTCLIENT_H