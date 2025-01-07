#include <Mod/CppUserModBase.hpp>
#include <DynamicOutput/DynamicOutput.hpp>
#include <chrono>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UEngine.hpp>

#include "restclient.hpp"

using namespace RC;
using namespace RC::Unreal;

class TSWWeatherMod : public RC::CppUserModBase
{
private:
    std::chrono::system_clock::time_point last_sync_time = std::chrono::system_clock::now();
    Unreal::UClass *locationLib;
    Unreal::UFunction *StaticFunction;
    Unreal::UEngine *GEngine;
    const float AccumulatedTime = 0.0f;
    const float IntervalInSeconds = 15.0f * 60.0f; // 15 minutes in seconds

public:
    TSWWeatherMod() : CppUserModBase()
    {
        ModName = STR("TSWWeatherMod");
        ModVersion = STR("1.0");
        ModDescription = STR("Real life weather sync for TSW5");
        ModAuthors = STR("Luex");
        // Do not change this unless you want to target a UE4SS version
        // other than the one you're currently building with somehow.
        // ModIntendedSDKVersion = STR("2.6");

        GEngine = (Unreal::UEngine *)Unreal::UObjectGlobals::StaticFindObject(Unreal::UObject::StaticClass(), nullptr, TEXT("/Script/Engine.Engine"));

        Output::send<LogLevel::Verbose>(STR("TSWWeatherMod says hello\n"));

        locationLib = Unreal::UObjectGlobals::FindObject<UClass>(nullptr, TEXT("/Script/TS2Prototype.TS2DebugFunctionLibrary"));
        StaticFunction = locationLib->GetFunctionByName(TEXT("GetActorLatLong"));

        struct
        {
            Unreal::AActor *Actor;
            float *OutLat;
            float *OutLong;
            bool *success;
        } Params;

        float lat, lon;
        bool success = false;

        Params.Actor = nullptr;
        Params.OutLat = &lat;
        Params.OutLong = &lon;
        Params.success = &success;

        locationLib->ProcessEvent(StaticFunction, &Params);

        // send lat and long and sucess to Output
        Output::send<LogLevel::Verbose>(STR("Lat: %f, Long: %f, Success: %d\n"), lat, lon, success);

        RestClient client(L"api.open-meteo.com", INTERNET_DEFAULT_HTTPS_PORT, true);

        // Example GET request
        nlohmann::json getResponse = client.Get(L"/v1/forecast?latitude=52.52&longitude=13.41&current=temperature_2m,relative_humidity_2m,apparent_temperature,precipitation,rain,showers,snowfall,weather_code,cloud_cover,pressure_msl,surface_pressure,wind_speed_10m,wind_direction_10m,wind_gusts_10m");
        getResponse["current"]["temperature_2m"];
        getResponse["current"]["relative_humidity_2m"];
        getResponse["current"]["rain"];
        getResponse["current"]["showers"];
        getResponse["current"]["snowfall"];
        getResponse["current"]["weather_code"];
        getResponse["current"]["cloud_cover"];
        getResponse["current"]["pressure_msl"];
        getResponse["current"]["surface_pressure"];
        getResponse["current"]["wind_speed_10m"];
        getResponse["current"]["wind_direction_10m"];
        getResponse["current"]["wind_gusts_10m"];
    }

    ~TSWWeatherMod() override
    {
    }

    auto on_update() -> void override
    {
        if (GEngine && GEngine->GameViewport)
        {
            UWorld *World = GEngine->GameViewport->GetWorld();
            if (World)
            {
                float DeltaTime = World->GetDeltaSeconds();
                AccumulatedTime += DeltaTime;

                // Check if enough time has passed
                if (AccumulatedTime >= IntervalInSeconds)
                {
                    // Reset accumulated time
                    AccumulatedTime -= IntervalInSeconds; // Allows for slight drift correction

                    // Run your 15-minute code here
                    UE_LOG(LogTemp, Log, TEXT("15 minutes have passed! Running scheduled code."));
                }
            }
        }
    }
};

#define TSW_WEATHER_MOD_API __declspec(dllexport)
extern "C"
{
    TSW_WEATHER_MOD_API RC::CppUserModBase *start_mod()
    {
        return new TSWWeatherMod();
    }

    TSW_WEATHER_MOD_API void uninstall_mod(RC::CppUserModBase *mod)
    {
        delete mod;
    }
}
