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

static bool real_weather_enabled = false;
static bool historical_weather_enabled = false;
constexpr float epsilon = 1e-6f;

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

static auto mode_slider_settings_post_update_mode(const UnrealScriptFunctionCallableContext& context,
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
        FText text;
    } params_text;

    struct
    {
        FText Text;
    } params_get_text;

    params_text.text = FText(STR("Custom"));

    (*mode_slider)->ProcessEvent(selected_option_index, &params);
    Output::send<LogLevel::Default>(STR("Selected index: {}\n"), params.index);

    if (params.index >= 2)
    {
        presets_slide_display_text->SetString(FString(STR("Custom")));
        Output::send<LogLevel::Verbose>(STR("Selected text: {}\n"), params_get_text.Text.ToString());
        (*presets_slider)->ProcessEvent(set_text_function, &params_text);
        (*presets_slider)->ProcessEvent(disable_function, nullptr);
        historical_weather_enabled = false;
        real_weather_enabled = true;
        enable_disable_main_menu_slider_childs(elements, 6, true);
        if (params.index == 3)
        {
            historical_weather_enabled = true;
        }
    }
    else
    {
        (*presets_slider)->ProcessEvent(enable_function, nullptr);
        real_weather_enabled = false;

        if (params.index == 1)
        {
            enable_disable_main_menu_slider_childs(elements, 6, false);
        }
    }
}

static auto mode_slider_populate_callback(const UnrealScriptFunctionCallableContext& context, void* custom_data) -> void
{
    real_weather_enabled = false;
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
    params.Option = FString(STR("Live Weather"));
    (*mode_slider)->ProcessEvent(add_option_function, &params);
    struct
    {
        FString Option;
    } params2{};
    params2.Option = FString(STR("Historical Weather"));
    (*mode_slider)->ProcessEvent(add_option_function, &params2);
}

class TSWWeatherMod final : public CppUserModBase
{
    steady_clock::time_point last_time_force_sync_ = steady_clock::now();
    std::function<void()>* console_command_callback_ = nullptr;
    UObject* game_instance_cached_ = nullptr;
    steady_clock::time_point last_run_ = steady_clock::now();
    steady_clock::time_point last_run_minute_ = steady_clock::now();
    minutes interval_ = minutes(5);
    minutes interval_minute_ = minutes(1);
    UClass* location_lib_ = nullptr;
    UClass* weather_manager_ = nullptr;
    UFunction* static_function_ = nullptr;
    bool initialized_ = false;
    lat_lon last_lat_lon_ = {.lat = 0.0f, .lon = 0.0f, .kph = 0.0f};

    static void inject_settings_menu()
    {
        auto static main_menu_weather_environment_settings = STR("MainMenuWeatherEnvironmentSettings_C");
        const auto main_menu_weather_environment_settings_class = UObjectGlobals::FindFirstOf(
            main_menu_weather_environment_settings);

        if (!main_menu_weather_environment_settings_class)
        {
            Output::send<LogLevel::Error>(STR("MainMenuWeatherEnvironmentSettings not found\n"));
            return;
        }

        const auto construct_function = main_menu_weather_environment_settings_class->GetFunctionByName(
            STR("PopulateModeSlider"));
        const auto update_mode_function = main_menu_weather_environment_settings_class->GetFunctionByName(
            STR("UpdateMode"));

        UObjectGlobals::RegisterHook(construct_function, [](...)
        {
        }, &mode_slider_populate_callback, main_menu_weather_environment_settings_class);
        UObjectGlobals::RegisterHook(update_mode_function, [](...)
        {
        }, &mode_slider_settings_post_update_mode, main_menu_weather_environment_settings_class);

        //modeSlider->ProcessEvent(GetFunctionByName(STR("PopulateModeSlider"), modeSlider), PopulateModeSliderCallback);
    }

    void apply_weather_data(const weather_data& data)
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

        const auto local_players_array = game_instance->GetValuePtrByPropertyNameInChain<TArray<UObject*>>(
            STR("LocalPlayers"));
        const auto local_player = local_players_array->operator[](0);
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
            FString(std::format(L"{} {}", set_precipitation,
                                calculate_rain_value(data.snow > 0 && data.temperature < 5 ? data.snow : data.rain)).
                c_str()),
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

        return output < output_cap + epsilon ? 0.0f : output;
    }


    static float calculate_rain_value(const float input)
    {
        constexpr float input_start = 0.15f;
        constexpr float input_end = 3.175f;
        constexpr float output_start = 0.05f;
        constexpr float output_end = 1.0f;
        constexpr float output_cap = 1.0f;

        constexpr float slope = (output_end - output_start) / (input_end - input_start);

        const float output = output_start + (input - input_start) * slope;

        // Explicit clamping for small values
        if (output < 0.02f + epsilon)
        {
            return 0.0f;
        }

        return output > output_cap ? 1.0f : output;
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

        if (input < 40 && input >= 20)
            output = output * 2;
        if (input < 20)
            output = output * 4;

        return output < output_cap + epsilon ? 0.0f : output;
    }

    static weather_data get_current_weather(lat_lon latlon, bool& error)
    {
        weather_data data{};
        data.visibility = 10000;

        RestClient client(historical_weather_enabled ? L"archive-api.open-meteo.com" : L"api.open-meteo.com",
                          INTERNET_DEFAULT_HTTPS_PORT, true);

        std::wstring url_scheme;
        tsw_date_time game_time;
        if (historical_weather_enabled)
        {
            const auto actor = TSWHelper::get_camera_actor();
            if (!actor)
            {
                error = true;
                return data;
            }

            const auto [year, month, day] = TSWHelper::get_current_utc_date();
            game_time = TSWHelper::get_world_date_time(actor);

            if (game_time.year > year ||
                (game_time.year == year && (game_time.month > month ||
                    (game_time.month == month && game_time.day > day))))
            {
                game_time.year = year - 1;
            }

            const auto current_day_string = game_time.to_string_no_time();
            url_scheme = std::format(
                L"/v1/archive?latitude={}&longitude={}&start_date={}&end_date={}&hourly=temperature_2m,relative_humidity_2m,precipitation,rain,snowfall,weather_code,cloud_cover,wind_speed_10m,wind_direction_10m,wind_gusts_10m",
                latlon.lat, latlon.lon, current_day_string, current_day_string);
        }
        else
        {
            url_scheme = std::format(
                L"/v1/forecast?latitude={}&longitude={}&current=temperature_2m,relative_humidity_2m,precipitation,rain,showers,snowfall,weather_code,cloud_cover,wind_speed_10m,wind_direction_10m,wind_gusts_10m,visibility",
                latlon.lat, latlon.lon);
        }

        json get_response = client.get(url_scheme);

        const auto dmp = get_response.dump();
        Output::send<LogLevel::Verbose>(std::wstring(dmp.begin(), dmp.end()));

        if (get_response.contains("error"))
        {
            error = true;
            return data;
        }
        if (historical_weather_enabled)
        {
            const int current_hour = game_time.hour;
            if (current_hour < 0 || current_hour > 23)
            {
                error = true;
                return data;
            }
            if (!get_response.contains("hourly"))
            {
                error = true;
                return data;
            }

            data.temperature = get_response["hourly"]["temperature_2m"][current_hour];
            data.rain = get_response["hourly"]["rain"][current_hour];
            data.snow = get_response["hourly"]["snowfall"][current_hour];
            data.cloud_cover = get_response["hourly"]["cloud_cover"][current_hour];
            data.weather_code = get_response["hourly"]["weather_code"][current_hour];
            data.wind_speed = get_response["hourly"]["wind_speed_10m"][current_hour];
            data.wind_direction = get_response["hourly"]["wind_direction_10m"][current_hour];
            if (data.weather_code == 45 || data.weather_code == 48)
            {
                data.visibility = 100;
            }
            data.percipitation = get_response["hourly"]["precipitation"][current_hour];
        }
        else
        {
            if (!get_response.contains("current"))
            {
                error = true;
                return data;
            }

            data.temperature = get_response["current"]["temperature_2m"];
            data.rain = get_response["current"]["rain"];
            data.snow = get_response["current"]["snowfall"];
            data.cloud_cover = get_response["current"]["cloud_cover"];
            data.weather_code = get_response["current"]["weather_code"];
            data.wind_speed = get_response["current"]["wind_speed_10m"];
            data.wind_direction = get_response["current"]["wind_direction_10m"];
            data.visibility = get_response["current"]["visibility"];
            data.percipitation = get_response["current"]["precipitation"];
        }

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
        if (!real_weather_enabled) return;
        Output::send<LogLevel::Verbose>(STR("Syncing weather\n"));
        bool error = false;
        const weather_data data = get_current_weather(pos, error);
        if (error)
        {
            Output::send<LogLevel::Error>(STR("Weather error interpreting data\n"));
            return;
        }
        apply_weather_data(data);
    }

public:
    TSWWeatherMod() : CppUserModBase()
    {
        ModName = STR("TSWWeatherMod");
        ModVersion = STR("0.2");
        ModDescription = STR("Live weather sync for TSW5 with support for historical data.");
        ModAuthors = STR("Luex");

        Output::send<LogLevel::Verbose>(STR("TSWWeatherMod Code running\n"));
    }

    ~TSWWeatherMod() override
    {
        if (console_command_callback_)
        {
            free(console_command_callback_);
            console_command_callback_ = nullptr;
        }
    }

    auto on_update() -> void override
    {
        if (!initialized_) return;
        if (const auto now = steady_clock::now(); now - last_run_ >= interval_)
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
            if (distance > 30)
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
        TSWHelper::inject_set_date();
        TSWHelper::listen_for_console_command([this]
        {
            constexpr auto min_interval = milliseconds(250);
            const auto now = steady_clock::now();
            if (now - last_time_force_sync_ < min_interval)
            {
                return;
            }
            const auto latlong = TSWHelper::get_instance()->get_current_position_in_game();
            last_time_force_sync_ = now;
            Output::send<LogLevel::Verbose>(STR("Force Syncing weather\n"));
            sync_weather(latlong);
        });
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
