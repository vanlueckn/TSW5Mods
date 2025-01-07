#ifndef RESTCLIENT_H
#define RESTCLIENT_H
#define WIN32_LEAN_AND_MEAN

#include <string>
#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <vector>
#include <deps/nlohmann/include/nlohmann/json.hpp>
#include <DynamicOutput/DynamicOutput.hpp>

#pragma comment(lib, "winhttp.lib")

using json = nlohmann::json;
using namespace RC;

class RestClient {
public:
  RestClient(const std::wstring &host, INTERNET_PORT port,
             bool useHttps = true);
  ~RestClient();

  json Get(const std::wstring &endpoint);
  json Post(const std::wstring &endpoint, const json &body);

private:
  HINTERNET hSession;
  HINTERNET hConnect;
  bool useHttps;
  std::wstring host;
  INTERNET_PORT port;

  std::string PerformRequest(const std::wstring &endpoint,
                              const std::wstring &method,
                              const std::wstring &body = L"");
  std::wstring ConvertUtf8ToWide(const std::string& str);
};

#endif // RESTCLIENT_H