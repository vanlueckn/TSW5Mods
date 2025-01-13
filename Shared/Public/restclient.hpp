#ifndef RESTCLIENT_H
#define RESTCLIENT_H
#define WIN32_LEAN_AND_MEAN

#include <string>
#include <windows.h>
#include <winhttp.h>
#include "json.hpp"

#pragma comment(lib, "winhttp.lib")

using json = nlohmann::json;

namespace TSWShared
{
    class RestClient
    {
    public:
        RestClient(const std::wstring& host, INTERNET_PORT port,
                   bool use_https = true);
        ~RestClient();

        json get(const std::wstring& endpoint);
        json post(const std::wstring& endpoint, const json& body);

    private:
        HINTERNET h_session_;
        HINTERNET h_connect_;
        bool use_https_;
        std::wstring host_;
        INTERNET_PORT port_;

        std::string perform_request(const std::wstring& endpoint,
                                    const std::wstring& method,
                                    const std::wstring& body = L"") const;
        static std::wstring convert_utf8_to_wide(const std::string& str);
    };
}


#endif // RESTCLIENT_H
