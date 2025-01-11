#include <DynamicOutput/DynamicOutput.hpp>
#include <Mod/CppUserModBase.hpp>
#include <Unreal/UClass.hpp>
#include <Unreal/UEngine.hpp>
#include <Unreal/UFunction.hpp>
#include <Unreal/UObject.hpp>
#include <Unreal/UObjectGlobals.hpp>
#include <Unreal/UKismetSystemLibrary.hpp>
#include <Unreal/FText.hpp>
#include <chrono>
#include <deps/nlohmann/include/nlohmann/json.hpp>
#include <math.h>
#include <numbers>

#include "restclient.hpp"

using namespace RC;
using namespace RC::Unreal;
using namespace std::chrono;
using json = nlohmann::json;

static bool RealWeatherEnabled = false;

struct WeatherData {
	float temperature;
	float rain;
	float snow;
	float cloud_cover;
	int weather_code;
	float wind_speed;
	float wind_direction;
	float visibility;
	float percipitation;
};

struct LatLon {
	float lat;
	float lon;
};

static UFunction* GetFunctionByNameInChain(RC::StringType name, UClass* inputObject)
{
	for (UFunction* Function : inputObject->ForEachFunctionInChain())
	{
		if (Function->GetNamePrivate().ToString() == name)
		{
			return Function;
		}
	}

	return nullptr;
}

auto EnableDisableMainMenuSliderChilds(const uintptr_t* p, const size_t len, const bool enable) {
	for (size_t i = 0; i < len; ++i)
	{
		auto element = reinterpret_cast<UObject*>(p[i]);
		auto elementClass = element->GetClassPrivate();
		if (!element || !elementClass) {
			Output::send<LogLevel::Error>(STR("Element or Element class invalid\n"));
			continue;
		}

		auto blockedFunction = GetFunctionByNameInChain(STR("SetBlocked"), elementClass);
		if (!blockedFunction) {
			Output::send<LogLevel::Error>(STR("Blocked function not found\n"));
			continue;
		}

		struct {
			bool blocked;
		} Params{};

		Params.blocked = enable;
		element->ProcessEvent(blockedFunction, &Params);
	}
}

auto ModeSliderSettingsPostUpdateMode(UnrealScriptFunctionCallableContext& context, void* customData) -> void {
	auto modeSlider = context.Context->GetValuePtrByPropertyNameInChain<UObject*>(STR("ModeSlider"));
	auto presetsSlider = context.Context->GetValuePtrByPropertyNameInChain<UObject*>(STR("PresetsSlider"));

	//Main Menu Slider childs
	auto fogDensity = context.Context->GetValuePtrByPropertyNameInChain<UObject*>(STR("FogDensity"));
	auto cloudLevel = context.Context->GetValuePtrByPropertyNameInChain<UObject*>(STR("CloudLevel"));
	auto snowLevel = context.Context->GetValuePtrByPropertyNameInChain<UObject*>(STR("SnowLevel"));
	auto windStrength = context.Context->GetValuePtrByPropertyNameInChain<UObject*>(STR("WindStrength"));
	auto wetness = context.Context->GetValuePtrByPropertyNameInChain<UObject*>(STR("Wetness"));
	auto precipitation = context.Context->GetValuePtrByPropertyNameInChain<UObject*>(STR("Precipitation"));

	auto elements = new uintptr_t[]{
		(uintptr_t)(*fogDensity),
		(uintptr_t)(*cloudLevel),
		(uintptr_t)(*snowLevel),
		(uintptr_t)(*windStrength),
		(uintptr_t)(*wetness),
		(uintptr_t)(*precipitation)
	};

	if (!modeSlider || !(*modeSlider) || !presetsSlider || !(*presetsSlider)) {
		Output::send<LogLevel::Error>(STR("ModeSlider or PresetsSlider not found\n"));
		return;
	}

	if (!fogDensity || !cloudLevel || !snowLevel || !windStrength || !wetness || !precipitation) {
		Output::send<LogLevel::Error>(STR("FogDensity, CloudLevel, SnowLevel, WindStrength, Wetness or Precipitation not found\n"));
		return;
	}

	if (!(*fogDensity) || !(*cloudLevel) || !(*snowLevel) || !(*windStrength) || !(*wetness) || !(*precipitation)) {
		Output::send<LogLevel::Error>(STR("FogDensity, CloudLevel, SnowLevel, WindStrength, Wetness or Precipitation invalid\n"));
		return;
	}

	if (!(*modeSlider)->GetClassPrivate() || !(*presetsSlider)->GetClassPrivate()) {
		Output::send<LogLevel::Error>(STR("ModeSlider or PresetsSlider class invalid\n"));
		return;
	}

	auto presetsSliderTextBox = (*presetsSlider)->GetValuePtrByPropertyNameInChain<UObject*>(STR("buttonText"));
	if (!presetsSliderTextBox || !(*presetsSliderTextBox)) {
		Output::send<LogLevel::Error>(STR("PresetsSliderTextBox class invalid\n"));
		return;
	}

	auto presetsSliderTextBoxClass = (*presetsSliderTextBox)->GetClassPrivate();
	auto setTextFunction = GetFunctionByNameInChain(STR("SetText"), presetsSliderTextBoxClass);
	if (!setTextFunction) {
		Output::send<LogLevel::Error>(STR("SetText function not found\n"));
		return;
	}

	auto modeSliderClass = (*modeSlider)->GetClassPrivate();
	auto presetsSliderClass = (*presetsSlider)->GetClassPrivate();
	auto enableFunction = GetFunctionByNameInChain(STR("Enable"), presetsSliderClass);
	auto disableFunction = GetFunctionByNameInChain(STR("Disable"), presetsSliderClass);
	auto selectedOptionIndex = GetFunctionByNameInChain(STR("GetSelectedOptionIndex"), modeSliderClass);

	if (!enableFunction || !disableFunction || !selectedOptionIndex) {
		Output::send<LogLevel::Error>(STR("Enable or disable function not found\n"));
		return;
	}

	struct {
		int32 Index;
	} Params{};

	struct {
		FText Text;
	} ParamsText{};

	ParamsText.Text = FText(STR("Custom"));

	(*modeSlider)->ProcessEvent(selectedOptionIndex, &Params);
	Output::send<LogLevel::Default>(STR("Selected index: {}\n"), Params.Index);

	if (Params.Index == 2) {
		(*presetsSlider)->ProcessEvent(setTextFunction, &ParamsText);
		(*presetsSlider)->ProcessEvent(disableFunction, nullptr);
		RealWeatherEnabled = true;
		EnableDisableMainMenuSliderChilds(elements, 6, true);
	}
	else {
		(*presetsSlider)->ProcessEvent(enableFunction, nullptr);
		RealWeatherEnabled = false;

		if (Params.Index == 1) {
			EnableDisableMainMenuSliderChilds(elements, 6, false);
		}
	}
}

auto ModeSliderPopulateCallback(UnrealScriptFunctionCallableContext& context, void* customData) -> void {
	RealWeatherEnabled = false;
	auto modeSlider = context.Context->GetValuePtrByPropertyNameInChain<UObject*>(STR("ModeSlider"));

	if (!modeSlider || !(*modeSlider)) {
		Output::send<LogLevel::Error>(STR("ModeSlider not found\n"));
		return;
	}

	if (!(*modeSlider)->GetClassPrivate()) {
		Output::send<LogLevel::Error>(STR("ModeSlider class invalid\n"));
	}

	auto modeSliderClass = (*modeSlider)->GetClassPrivate();
	auto addOptionFunction = GetFunctionByNameInChain(STR("AddOption"), modeSliderClass);

	if (!addOptionFunction) {
		Output::send<LogLevel::Error>(STR("AddOption function not found\n"));
		return;
	}

	struct {
		FString Option;
	} Params{};
	Params.Option = FString(STR("Real Weather"));
	(*modeSlider)->ProcessEvent(addOptionFunction, &Params);
}

class TSWWeatherMod : public RC::CppUserModBase {
private:
	UObject* playerControllerCached = nullptr;
	UObject* kismetLibraryCached = nullptr;
	UObject* gameInstanceCached = nullptr;
	steady_clock::time_point lastRun = steady_clock::now();
	steady_clock::time_point lastRunMinute = steady_clock::now();
	minutes interval = minutes(15);
	minutes intervalMinute = minutes(1);
	Unreal::UClass* locationLib;
	Unreal::UClass* weatherManager;
	Unreal::UFunction* StaticFunction;
	bool initialized = false;
	LatLon lastLatLon = { 0.0f, 0.0f };

	void InjectSettingsMenu() {
		auto static mainMenuWeatherEnvironmentSettings = STR("MainMenuWeatherEnvironmentSettings_C");
		auto mainMenuWeatherEnvironmentSettingsClass = UObjectGlobals::FindFirstOf(mainMenuWeatherEnvironmentSettings);

		if (!mainMenuWeatherEnvironmentSettingsClass) {
			Output::send<LogLevel::Error>(STR("MainMenuWeatherEnvironmentSettings not found\n"));
			return;
		}

		for (UFunction* Function : UObjectGlobals::FindObject<UClass>(nullptr, STR("/Game/Core/Assets/HUD/MenuScreens/WeatherSettings/WeatherSliderButton.WeatherSliderButton_C"))->ForEachFunction())
		{
			Output::send<LogLevel::Error>(STR("Function: {}\n"), Function->GetNamePrivate().ToString());
		}

		auto constructFunction = mainMenuWeatherEnvironmentSettingsClass->GetFunctionByName(STR("PopulateModeSlider"));
		auto updateModeFunction = mainMenuWeatherEnvironmentSettingsClass->GetFunctionByName(STR("UpdateMode"));

		UObjectGlobals::RegisterHook(constructFunction, [](...) {}, &ModeSliderPopulateCallback, mainMenuWeatherEnvironmentSettingsClass);
		UObjectGlobals::RegisterHook(updateModeFunction, [](...) {}, &ModeSliderSettingsPostUpdateMode, mainMenuWeatherEnvironmentSettingsClass);

		//modeSlider->ProcessEvent(GetFunctionByName(STR("PopulateModeSlider"), modeSlider), PopulateModeSliderCallback);

	}

	void ApplyWeatherData(WeatherData data) {
		auto static setTemperature = STR("ts2.dbg.SetTemperature");
		auto static setWetness = STR("ts2.dbg.SetWetness");
		auto static setWindAngle = STR("ts2.dbg.SetWindAngle");
		auto static setWindStrength = STR("ts2.dbg.SetWindStrength");
		auto static setPrecipitation = STR("ts2.dbg.SetPrecipitation");
		auto static setPiledSnow = STR("ts2.dbg.SetPiledSnow");
		auto static setGroundSnow = STR("ts2.dbg.SetGroundSnow");
		auto static setFogDensity = STR("ts2.dbg.SetFogDensity");
		auto static setCloudiness = STR("ts2.dbg.SetCloudiness");

		auto gameInstance = GetGameInstanceCached();
		if (!gameInstance) return;

		auto LocalPlayersArray = gameInstance->GetValuePtrByPropertyNameInChain<TArray<UObject*>>(STR("LocalPlayers"));
		auto localPlayer = LocalPlayersArray->operator[](0);
		auto playerController = *localPlayer->GetValuePtrByPropertyNameInChain<UObject*>(STR("PlayerController"));

		auto kismetLib = GetKismetLibraryCached();
		if (!kismetLib) return;

		auto kismetStaticFunction = GetFunctionByName(STR("ExecuteConsoleCommand"), kismetLib);
		if (!kismetStaticFunction) {
			Output::send<LogLevel::Error>(STR("Static kismet function not found\n"));
			return;
		}


		ExecuteConsoleCommand(FString(std::format(L"{} {}", setTemperature, (int)roundf(data.temperature)).c_str()), playerController, kismetStaticFunction, kismetLib);
		//ExecuteConsoleCommand(FString(std::format(L"{} {}", setWetness, data.rain).c_str()), playerController, kismetStaticFunction, kismetLib);
		ExecuteConsoleCommand(FString(std::format(L"{} {}", setWindAngle, (int)roundf(data.wind_direction)).c_str()), playerController, kismetStaticFunction, kismetLib);
		ExecuteConsoleCommand(FString(std::format(L"{} {}", setWindStrength, CalculateWindValue(data.wind_speed)).c_str()), playerController, kismetStaticFunction, kismetLib);
		ExecuteConsoleCommand(FString(std::format(L"{} {}", setPrecipitation, CalculateRainValue(data.percipitation)).c_str()), playerController, kismetStaticFunction, kismetLib);
		//ExecuteConsoleCommand(FString(std::format(L"{} {}", setPiledSnow, data.snow).c_str()), playerController, kismetStaticFunction, kismetLib);
		//ExecuteConsoleCommand(FString(std::format(L"{} {}", setGroundSnow, data.snow).c_str()), playerController, kismetStaticFunction, kismetLib);
		ExecuteConsoleCommand(FString(std::format(L"{} {}", setFogDensity, CalculateFogValue(data.visibility)).c_str()), playerController, kismetStaticFunction, kismetLib);
		ExecuteConsoleCommand(FString(std::format(L"{} {}", setCloudiness, data.cloud_cover / 100).c_str()), playerController, kismetStaticFunction, kismetLib);
	}

	float CalculateWindValue(float input) {
		const float inputStart = 10.0f;
		const float inputEnd = 50.0f;
		const float outputStart = 0.1f;
		const float outputEnd = 1.0f;
		const float outputCap = 0.09f;

		float slope = (outputEnd - outputStart) / (inputEnd - inputStart);

		float output = outputStart + (input - inputStart) * slope;


		return (output < outputCap) ? 0.0f : output;
	}


	float CalculateRainValue(float input) {
		const float inputStart = 0.025f;
		const float inputEnd = 0.5f;
		const float outputStart = 0.1f;
		const float outputEnd = 1.0f;
		const float outputCap = 1.0f;

		float slope = (outputEnd - outputStart) / (inputEnd - inputStart);

		float output = outputStart + (input - inputStart) * slope;


		return (output > outputCap) ? 1.0f : output;
	}

	float CalculateFogValue(float input) {
		const float inputStart = 68.0f;
		const float inputEnd = 150.0f;
		const float outputStart = 0.3f;
		const float outputEnd = 0.03f;
		const float outputCap = 0.0001f;

		float slope = (outputEnd - outputStart) / (inputEnd - inputStart);

		float output = outputStart + (input - inputStart) * slope;

		if (input < 40 && input >= 20) output = output * 2;
		if (input < 20) output = output * 4;

		return (output < outputCap) ? 0.0f : output;
	}

	WeatherData GetCurrentWeather(LatLon latlon) {
		RestClient client(L"api.open-meteo.com", INTERNET_DEFAULT_HTTPS_PORT, true);

		json getResponse = client.Get(std::format(L"/v1/forecast?latitude={}&longitude={}&current=temperature_2m,relative_humidity_2m,apparent_temperature,precipitation,rain,showers,snowfall,weather_code,cloud_cover,pressure_msl,surface_pressure,wind_speed_10m,wind_direction_10m,wind_gusts_10m,visibility", latlon.lat, latlon.lon));

		WeatherData data{};
		data.temperature = getResponse["current"]["temperature_2m"];
		data.rain = getResponse["current"]["rain"];
		data.snow = getResponse["current"]["snowfall"];
		data.cloud_cover = getResponse["current"]["cloud_cover"];
		data.weather_code = getResponse["current"]["weather_code"];
		data.wind_speed = getResponse["current"]["wind_speed_10m"];
		data.wind_direction = getResponse["current"]["wind_direction_10m"];
		data.visibility = getResponse["current"]["visibility"];
		data.percipitation = getResponse["current"]["precipitation"];

		return data;
	}

	void ExecuteConsoleCommand(FString command, UObject* playerController, UFunction* kismetStaticFunction, UObject* kismetLib) {
		struct {
			UObject* worldContextObj;
			FString Command;
			class APlayerController* Player;
		} Params{};

		auto playerControllerCasted = (AActor*)playerController;

		Params.Player = (APlayerController*)playerController;
		Params.Command = command;
		Params.worldContextObj = playerController;

		if (!IsValidUObject(playerController)) {
			Output::send<LogLevel::Verbose>(STR("Player controller is invalid\n"));
		}

		if (!IsValidUObject(kismetStaticFunction)) {
			Output::send<LogLevel::Verbose>(STR("kismetStaticFunction is invalid\n"));
		}

		auto stringData = command.GetCharTArray().GetData();

		Output::send<LogLevel::Verbose>(STR("Executing command: {}\n"), stringData);

		kismetLib->ProcessEvent(kismetStaticFunction, &Params);
	}

	UObject* GetGameInstanceCached() {
		if (gameInstanceCached) {
			return gameInstanceCached;
		}

		gameInstanceCached = GetGameInstance();
		return gameInstanceCached;
	}

	UObject* GetGameInstance() {
		auto gameInstance = UObjectGlobals::FindFirstOf(STR("GameInstance"));

		if (!gameInstance) {
			Output::send<LogLevel::Error>(STR("Game Instance not found\n"));
			return nullptr;
		}

		return gameInstance;
	}

	UObject* GetKismetLibraryCached() {
		if (kismetLibraryCached) {
			return kismetLibraryCached;
		}

		kismetLibraryCached = GetKismetLibrary();
		return kismetLibraryCached;
	}

	UObject* GetKismetLibrary() {
		auto kismetLibrary = Unreal::UObjectGlobals::FindObject<UObject>(
			nullptr, TEXT("/Script/Engine.Default__KismetSystemLibrary"));

		if (!kismetLibrary) {
			Output::send<LogLevel::Error>(STR("Kismet library not found\n"));
			return nullptr;
		}

		return kismetLibrary;
	}

	AActor* GetCameraActor() {
		auto cameraActorClass = STR("TS2DefaultCameraManager_C");
		auto cameraActor = Unreal::UObjectGlobals::FindFirstOf(cameraActorClass);

		if (cameraActor) {
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

		locationLib = Unreal::UObjectGlobals::FindObject<UClass>(
			nullptr, TEXT("/Script/TS2Prototype.TS2DebugFunctionLibrary"));

		if (!locationLib) {
			Output::send<LogLevel::Error>(STR("Location library not found\n"));
			return latlon;
		}

		StaticFunction = GetFunctionByName(STR("GetActorLatLong"), locationLib);

		if (!StaticFunction) {
			Output::send<LogLevel::Error>(STR("Static function not found\n"));
			return latlon;
		}

		struct {
			Unreal::AActor* Actor;
			float outLat;
			float outLong;
			bool success;
		} Params{};

		Params.Actor = GetCameraActor();

		locationLib->ProcessEvent(StaticFunction, &Params);

		// send lat and long and sucess to Output
		Output::send<LogLevel::Verbose>(STR("Lat: {}, Long: {}, Success: {}\n"),
			Params.outLat, Params.outLong, Params.success);

		latlon.lat = Params.outLat;
		latlon.lon = Params.outLong;

		return latlon;
	}

	UFunction* GetFunctionByName(RC::StringType name, UObject* inputObject)
	{
		for (UFunction* Function : inputObject->GetClassPrivate()->ForEachFunction())
		{
			if (Function->GetNamePrivate().ToString() == name)
			{
				return Function;
			}
		}


		return nullptr;
	}

	UFunction* GetFunctionByName(RC::StringType name, UClass* inputClass)
	{
		for (UFunction* Function : inputClass->ForEachFunction())
		{
			if (Function->GetNamePrivate().ToString() == name)
			{
				return Function;
			}
		}
		kismetLibraryCached->GetClassPrivate()->ForEachFunction();

		return nullptr;
	}

	void SyncWeather(LatLon pos) {
		if (!RealWeatherEnabled) return;
		Output::send<LogLevel::Verbose>(STR("Syncing weather\n"));
		WeatherData data = GetCurrentWeather(pos);
		ApplyWeatherData(data);
	}

	bool IsValidUObject(UObject* object) {
		return object && UObjectArray::IsValid(object->GetObjectItem(), false);
	}

	float CalculateDistanceInMilesLatLon(LatLon p1, LatLon p2) {
		const float R = 6371e3;
		const float phi1 = p1.lat * std::numbers::pi / 180;
		const float phi2 = p2.lat * std::numbers::pi / 180;
		const float deltaPhi = (p2.lat - p1.lat) * std::numbers::pi / 180;
		const float deltaLambda = (p2.lon - p1.lon) * std::numbers::pi / 180;
		const float a = sin(deltaPhi / 2) * sin(deltaPhi / 2) +
			cos(phi1) * cos(phi2) *
			sin(deltaLambda / 2) * sin(deltaLambda / 2);
		const float c = 2 * atan2(sqrt(a), sqrt(1 - a));
		return R * c * 0.000621371;
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
		if (!initialized) return;
		auto now = steady_clock::now();
		if (now - lastRun >= interval) {
			auto latlon = GetCurrentPositionInGame();
			lastLatLon = latlon;
			SyncWeather(latlon);
			lastRun = now;
		}
		else if (now - lastRunMinute >= intervalMinute) {
			auto latlon = GetCurrentPositionInGame();
			auto distance = CalculateDistanceInMilesLatLon(latlon, lastLatLon);
			Output::send<LogLevel::Verbose>(STR("Distance: {}mi\n"), distance);
			if (distance > 40) {
				//force sync
				SyncWeather(latlon);
			}
			lastLatLon = latlon;
			lastRunMinute = now;
		}

	}

	auto on_unreal_init() -> void override {
		initialized = true;
		InjectSettingsMenu();
	}

	static UObject* GetTextSliderWithArrows() {
		auto textSliderWithArrows = UObjectGlobals::FindFirstOf(STR("TextSliderWithArrows"));
		if (!textSliderWithArrows) {
			Output::send<LogLevel::Error>(STR("TextSliderWithArrows not found\n"));
			return nullptr;
		}
		return textSliderWithArrows;
	}
};

#define TSW_WEATHER_MOD_API __declspec(dllexport)
extern "C" {
	TSW_WEATHER_MOD_API RC::CppUserModBase* start_mod() {
		return new TSWWeatherMod();
	}

	TSW_WEATHER_MOD_API void uninstall_mod(RC::CppUserModBase* mod) { delete mod; }
}
