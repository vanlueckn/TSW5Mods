#include <DynamicOutput/DynamicOutput.hpp>
#include <Mod/CppUserModBase.hpp>
#include <Unreal/FText.hpp>
#include <Unreal/UClass.hpp>
#include <restclient.hpp>

#include "tswhelper.hpp"

using namespace RC;
using namespace RC::Unreal;
using namespace std::chrono;
using namespace TSWShared;
using json = nlohmann::json;

static bool RealWeatherEnabled = false;

struct weather_data
{
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

static auto enable_disable_main_menu_slider_childs(const uintptr_t* p, const size_t len, const bool enable)
{
    for (size_t i = 0; i < len; ++i)
    {
        const auto element = reinterpret_cast<UObject*>(p[i]);
        const auto element_class = element->GetClassPrivate();
        if (!element || !element_class)
        {
            Output::send<LogLevel::Error>(STR("Element or Element class invalid\n"));
            continue;
        }

        const auto blocked_function = TSWHelper::get_function_by_name_in_chain(STR("SetBlocked"), element_class);
        if (!blocked_function)
        {
            Output::send<LogLevel::Error>(STR("Blocked function not found\n"));
            continue;
        }

        struct
        {
            bool blocked;
        } params{};

        params.blocked = enable;
        element->ProcessEvent(blocked_function, &params);
    }
}

auto mode_slider_settings_post_update_mode(const UnrealScriptFunctionCallableContext& context,
                                           void* custom_data) -> void
{
    const auto mode_slider = context.Context->GetValuePtrByPropertyNameInChain<UObject*>(STR("ModeSlider"));
    const auto presets_slider = context.Context->GetValuePtrByPropertyNameInChain<UObject*>(STR("PresetsSlider"));

    //Main Menu Slider childs
    const auto fog_density = context.Context->GetValuePtrByPropertyNameInChain<UObject*>(STR("FogDensity"));
    const auto cloud_level = context.Context->GetValuePtrByPropertyNameInChain<UObject*>(STR("CloudLevel"));
    const auto snow_level = context.Context->GetValuePtrByPropertyNameInChain<UObject*>(STR("SnowLevel"));
    const auto wind_strength = context.Context->GetValuePtrByPropertyNameInChain<UObject*>(STR("WindStrength"));
    const auto wetness = context.Context->GetValuePtrByPropertyNameInChain<UObject*>(STR("Wetness"));
    const auto precipitation = context.Context->GetValuePtrByPropertyNameInChain<UObject*>(STR("Precipitation"));

    const auto elements = new uintptr_t[]{
        reinterpret_cast<uintptr_t>(*fog_density),
        reinterpret_cast<uintptr_t>(*cloud_level),
        reinterpret_cast<uintptr_t>(*snow_level),
        reinterpret_cast<uintptr_t>(*wind_strength),
        reinterpret_cast<uintptr_t>(*wetness),
        reinterpret_cast<uintptr_t>(*precipitation)
    };

    if (!mode_slider || !(*mode_slider) || !presets_slider || !(*presets_slider))
    {
        Output::send<LogLevel::Error>(STR("ModeSlider or PresetsSlider not found\n"));
        return;
    }

    if (!fog_density || !cloud_level || !snow_level || !wind_strength || !wetness || !precipitation)
    {
        Output::send<LogLevel::Error>(
            STR("FogDensity, CloudLevel, SnowLevel, WindStrength, Wetness or Precipitation not found\n"));
        return;
    }

    if (!(*fog_density) || !(*cloud_level) || !(*snow_level) || !(*wind_strength) || !(*wetness) || !(*precipitation))
    {
        Output::send<LogLevel::Error>(
            STR("FogDensity, CloudLevel, SnowLevel, WindStrength, Wetness or Precipitation invalid\n"));
        return;
    }

    if (!(*mode_slider)->GetClassPrivate() || !(*presets_slider)->GetClassPrivate())
    {
        Output::send<LogLevel::Error>(STR("ModeSlider or PresetsSlider class invalid\n"));
        return;
    }

    for (UStruct* Struct = (*presets_slider)->IsA<UStruct>()
                               ? static_cast<UStruct*>(*presets_slider)
                               : (*presets_slider)->GetClassPrivate(); FProperty* property : Struct->
         ForEachPropertyInChain())
    {
        Output::send<LogLevel::Verbose>(
            STR("Property: {} with Type {}\n"), property->GetFName().ToString(), property->GetCPPType().GetCharArray());
    }
    const auto presets_slide_display_text = (*presets_slider)->GetValuePtrByPropertyNameInChain<FText>(
        STR("DisplayText"));
    if (!presets_slide_display_text)
    {
        Output::send<LogLevel::Error>(STR("PresetsSliderDisplayText class invalid\n"));
        return;
    }
    const auto presets_slider_text_box = (*presets_slider)->GetValuePtrByPropertyNameInChain<UObject
        *>(STR("buttonText"));
    if (!presets_slider_text_box || !(*presets_slider_text_box))
    {
        Output::send<LogLevel::Error>(STR("PresetsSliderTextBox class invalid\n"));
        return;
    }
    const auto presets_slider_text_box_class = (*presets_slider_text_box)->GetClassPrivate();
    const auto set_text_function = TSWHelper::get_function_by_name_in_chain(
        STR("SetText"), presets_slider_text_box_class);
    if (!set_text_function)
    {
        Output::send<LogLevel::Error>(STR("SetText function not found\n"));
        return;
    }

    const auto get_text_function = TSWHelper::get_function_by_name_in_chain(
        STR("GetText"), presets_slider_text_box_class);
    if (!get_text_function)
    {
        Output::send<LogLevel::Error>(STR("GetText function not found\n"));
        return;
    }

    const auto mode_slider_class = (*mode_slider)->GetClassPrivate();
    const auto presets_slider_class = (*presets_slider)->GetClassPrivate();
    const auto enable_function = TSWHelper::get_function_by_name_in_chain(STR("Enable"), presets_slider_class);
    const auto disable_function = TSWHelper::get_function_by_name_in_chain(STR("Disable"), presets_slider_class);
    const auto selected_option_index = TSWHelper::get_function_by_name_in_chain(
        STR("GetSelectedOptionIndex"), mode_slider_class);

    if (!enable_function || !disable_function || !selected_option_index)
    {
        Output::send<LogLevel::Error>(STR("Enable or disable function not found\n"));
        return;
    }

    struct
    {
        int32 index;
    } params{};

    struct
    {
        FText Text;
    } params_text;

    struct
    {
        FText Text;
    } params_get_text;

    params_text.Text = FText(STR("Custom"));

    (*mode_slider)->ProcessEvent(selected_option_index, &params);
    Output::send<LogLevel::Default>(STR("Selected index: {}\n"), params.index);

    if (params.index == 2)
    {
        presets_slide_display_text->SetString(FString(STR("Custom")));
        Output::send<LogLevel::Verbose>(STR("Selected text: {}\n"), params_get_text.Text.ToString());
        (*presets_slider)->ProcessEvent(set_text_function, &params_text);
        (*presets_slider)->ProcessEvent(disable_function, nullptr);
        RealWeatherEnabled = true;
        enable_disable_main_menu_slider_childs(elements, 6, true);
    }
    else
    {
        (*presets_slider)->ProcessEvent(enable_function, nullptr);
        RealWeatherEnabled = false;

        if (params.index == 1)
        {
            enable_disable_main_menu_slider_childs(elements, 6, false);
        }
    }
}

auto mode_slider_populate_callback(const UnrealScriptFunctionCallableContext& context, void* custom_data) -> void
{
    RealWeatherEnabled = false;
    const auto mode_slider = context.Context->GetValuePtrByPropertyNameInChain<UObject*>(STR("ModeSlider"));

    if (!mode_slider || !(*mode_slider))
    {
        Output::send<LogLevel::Error>(STR("ModeSlider not found\n"));
        return;
    }

    if (!(*mode_slider)->GetClassPrivate())
    {
        Output::send<LogLevel::Error>(STR("ModeSlider class invalid\n"));
    }

    const auto mode_slider_class = (*mode_slider)->GetClassPrivate();
    const auto add_option_function = TSWHelper::get_function_by_name_in_chain(STR("AddOption"), mode_slider_class);

    if (!add_option_function)
    {
        Output::send<LogLevel::Error>(STR("AddOption function not found\n"));
        return;
    }

    struct
    {
        FString Option;
    } params{};
    params.Option = FString(STR("Real Weather"));
    (*mode_slider)->ProcessEvent(add_option_function, &params);
}

class TSWWeatherMod final : public CppUserModBase
{
private:
    UObject* game_instance_cached_ = nullptr;
    steady_clock::time_point last_run_ = steady_clock::now();
    steady_clock::time_point last_run_minute_ = steady_clock::now();
    minutes interval_ = minutes(15);
    minutes interval_minute_ = minutes(1);
    UClass* location_lib_ = nullptr;
    UClass* weather_manager_ = nullptr;
    UFunction* static_function_ = nullptr;
    bool initialized_ = false;
    lat_lon last_lat_lon_ = {0.0f, 0.0f};

    static void inject_settings_menu()
    {
        auto static main_menu_weather_environment_settings = STR("MainMenuWeatherEnvironmentSettings_C");
        const auto mainMenuWeatherEnvironmentSettingsClass = UObjectGlobals::FindFirstOf(
            main_menu_weather_environment_settings);

        if (!mainMenuWeatherEnvironmentSettingsClass)
        {
            Output::send<LogLevel::Error>(STR("MainMenuWeatherEnvironmentSettings not found\n"));
            return;
        }

        const auto construct_function = mainMenuWeatherEnvironmentSettingsClass->GetFunctionByName(
            STR("PopulateModeSlider"));
        const auto updateModeFunction = mainMenuWeatherEnvironmentSettingsClass->GetFunctionByName(STR("UpdateMode"));

        UObjectGlobals::RegisterHook(construct_function, [](...)
        {
        }, &mode_slider_populate_callback, mainMenuWeatherEnvironmentSettingsClass);
        UObjectGlobals::RegisterHook(updateModeFunction, [](...)
        {
        }, &mode_slider_settings_post_update_mode, mainMenuWeatherEnvironmentSettingsClass);

        //modeSlider->ProcessEvent(GetFunctionByName(STR("PopulateModeSlider"), modeSlider), PopulateModeSliderCallback);
    }

    void ApplyWeatherData(const weather_data& data)
    {
        auto static set_temperature = STR("ts2.dbg.SetTemperature");
        auto static set_wetness = STR("ts2.dbg.SetWetness");
        auto static set_wind_angle = STR("ts2.dbg.SetWindAngle");
        auto static set_wind_strength = STR("ts2.dbg.SetWindStrength");
        auto static set_precipitation = STR("ts2.dbg.SetPrecipitation");
        auto static set_piled_snow = STR("ts2.dbg.SetPiledSnow");
        auto static set_ground_snow = STR("ts2.dbg.SetGroundSnow");
        auto static set_fog_density = STR("ts2.dbg.SetFogDensity");
        auto static set_cloudiness = STR("ts2.dbg.SetCloudiness");

        const auto game_instance = get_game_instance_cached();
        if (!game_instance) return;

        const auto LocalPlayersArray = game_instance->GetValuePtrByPropertyNameInChain<TArray<UObject*>>(
            STR("LocalPlayers"));
        const auto local_player = LocalPlayersArray->operator[](0);
        const auto player_controller = *local_player->GetValuePtrByPropertyNameInChain<UObject*>(
            STR("PlayerController"));

        const auto kismet_lib = TSWHelper::get_instance()->get_kismet_library_cached();
        if (!kismet_lib) return;

        const auto kismet_static_function = TSWHelper::get_function_by_name(STR("ExecuteConsoleCommand"), kismet_lib);
        if (!kismet_static_function)
        {
            Output::send<LogLevel::Error>(STR("Static kismet function not found\n"));
            return;
        }


        TSWHelper::execute_console_command(
            FString(std::format(L"{} {}", set_temperature, static_cast<int>(roundf(data.temperature))).c_str()),
            player_controller, kismet_static_function, kismet_lib);
        //ExecuteConsoleCommand(FString(std::format(L"{} {}", setWetness, data.rain).c_str()), playerController, kismetStaticFunction, kismetLib);
        TSWHelper::execute_console_command(
            FString(std::format(L"{} {}", set_wind_angle, static_cast<int>(roundf(data.wind_direction))).c_str()),
            player_controller, kismet_static_function, kismet_lib);
        TSWHelper::execute_console_command(
            FString(std::format(L"{} {}", set_wind_strength, calculate_wind_value(data.wind_speed)).c_str()),
            player_controller, kismet_static_function, kismet_lib);
        TSWHelper::execute_console_command(
            FString(std::format(L"{} {}", set_precipitation, calculate_rain_value(data.percipitation)).c_str()),
            player_controller, kismet_static_function, kismet_lib);
        //ExecuteConsoleCommand(FString(std::format(L"{} {}", setPiledSnow, data.snow).c_str()), playerController, kismetStaticFunction, kismetLib);
        //ExecuteConsoleCommand(FString(std::format(L"{} {}", setGroundSnow, data.snow).c_str()), playerController, kismetStaticFunction, kismetLib);
        TSWHelper::execute_console_command(
            FString(std::format(L"{} {}", set_fog_density, calculate_fog_value(data.visibility)).c_str()),
            player_controller, kismet_static_function, kismet_lib);
        TSWHelper::execute_console_command(
            FString(std::format(L"{} {}", set_cloudiness, data.cloud_cover / 100).c_str()),
            player_controller, kismet_static_function, kismet_lib);
    }

    static float calculate_wind_value(const float input)
    {
        constexpr float input_start = 10.0f;
        constexpr float input_end = 50.0f;
        constexpr float output_start = 0.1f;
        constexpr float output_end = 1.0f;
        constexpr float output_cap = 0.09f;

        constexpr float slope = (output_end - output_start) / (input_end - input_start);

        const float output = output_start + (input - input_start) * slope;

        return (output < output_cap) ? 0.0f : output;
    }


    static float calculate_rain_value(const float input)
    {
        constexpr float input_start = 0.025f;
        constexpr float input_end = 0.5f;
        constexpr float output_start = 0.1f;
        constexpr float output_end = 1.0f;
        constexpr float output_cap = 1.0f;

        constexpr float slope = (output_end - output_start) / (input_end - input_start);

        const float output = output_start + (input - input_start) * slope;

        return (output > output_cap) ? 1.0f : output;
    }

    static float calculate_fog_value(const float input)
    {
        constexpr float input_start = 68.0f;
        constexpr float input_end = 150.0f;
        constexpr float output_start = 0.3f;
        constexpr float output_end = 0.03f;
        constexpr float output_cap = 0.0001f;

        constexpr float slope = (output_end - output_start) / (input_end - input_start);

        float output = output_start + (input - input_start) * slope;

        if (input < 40 && input >= 20) output = output * 2;
        if (input < 20) output = output * 4;

        return (output < output_cap) ? 0.0f : output;
    }

    static weather_data get_current_weather(lat_lon latlon)
    {
        RestClient client(L"api.open-meteo.com", INTERNET_DEFAULT_HTTPS_PORT, true);

        json get_response = client.get(std::format(
            L"/v1/forecast?latitude={}&longitude={}&current=temperature_2m,relative_humidity_2m,apparent_temperature,precipitation,rain,showers,snowfall,weather_code,cloud_cover,pressure_msl,surface_pressure,wind_speed_10m,wind_direction_10m,wind_gusts_10m,visibility",
            latlon.lat, latlon.lon));

        weather_data data;
        data.temperature = get_response["current"]["temperature_2m"];
        data.rain = get_response["current"]["rain"];
        data.snow = get_response["current"]["snowfall"];
        data.cloud_cover = get_response["current"]["cloud_cover"];
        data.weather_code = get_response["current"]["weather_code"];
        data.wind_speed = get_response["current"]["wind_speed_10m"];
        data.wind_direction = get_response["current"]["wind_direction_10m"];
        data.visibility = get_response["current"]["visibility"];
        data.percipitation = get_response["current"]["precipitation"];

        return data;
    }

    UObject* get_game_instance_cached()
    {
        if (game_instance_cached_)
        {
            return game_instance_cached_;
        }

        game_instance_cached_ = get_game_instance();
        return game_instance_cached_;
    }

    static UObject* get_game_instance()
    {
        const auto game_instance = UObjectGlobals::FindFirstOf(STR("GameInstance"));

        if (!game_instance)
        {
            Output::send<LogLevel::Error>(STR("Game Instance not found\n"));
            return nullptr;
        }

        return game_instance;
    }

    void sync_weather(const lat_lon pos)
    {
        if (!RealWeatherEnabled) return;
        Output::send<LogLevel::Verbose>(STR("Syncing weather\n"));
        const weather_data data = get_current_weather(pos);
        ApplyWeatherData(data);
    }

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

        Output::send<LogLevel::Verbose>(STR("TSWWeatherMod Code running\n"));
    }

    ~TSWWeatherMod() override
    {
    }

    auto on_update() -> void override
    {
        if (!initialized_) return;
        const auto now = steady_clock::now();
        if (now - last_run_ >= interval_)
        {
            const auto latlon = TSWHelper::get_instance()->get_current_position_in_game();
            last_lat_lon_ = latlon;
            sync_weather(latlon);
            last_run_ = now;
        }
        else if (now - last_run_minute_ >= interval_minute_)
        {
            const auto latlon = TSWHelper::get_instance()->get_current_position_in_game();
            const auto distance = TSWHelper::calculate_distance_in_miles_lat_lon(latlon, last_lat_lon_);
            Output::send<LogLevel::Verbose>(STR("Distance: {}mi\n"), distance);
            if (distance > 40)
            {
                //force sync
                sync_weather(latlon);
            }
            last_lat_lon_ = latlon;
            last_run_minute_ = now;
        }
    }

    auto on_unreal_init() -> void override
    {
        initialized_ = true;
        inject_settings_menu();
    }

    static UObject* get_text_slider_with_arrows()
    {
        const auto text_slider_with_arrows = UObjectGlobals::FindFirstOf(STR("TextSliderWithArrows"));
        if (!text_slider_with_arrows)
        {
            Output::send<LogLevel::Error>(STR("TextSliderWithArrows not found\n"));
            return nullptr;
        }
        return text_slider_with_arrows;
    }
};

#define TSW_WEATHER_MOD_API __declspec(dllexport)

extern "C" {
TSW_WEATHER_MOD_API CppUserModBase* start_mod()
{
    return new TSWWeatherMod();
}

TSW_WEATHER_MOD_API void uninstall_mod(CppUserModBase* mod) { delete mod; }
}
