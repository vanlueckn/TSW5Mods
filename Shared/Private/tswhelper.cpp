#include <numbers>
#include <DynamicOutput/DynamicOutput.hpp>
#include <Mod/CppUserModBase.hpp>
#include <Unreal/FText.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <restclient.hpp>

#include "tswhelper.hpp"

using namespace RC;
using namespace RC::Unreal;

struct lat_lon;

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
    if (location_library_cached_)
    {
        return location_library_cached_;
    }

    location_library_cached_ = get_kismet_library();
    return location_library_cached_;
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

UObject* TSWShared::TSWHelper::get_location_library_cached()
{
    if (location_library_cached_)
    {
        return location_library_cached_;
    }

    location_library_cached_ = get_location_library();
    return location_library_cached_;
}

UObject* TSWShared::TSWHelper::get_location_library()
{
    const auto location_library = Unreal::UObjectGlobals::FindObject<UObject>(
        nullptr, TEXT("/Script/Engine.Default__KismetSystemLibrary"));

    if (!location_library)
    {
        Output::send<LogLevel::Error>(STR("Location library not found\n"));
        return nullptr;
    }

    return location_library;
}

Unreal::UObject* TSWShared::TSWHelper::get_function_library_cached()
{
    if (function_library_cached_)
    {
        return function_library_cached_;
    }

    function_library_cached_ = get_function_library();
    return function_library_cached_;
}

Unreal::UObject* TSWShared::TSWHelper::get_function_library()
{
    const auto function_library = UObjectGlobals::FindObject<UObject>(
        nullptr, TEXT("/Script/Engine.Default__TS2GameplayFunctionLibrary"));

    if (!function_library)
    {
        Output::send<LogLevel::Error>(STR("Function library not found\n"));
        return nullptr;
    }

    return function_library;
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

TSWShared::lat_lon TSWShared::TSWHelper::get_current_position_in_game()
{
    lat_lon latlon;
    latlon.lat = 0.0f;
    latlon.lon = 0.0f;

    const auto location_lib = get_location_library_cached();
    if (!cached_get_lat_long_)
    {
        cached_get_lat_long_ = get_function_by_name(STR("GetActorLatLong"), location_lib);
        if (!cached_get_lat_long_)
        {
            Output::send<LogLevel::Error>(STR("GetActorLatLong function not found\n"));
            return latlon;
        }
    }

    struct
    {
        AActor* actor;
        float out_lat;
        float out_long;
        bool success;
    } params{};

    params.actor = get_camera_actor();

    location_lib->ProcessEvent(cached_get_lat_long_, &params);

    Output::send<LogLevel::Verbose>(STR("Lat: {}, Long: {}, Success: {}\n"),
                                    params.out_lat, params.out_long, params.success);

    latlon.lat = params.out_lat;
    latlon.lon = params.out_long;

    return latlon;
}

TSWShared::tsw_date_time TSWShared::TSWHelper::get_world_date_time(UObject* world_context_object)
{
    const auto function_lib = get_function_library_cached();

    if (cache_functions(cached_get_world_date_time_, STR("GetDateTime"), function_lib))
    {
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
    params.local_time = false; // because we want the world time in UTC
    
    function_lib->ProcessEvent(cached_get_world_date_time_, &params);

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

    auto player_controller_casted = reinterpret_cast<AActor*>(player_controller);

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

    Output::send<LogLevel::Verbose>(STR("Executing command: {}\n"), string_data);

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

bool TSWShared::TSWHelper::cache_functions(const UFunction* func, const wchar_t* func_name, UObject* func_lib)
{
    if (!func)
    {
        func = get_function_by_name(func_name, func_lib);
        if (!func)
        {
            Output::send<LogLevel::Error>(STR("GetWorldDateTime function not found\n"));
            return false;
        }
    }

    return true;
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

    l = FMath::FloorToInt(static_cast<float>(GetJulianDay(ue_date_time) + 0.5)) + 68569;
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

    int hour = static_cast<int32>(ticks / ticks_per_hour) % 24;
    int minute = static_cast<int32>(ticks / ticks_per_minute) % 60;
    int second = static_cast<int32>(ticks / ticks_per_second) % 60;

    tsw_date_time.hour = hour;
    tsw_date_time.minute = minute;
    tsw_date_time.second = second;

    return tsw_date_time;
}
