#include <uwebsockets/App.h>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Mod/CppUserModBase.hpp>
#include <algorithm>
#include <thread>
#include <mutex>
#include <functional>
#include <condition_variable>
#include <queue>
#include <future>
#include <set>
#include <chrono>

#include "tswhelper.hpp"

using namespace RC;
using namespace RC::Unreal;
using namespace std::chrono;

class EbulaWebsocketMod final : public CppUserModBase
{
    struct PerSocketData
    {
    };

    std::set<uWS::WebSocket<false, false, PerSocketData>> websockets_;
    std::mutex vector_mutex;
    std::condition_variable vector_cv;
    steady_clock::time_point last_run_minute_ = steady_clock::now();
    minutes interval_minute_ = minutes(1);


    bool initialized_ = false;
    std::queue<std::function<void()>> queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    bool stop_processing_ = false;

    template <typename Func>
    auto enqueue_task(Func task) -> std::future<decltype(task())>
    {
        using return_type = decltype(task());
        auto promise = std::make_shared<std::promise<return_type>>();
        auto future = promise->get_future();

        {
            std::lock_guard lock(queue_mutex_);
            queue_.push([promise, task]()
            {
                try
                {
                    promise->set_value(task());
                }
                catch (...)
                {
                    promise->set_exception(std::current_exception());
                }
            });
        }
        queue_cv_.notify_one();
        return future;
    }

    auto process_tasks()
    {
        while (!stop_processing_)
        {
            std::unique_lock lock(queue_mutex_);
            queue_cv_.wait(lock, [&] { return !queue_.empty() || stop_processing_; });

            while (!queue_.empty())
            {
                auto task = std::move(queue_.front());
                queue_.pop();
                lock.unlock();
                task();
                lock.lock();
            }
        }
    }

    auto init_websocket_server()
    {
        /* ws->getUserData returns one of these */

        std::vector<std::thread*> threads(std::max(static_cast<int>(std::thread::hardware_concurrency()), 1));

        std::ranges::transform(threads, threads.begin(), [](std::thread*)
        {
            return new std::thread([]()
            {
                uWS::App().ws<PerSocketData>("/pos", {
                                                 /* Settings */
                                                 .compression = uWS::SHARED_COMPRESSOR,
                                                 .maxPayloadLength = 16 * 1024,
                                                 .idleTimeout = 10,
                                                 .maxBackpressure = 1 * 1024 * 1024,
                                                 /* Handlers */
                                                 .upgrade = nullptr,
                                                 .open = [this](auto* ws)
                                                 {
                                                     {
                                                         std::lock_guard lock(vector_mutex);
                                                         websockets_.insert(ws);
                                                     }
                                                 },
                                                 .message = [](auto* ws, std::string_view message, uWS::OpCode opCode)
                                                 {
                                                     ws->send(message, opCode);
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
                                                 .close = [this](auto* ws, int /*code*/, std::string_view /*message*/)
                                                 {
                                                     {
                                                         std::lock_guard lock(vector_mutex);
                                                         websockets_.erase(ws);
                                                     }
                                                 }
                                             }).listen(9187, [](auto* listen_socket)
                {
                    if (listen_socket)
                    {
                        Output::send<LogLevel::Verbose>(
                            STR("Thread {} listening on port {}\n"), std::this_thread::get_id(), 9187);
                    }
                    else
                    {
                        Output::send<LogLevel::Error>(
                            STR("Thread {} faield to listen on port {}\n"), std::this_thread::get_id(), 9187);
                    }
                }).run();
            });
        });

        std::ranges::for_each(threads, [](std::thread* t)
        {
            t->join();
        });
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
        process_tasks();
        if (!initialized_) return;
        const auto now = steady_clock::now();
        if (now - last_run_minute_ >= interval_minute_)
        {
            const auto latlon = TSWShared::TSWHelper::get_instance()->get_current_position_in_game();
            
            last_run_minute_ = now;
        }
    }

    auto on_unreal_init() -> void override
    {
        initialized_ = true;
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
