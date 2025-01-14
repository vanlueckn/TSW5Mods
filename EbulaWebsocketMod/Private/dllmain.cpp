#include <uwebsockets/App.h>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Mod/CppUserModBase.hpp>
#include <thread>
#include <memory>

#include "tswhelper.hpp"
#include "json.hpp"

using namespace RC;
using namespace RC::Unreal;
using namespace std::chrono;

uWS::App* global_app;
bool global_unreal_init = false;

struct UpdateData
{
    TSWShared::lat_lon position;
    TSWShared::tsw_date_time date_time;
};

class EbulaWebsocketMod final : public CppUserModBase
{
    struct PerSocketData
    {
    };

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


    bool interpret_message(std::string_view message, uWS::WebSocket<false, true, PerSocketData>* ws) const
    {
        try
        {
            auto parsed = nlohmann::json::parse(message);
            if (parsed.contains("subscribe"))
            {
                if (const auto string_data = parsed["subscribe"].dump(); string_data.contains("pos"))
                {
                    ws->subscribe("pos");
                }
                else if (string_data.contains("traindata"))
                {
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
        }
        catch (...)
        {
            return false;
        }

        return false;
    }

    auto ws_start()
    {
        auto app = uWS::App().ws<PerSocketData>("/api", {
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
                                                    .close = [](
                                                    auto* ws, int /*code*/, std::string_view /*message*/)
                                                    {
                                                    }
                                                }).listen(9187, [](auto* listen_socket)
        {
            if (listen_socket)
            {
                Output::send<LogLevel::Verbose>(
                    STR("Thread listening on port {}\n"), 9187);
            }
            else
            {
                Output::send<LogLevel::Error>(
                    STR("Thread failed to listen on port {}\n"), 9187);
            }
        });

        const auto loop = reinterpret_cast<struct us_loop_t*>(uWS::Loop::get());
        us_timer_t* delay_timer = us_create_timer(loop, 0, 0);

        us_timer_set(delay_timer, [](struct us_timer_t*/*t*/)
        {
            if (!global_unreal_init) return;
            UpdateData data;
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
            auto json_dumped = json_data.dump();

            global_app->publish("pos", std::string_view(json_dumped.c_str(), json_dumped.size()), uWS::OpCode::TEXT,
                                false);
        }, 10000, 10000);

        global_app = &app;
        global_app->run();
    }

public:
    EbulaWebsocketMod() : CppUserModBase()
    {
        ModName = STR("EbulaWebsocketMod");
        ModVersion = STR("1.0");
        ModDescription = STR("Ebula Websocket Mod");
        ModAuthors = STR("Luex");
        // Do not change this unless you want to target a UE4SS version
        // other than the one you're currently building with somehow.
        // ModIntendedSDKVersion = STR("2.6");

        Output::send<LogLevel::Verbose>(STR("EbulaWebsocketMod Code running\n"));
    }

    ~EbulaWebsocketMod() override
    {
    }

    auto on_update() -> void override
    {
        if (!global_unreal_init) return;
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
