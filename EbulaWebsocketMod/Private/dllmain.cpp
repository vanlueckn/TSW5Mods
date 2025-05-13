#include <uwebsockets/App.h>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Mod/CppUserModBase.hpp>
#include <thread>
#include <memory>
#include <unordered_set>

#include "tswhelper.hpp"
#include "json.hpp"
#include "mainthreadqueue.hpp"

using namespace RC;
using namespace RC::Unreal;
using namespace std::chrono;

struct timed_function
{
    int interval_ms;
    int time_remaining;
};

struct per_socket_data
{
    timed_function interval_pos;
    timed_function interval_traindata;
};

struct update_data
{
    TSWShared::lat_lon position;
    TSWShared::tsw_date_time date_time;
};

uWS::App* global_app;
bool global_unreal_init = false;
std::unordered_set<uWS::WebSocket<false, true, per_socket_data>*> global_clients;

void initialize_pos_timer(int interval);
void disarm_pos_timer();

class EbulaWebsocketMod final : public CppUserModBase
{
    std::unique_ptr<std::thread> thread_1_ = nullptr;
    nlohmann::json join_message_json_ = {
        {"event", "join_message"},
    };
    std::string join_message_string_ = join_message_json_.dump();
    std::string_view join_message_ = std::string_view(join_message_string_.c_str(), join_message_string_.size());

    nlohmann::json subscribe_message_json_ = {
        {"event", "subscribe_message"},
    };
    std::string subscribe_message_string_ = subscribe_message_json_.dump();
    std::string_view subscribe_message_ = std::string_view(subscribe_message_string_.c_str(),
                                                           subscribe_message_string_.size());

    nlohmann::json unsubscribe_message_json_ = {
        {"event", "unsubscribe_message"},
    };
    std::string unsubscribe_message_string_ = unsubscribe_message_json_.dump();
    std::string_view unsubscribe_message_ = std::string_view(unsubscribe_message_string_.c_str(),
                                                             unsubscribe_message_string_.size());

    nlohmann::json execute_message_json_ = {
        {"event", "command_executed"},
    };
    std::string execute_message_json_string_ = execute_message_json_.dump();
    std::string_view execute_message_ = std::string_view(execute_message_json_string_.c_str(),
                                                         execute_message_json_string_.size());

    static std::string get_current_pos_json()
    {
        update_data data;
        data.position = TSWShared::TSWHelper::get_instance()->get_current_position_in_game(true);
        const auto actor = TSWShared::TSWHelper::get_instance()->get_camera_actor();
        data.date_time = TSWShared::TSWHelper::get_instance()->get_world_date_time(actor);

        Output::send<LogLevel::Verbose>(std::format(L"New Data: lat {}, long {}, time {}", data.position.lat,
                                                    data.position.lon,
                                                    data.date_time.to_iso8601()));
        auto iso_date = data.date_time.to_iso8601();
        nlohmann::json json_data = {
            {"event", "pos"},
            {"lat", data.position.lat},
            {"long", data.position.lon},
            {"speed_kph", data.position.kph},
            {"time_iso_8601", std::string(iso_date.begin(), iso_date.end())}
        };
        return json_data.dump();
    }

    static bool is_valid_command(const std::string& command)
    {
        if (command.size() > 1023) return false; // max length of ue Fstring

        // Example: Commands must be alphanumeric and can include underscores
        //const std::regex commandRegex("^[a-zA-Z0-9_]+$");
        //return std::regex_match(command, commandRegex);
        return true;
    }

    static void remove_quotes(std::string& str)
    {
        // Check if the string starts with a quote and ends with a quote
        if (str.front() == '"' && str.back() == '"')
        {
            // Remove the quotes by erasing the first and last characters
            str = str.substr(1, str.length() - 2);
        }
    }

    bool interpret_message(std::string_view message, uWS::WebSocket<false, true, per_socket_data>* ws) const
    {
        try
        {
            auto parsed = nlohmann::json::parse(message);
            if (parsed.contains("subscribe"))
            {
                auto interval = 10000;
                if (parsed.contains("interval"))
                {
                    interval = parsed["interval"].get<int>();
                }
                if (const auto string_data = parsed["subscribe"].dump(); string_data.contains("pos"))
                {
                    ws->getUserData()->interval_pos.interval_ms = interval;
                    ws->getUserData()->interval_pos.time_remaining = 0;
                    ws->subscribe("pos");
                }
                else if (string_data.contains("traindata"))
                {
                    ws->getUserData()->interval_traindata.interval_ms = interval;
                    ws->getUserData()->interval_traindata.time_remaining = 0;
                    ws->subscribe("traindata");
                }
                else
                {
                    return false;
                }

                ws->send(subscribe_message_, uWS::OpCode::TEXT);
                return true;
            }
            if (parsed.contains("unsubscribe"))
            {
                if (const auto string_data = parsed["unsubscribe"].dump(); string_data.contains("pos"))
                {
                    ws->unsubscribe("pos");
                }
                else if (string_data.contains("traindata"))
                {
                    ws->unsubscribe("traindata");
                }
                else
                {
                    return false;
                }
                ws->send(unsubscribe_message_, uWS::OpCode::TEXT);
                return true;
            }
            if (parsed.contains("execute"))
            {
                auto string_data = parsed["execute"].dump();
                remove_quotes(string_data);
                if (!is_valid_command(string_data)) return false;

                MainThreadQueue::get_instance().add_function([string_data]
                {
                    const auto command = std::wstring(string_data.begin(), string_data.end());
                    const auto player_controller = TSWShared::TSWHelper::get_instance()->get_player_controller();
                    const auto kismet_lib = TSWShared::TSWHelper::get_instance()->get_kismet_library_cached();
                    if (!kismet_lib) return;

                    const auto kismet_static_function = TSWShared::TSWHelper::get_function_by_name(
                        STR("ExecuteConsoleCommand"), kismet_lib);
                    if (!kismet_static_function)
                    {
                        Output::send<LogLevel::Error>(STR("Static kismet function not found\n"));
                    }
                    TSWShared::TSWHelper::execute_console_command(FString(command.c_str()), player_controller,
                                                                  kismet_static_function, kismet_lib);
                });

                ws->send(execute_message_, uWS::OpCode::TEXT);
                return true;
            }
        }
        catch (...)
        {
            return false;
        }

        return false;
    }

    auto ws_start()
    {
        auto app = uWS::App().ws<per_socket_data>("/api", {
                                                      /* Settings */
                                                      .compression = uWS::SHARED_COMPRESSOR,
                                                      .maxPayloadLength = 16 * 1024,
                                                      .idleTimeout = 10,
                                                      .maxBackpressure = 1 * 1024 * 1024,
                                                      /* Handlers */
                                                      .upgrade = nullptr,
                                                      .open = [&](auto* ws)
                                                      {
                                                          ws->send(join_message_, uWS::OpCode::TEXT);
                                                          global_clients.insert(ws);
                                                      },
                                                      .message = [&](
                                                      auto* ws, std::string_view message, uWS::OpCode opCode)
                                                      {
                                                          auto success = interpret_message(message, ws);
                                                          if (!success)
                                                          {
                                                              Output::send<LogLevel::Error>(
                                                                  STR("Incoming json message invalid\nMessage: {}\n"),
                                                                  std::wstring(message.begin(), message.end()));
                                                          }
                                                      },
                                                      .drain = [](auto*/*ws*/)
                                                      {
                                                          /* Check getBufferedAmount here */
                                                      },
                                                      .ping = [](auto*/*ws*/, std::string_view)
                                                      {
                                                      },
                                                      .pong = [](auto*/*ws*/, std::string_view)
                                                      {
                                                      },
                                                      .close = [&](
                                                      auto* ws, int /*code*/, std::string_view /*message*/)
                                                      {
                                                          global_clients.erase(ws);
                                                      }
                                                  }).listen(9187, [](auto* listen_socket)
        {
            if (listen_socket)
            {
                Output::send<LogLevel::Verbose>(
                    STR("Server listening on port {}\n"), 9187);
            }
            else
            {
                Output::send<LogLevel::Error>(
                    STR("Server failed to listen on port {}\n"), 9187);
            }
        });

        const auto loop = reinterpret_cast<struct us_loop_t*>(uWS::Loop::get());
        const auto ws_update_timer = us_create_timer(loop, 0, 0);
        us_timer_set(ws_update_timer, [](struct us_timer_t*/*t*/)
        {
            if (!global_unreal_init) return;

            bool pos_already_calculated = false;
            std::string_view cached_pos_json;

            for (auto* ws : global_clients)
            {
                const auto ws_user_data = ws->getUserData();
                if (ws->isSubscribed("pos"))
                {
                    if (!pos_already_calculated)
                    {
                        const auto json_val = get_current_pos_json();
                        cached_pos_json = std::string_view(json_val.c_str(), json_val.size());
                        pos_already_calculated = true;
                    }

                    ws_user_data->interval_pos.time_remaining -= 1000;
                    if (ws_user_data->interval_pos.time_remaining <= 0)
                    {
                        ws->send(cached_pos_json, uWS::OpCode::TEXT, false);
                        ws_user_data->interval_pos.time_remaining += ws_user_data->interval_pos.interval_ms;
                    }
                }
            }
        }, 1000, 1000);

        global_app = &app;
        global_app->run();
    }

public:
    EbulaWebsocketMod() : CppUserModBase()
    {
        ModName = STR("WebsocketMod");
        ModVersion = STR("0.3");
        ModDescription = STR("This mod exposes a Websocket Server");
        ModAuthors = STR("Luex & ClutchFred");
        // Do not change this unless you want to target a UE4SS version
        // other than the one you're currently building with somehow.
        // ModIntendedSDKVersion = STR("2.6");

        Output::send<LogLevel::Verbose>(STR("WebsocketMod Code running\n"));
    }

    ~EbulaWebsocketMod() override
    {
    }

    auto on_update() -> void override
    {
        if (!global_unreal_init) return;
        MainThreadQueue::get_instance().execute_next();
    }

    auto on_unreal_init() -> void override
    {
        global_unreal_init = true;
        thread_1_ = std::make_unique<std::thread>(&EbulaWebsocketMod::ws_start, this);
    }
};

#define EBULA_WEBSOCKET_API __declspec(dllexport)

extern "C" {
EBULA_WEBSOCKET_API CppUserModBase* start_mod()
{
    return new EbulaWebsocketMod();
}

EBULA_WEBSOCKET_API void uninstall_mod(CppUserModBase* mod) { delete mod; }
}
