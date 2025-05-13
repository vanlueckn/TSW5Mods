#include <numbers>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Mod/CppUserModBase.hpp>
#include <Unreal/FText.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <restclient.hpp>
#include <chrono>
#include <iostream>
#include <ctime>
#include <tuple>
#include <thread>
#include "tswhelper.hpp"

#include "ExceptionHandling.hpp"

using namespace RC;
using namespace RC::Unreal;

struct lat_lon;
typedef std::chrono::high_resolution_clock Clock;

TSWShared::TSWHelper* TSWShared::TSWHelper::singleton_ = nullptr;

/**
 * Static methods should be defined outside the class.
 */
TSWShared::TSWHelper* TSWShared::TSWHelper::get_instance()
{
    /**
     * This is a safer way to create an instance. instance = new Singleton is
     * dangeruous in case two instance threads wants to access at the same time
     */
    if (singleton_ == nullptr)
    {
        singleton_ = new TSWHelper();
    }
    return singleton_;
}

UObject* TSWShared::TSWHelper::get_kismet_library_cached()
{
    if (kismet_library_cached_)
    {
        return kismet_library_cached_;
    }

    kismet_library_cached_ = get_kismet_library();
    return kismet_library_cached_;
}

UObject* TSWShared::TSWHelper::get_kismet_library()
{
    const auto kismet_library = UObjectGlobals::FindObject<UObject>(
        nullptr, TEXT("/Script/Engine.Default__KismetSystemLibrary"));

    if (!kismet_library)
    {
        Output::send<LogLevel::Error>(STR("Kismet library not found\n"));
        return nullptr;
    }

    return kismet_library;
}

float TSWShared::TSWHelper::get_actor_velocity_in_kph(AActor* actor)
{
    if (!actor) return 0.0f;

    const auto velocity_function = get_function_by_name_in_chain(STR("GetVelocity"), actor->GetClassPrivate());
    if (!velocity_function)
    {
        Output::send<LogLevel::Error>(STR("GetVelocity function not found\n"));
        return 0.0f;
    }

    struct params
    {
        FVector velocity;
    } params{};

    actor->ProcessEvent(velocity_function, &params);
    auto velo = params.velocity;
    const float speed_cm_per_sec = FMath::Sqrt(velo.X() * velo.X() + velo.Y() * velo.Y() + velo.Z() * velo.Z());
    return speed_cm_per_sec * 0.036f;
}

UFunction* TSWShared::TSWHelper::get_function_by_name(const StringType& name, UObject* input_object)
{
    for (UFunction* Function : input_object->GetClassPrivate()->ForEachFunction())
    {
        if (Function->GetNamePrivate().ToString() == name)
        {
            return Function;
        }
    }

    return nullptr;
}

UFunction* TSWShared::TSWHelper::get_function_by_name(const StringType& name, UClass* input_class)
{
    for (UFunction* function : input_class->ForEachFunction())
    {
        if (function->GetNamePrivate().ToString() == name)
        {
            return function;
        }
    }
    input_class->GetClassPrivate()->ForEachFunction();

    return nullptr;
}

TSWShared::lat_lon TSWShared::TSWHelper::get_current_position_in_game(const bool with_speed)
{
    const auto start_time_complete = Clock::now();
    lat_lon latlon;
    latlon.lat = 0.0f;
    latlon.lon = 0.0f;
    latlon.kph = 0.0f;

    const auto location_lib = Unreal::UObjectGlobals::FindObject<UClass>(
        nullptr, TEXT("/Script/TS2Prototype.TS2DebugFunctionLibrary"));

    if (!location_lib)
    {
        Output::send<LogLevel::Error>(STR("Location library not found\n"));
        return latlon;
    }

    const auto static_function = get_function_by_name(STR("GetActorLatLong"), location_lib);
    if (!static_function)
    {
        Output::send<LogLevel::Error>(STR("Static function not found\n"));
        return latlon;
    }

    struct
    {
        AActor* actor;
        float out_lat;
        float out_long;
        bool success;
    } params{};

    const auto pawn = get_player_pawn();
    if (!pawn)
    {
        Output::send<LogLevel::Error>(STR("Pawn not found\n"));
        return latlon;
    }

    params.actor = static_cast<AActor*>(pawn);
    location_lib->ProcessEvent(static_function, &params);
    latlon.lat = params.out_lat;
    latlon.lon = params.out_long;

    if (with_speed)
    {
        latlon.kph = get_actor_velocity_in_kph(static_cast<AActor*>(pawn));
    }

    auto const end_time_complete = Clock::now();
    Output::send<LogLevel::Verbose>(std::format(L"Zeit complete: {}ns",
                                                std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                    end_time_complete - start_time_complete).count()));

    return latlon;
}

static auto set_date_callback(const UnrealScriptFunctionCallableContext& context, void* custom_data) -> void
{
    Output::send<LogLevel::Error>(STR("time was updated\n"));
}

void TSWShared::TSWHelper::inject_set_date()
{
    return;
    Output::send<LogLevel::Verbose>(STR("Injecting set date\n"));
    const auto tod_function = UObjectGlobals::StaticFindObject<UFunction*>(
        nullptr, nullptr, STR("/Script/Engine.PlayerController:ClientRestart"));

    if (!tod_function)
    {
        Output::send<LogLevel::Error>(STR("TS2Prototype.TS2GameplayFunctionLibrary not found\n"));
        return;
    }

    UObjectGlobals::RegisterHook(tod_function, [](...)
    {
    }, &set_date_callback, nullptr);
}

TSWShared::tsw_date_time TSWShared::TSWHelper::get_world_date_time(UObject* world_context_object)
{
    const auto start_time = Clock::now();
    const auto function_lib = Unreal::UObjectGlobals::FindObject<UClass>(
        nullptr, TEXT("/Script/TS2Prototype.TS2GameplayFunctionLibrary"));

    const auto static_function = get_function_by_name(STR("GetDateTime"), function_lib);

    if (!static_function)
    {
        Output::send<LogLevel::Error>(STR("GetDateTime: Function not found\n"));
        return {};
    }

    struct
    {
        // uworld, bool, pointer to struct
        UObject* world_context_obj;
        bool local_time;
        ue_datetime out_date_time;
    } params{};

    params.world_context_obj = world_context_object;
    params.local_time = false;

    function_lib->ProcessEvent(static_function, &params);
    auto const end_time = Clock::now();
    Output::send<LogLevel::Verbose>(std::format(L"Zeit Time: {}ns",
                                                std::chrono::duration_cast<std::chrono::nanoseconds>(
                                                    end_time - start_time).count()));
    return ue_to_tsw_date_time(params.out_date_time);
}

AActor* TSWShared::TSWHelper::get_camera_actor()
{
    const auto camera_actor_class = STR("TS2DefaultCameraManager_C");

    if (const auto camera_actor = UObjectGlobals::FindFirstOf(camera_actor_class))
    {
        return reinterpret_cast<AActor*>(camera_actor);
    }

    Output::send<LogLevel::Error>(STR("Camera Actor not found\n"));
    return nullptr;
}

UFunction* TSWShared::TSWHelper::get_function_by_name_in_chain(const StringType& name, UObject* input_object)
{
    for (UFunction* function : input_object->GetClassPrivate()->ForEachFunctionInChain())
    {
        if (function->GetNamePrivate().ToString() == name)
        {
            return function;
        }
    }

    return nullptr;
}

UFunction* TSWShared::TSWHelper::get_function_by_name_in_chain(const StringType& name, UClass* input_object)
{
    for (UFunction* function : input_object->ForEachFunctionInChain())
    {
        if (function->GetNamePrivate().ToString() == name)
        {
            return function;
        }
    }

    return nullptr;
}

UObject* TSWShared::TSWHelper::get_player_controller()
{
    if (player_controller_cached_ && is_valid_u_object(player_controller_cached_))
    {
        return player_controller_cached_;
    }

    std::vector<UObject*> controllers;
    UObjectGlobals::FindAllOf(STR("PlayerController"), controllers);
    Output::send<LogLevel::Verbose>(std::format(L"Count player controllers: {}", controllers.size()));

    for (const auto controller : controllers)
    {
        if (controller && is_valid_u_object(controller))
        {
            player_controller_cached_ = controller;
        }
    }

    return player_controller_cached_;
}

UObject* TSWShared::TSWHelper::get_player_pawn()
{
    const auto player_controller = get_player_controller();
    if (!player_controller || !is_valid_u_object(player_controller))
    {
        Output::send<LogLevel::Error>(STR("Player Controller not found or invalid\n"));
        return nullptr;
    }

    const auto get_pawn_function = get_function_by_name_in_chain(STR("K2_GetPawn"), player_controller);

    if (!get_pawn_function || !is_valid_u_object(get_pawn_function))
    {
        Output::send<LogLevel::Error>(STR("K2_GetPawn: Function not found or invalid\n"));
        player_controller_cached_ = nullptr;
        return nullptr;
    }

    struct params
    {
        AActor* pawn;
    } params{};

    player_controller->ProcessEvent(get_pawn_function, &params);
    if (!params.pawn)
    {
        Output::send<LogLevel::Error>(STR("No pawn found\n"));
        return nullptr;
    }
    return params.pawn;
}

void TSWShared::TSWHelper::execute_console_command(FString command, UObject* player_controller,
                                                   UFunction* kismet_static_function,
                                                   UObject* kismet_lib)
{
    struct
    {
        UObject* world_context_obj;
        FString command;
        class APlayerController* player;
    } params{};

    params.player = reinterpret_cast<APlayerController*>(player_controller);
    params.command = command;
    params.world_context_obj = player_controller;

    if (!is_valid_u_object(player_controller))
    {
        Output::send<LogLevel::Verbose>(STR("Player controller is invalid\n"));
    }

    if (!is_valid_u_object(kismet_static_function))
    {
        Output::send<LogLevel::Verbose>(STR("kismetStaticFunction is invalid\n"));
    }

    const auto string_data = command.GetCharTArray().GetData();

    Output::send<LogLevel::Verbose>(STR("Executing command: <{}>\n"), string_data);

    kismet_lib->ProcessEvent(kismet_static_function, &params);
}

bool TSWShared::TSWHelper::is_valid_u_object(UObject* object)
{
    return object && UObjectArray::IsValid(object->GetObjectItem(), false);
}

float TSWShared::TSWHelper::calculate_distance_in_miles_lat_lon(const lat_lon p1, const lat_lon p2)
{
    constexpr float R = 6371e3;
    const float phi1 = p1.lat * std::numbers::pi / 180;
    const float phi2 = p2.lat * std::numbers::pi / 180;
    const float delta_phi = (p2.lat - p1.lat) * std::numbers::pi / 180;
    const float delta_lambda = (p2.lon - p1.lon) * std::numbers::pi / 180;
    const float a = sin(delta_phi / 2) * sin(delta_phi / 2) +
        cos(phi1) * cos(phi2) *
        sin(delta_lambda / 2) * sin(delta_lambda / 2);
    const float c = 2 * atan2(sqrt(a), sqrt(1 - a));
    return R * c * 0.000621371;
}

std::tuple<int, int, int> TSWShared::TSWHelper::get_current_utc_date()
{
    const std::time_t now = std::time(nullptr);
    std::tm utc_time;

    // Convert to UTC time structure using gmtime_s (thread-safe)
    if (gmtime_s(&utc_time, &now) != 0)
    {
        throw std::runtime_error("Failed to get UTC time");
    }

    // Extract day, month, and year
    int day = utc_time.tm_mday;
    int month = utc_time.tm_mon + 1; // tm_mon is 0-based
    int year = utc_time.tm_year + 1900; // tm_year is years since 1900

    return std::make_tuple(year, month, day);
}

void TSWShared::TSWHelper::listen_for_console_command(std::function<void()> callback)
{
    Hook::RegisterProcessConsoleExecGlobalPostCallback(
        [callback](UObject* context, const TCHAR* cmd, FOutputDevice& ar, UObject* executor) -> std::pair<bool, bool>
        {
            (void)context;
            (void)executor;

            std::pair<bool, bool> return_value{};
            return_value.first = false;

            return TRY([&]
            {
                const auto command = File::StringType{ToCharTypePtr(cmd)};
                const auto command_parts = explode_by_occurrence_with_quotes(command, STR(' '));
                File::StringType command_name = command;
                if (command_parts.size() > 1)
                {
                    command_name = command_parts[0];
                }

                if (command_name.contains(L"SetLocalDateTime") || command_name.contains(L"SetWorldDateTime"))
                {
                    callback();
                }

                return return_value;
            });
        });
}

TSWShared::tsw_date_time TSWShared::TSWHelper::ue_to_tsw_date_time(const ue_datetime& ue_date_time)
{
    // Hols the ticks in 100 nanoseconds resolution since 1/1/0001 A.D.
    auto const ticks = ue_date_time.ticks;

    tsw_date_time tsw_date_time;

    // Based on FORTRAN code in:
    // Fliegel, H. F. and van Flandern, T. C.,
    // Communications of the ACM, Vol. 11, No. 10 (October 1968).
    int32 i, j, k, l, n;

    l = FMath::FloorToInt(static_cast<float>(get_julian_day(ue_date_time) + 0.5)) + 68569;
    n = 4 * l / 146097;
    l = l - (146097 * n + 3) / 4;
    i = 4000 * (l + 1) / 1461001;
    l = l - 1461 * i / 4 + 31;
    j = 80 * l / 2447;
    k = l - 2447 * j / 80;
    l = j / 11;
    j = j + 2 - 12 * l;
    i = 100 * (n - 49) + i + l;

    tsw_date_time.ticks = ticks;

    tsw_date_time.year = i;
    tsw_date_time.month = j;
    tsw_date_time.day = k;

    const int hour = static_cast<int32>((ticks / ticks_per_hour) % 24);
    const int minute = static_cast<int32>((ticks / ticks_per_minute) % 60);
    const int second = static_cast<int32>((ticks / ticks_per_second) % 60);

    tsw_date_time.hour = hour;
    tsw_date_time.minute = minute;
    tsw_date_time.second = second;

    return tsw_date_time;
}
