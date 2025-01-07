#include <DynamicOutput/DynamicOutput.hpp>
#include <Mod/CppUserModBase.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UEngine.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <chrono>
#include <deps/nlohmann/include/nlohmann/json.hpp>

#include "restclient.hpp"

using namespace RC;
using namespace RC::Unreal;
using namespace std::chrono;
using json = nlohmann::json;

struct WeatherData {
	float temperature;
	float rain;
	float snow;
	int cloud_cover;
	int weather_code;
	float wind_speed;
	float wind_direction;
};

struct LatLon {
	float lat;
	float lon;
};

class TSWWeatherMod : public RC::CppUserModBase {
private:
	steady_clock::time_point lastRun = steady_clock::now();
	minutes interval = minutes(1);
	Unreal::UClass* locationLib;
	Unreal::UFunction* StaticFunction;

	WeatherData GetCurrentWeather() {
		auto latlon = GetCurrentPositionInGame();
		RestClient client(L"api.open-meteo.com", INTERNET_DEFAULT_HTTPS_PORT, true);

		json getResponse = client.Get(
			L"/v1/"
			L"forecast?latitude=52.52&longitude=13.41&current=temperature_2m,"
			L"relative_humidity_2m,apparent_temperature,precipitation,rain,showers,"
			L"snowfall,weather_code,cloud_cover,pressure_msl,surface_pressure,wind_"
			L"speed_10m,wind_direction_10m,wind_gusts_10m");

		WeatherData data{};
		data.temperature = getResponse["current"]["temperature_2m"];
		data.rain = getResponse["current"]["rain"];
		data.snow = getResponse["current"]["snowfall"];
		data.cloud_cover = getResponse["current"]["cloud_cover"];
		data.weather_code = getResponse["current"]["weather_code"];
		data.wind_speed = getResponse["current"]["wind_speed_10m"];
		data.wind_direction = getResponse["current"]["wind_direction_10m"];

		return data;
	}

	AActor* GetCameraActor() {
		auto cameraActorClass = TEXT("/Game/Core/Player/TS2DefaultCameraManager.TS2DefaultCameraManager_C");
		auto cameraActor = Unreal::UObjectGlobals::FindFirstOf(cameraActorClass);

		if (cameraActor) {
			Output::send<LogLevel::Verbose>(STR("Camera Actor found\n"));
			return (AActor*)cameraActor;
		}
		else {
			Output::send<LogLevel::Error>(STR("Camera Actor not found\n"));
			return nullptr;
		}
	}

	LatLon GetCurrentPositionInGame() {
		LatLon latlon{};
		latlon.lat = 0.0f;
		latlon.lon = 0.0f;

		locationLib = Unreal::UObjectGlobals::FindObject<Unreal::UClass>(
			nullptr, TEXT("/Script/TS2Prototype.TS2DebugFunctionLibrary"));

		if (!locationLib) {
			Output::send<LogLevel::Error>(STR("Location library not found\n"));
			return latlon;
		}

		for (UFunction* Function : locationLib->ForEachFunction())
		{
			if (Function->GetNamePrivate().ToString() == STR("GetActorLatLong"))
			{
				StaticFunction = Function;

				Output::send<LogLevel::Verbose>(STR("Static Function found"));
				break;
			}
		}


		if (!StaticFunction) {
			Output::send<LogLevel::Error>(STR("Static function not found\n"));
			return latlon;
		}

		struct {
			Unreal::AActor* Actor;
			float* OutLat;
			float* OutLong;
			bool* success;
		} Params;

		float lat, lon;
		bool success = false;

		Params.Actor = GetCameraActor();
		Params.OutLat = &lat;
		Params.OutLong = &lon;
		Params.success = &success;

		locationLib->ProcessEvent(StaticFunction, &Params);

		// send lat and long and sucess to Output
		Output::send<LogLevel::Verbose>(STR("Lat: {}, Long: {}, Success: {}\n"),
			lat, lon, success);

		return latlon;
	}

	void SyncWeather() {
		Output::send<LogLevel::Verbose>(STR("Syncing weather\n"));
		WeatherData data = GetCurrentWeather();
	}

public:
	TSWWeatherMod() : CppUserModBase() {
		ModName = STR("TSWWeatherMod");
		ModVersion = STR("1.0");
		ModDescription = STR("Real life weather sync for TSW5");
		ModAuthors = STR("Luex");
		// Do not change this unless you want to target a UE4SS version
		// other than the one you're currently building with somehow.
		// ModIntendedSDKVersion = STR("2.6");

		Output::send<LogLevel::Verbose>(STR("TSWWeatherMod Code running\n"));
	}

	~TSWWeatherMod() override {}

	auto on_update() -> void override {
		auto now = steady_clock::now();
		if (now - lastRun >= interval) {
			SyncWeather();
			lastRun = now;
		}
	}
};

#define TSW_WEATHER_MOD_API __declspec(dllexport)
extern "C" {
	TSW_WEATHER_MOD_API RC::CppUserModBase* start_mod() {
		return new TSWWeatherMod();
	}

	TSW_WEATHER_MOD_API void uninstall_mod(RC::CppUserModBase* mod) { delete mod; }
}
