#pragma once

#include <Unreal/AActor.hpp>
#include <string>
#include <cassert>
#include <tuple>

inline auto append_zero_padded_integer = [](std::wstring& output, int value, const int digit_count)
{
    assert(digit_count > 0); // Ensure digit count is positive
    const size_t output_index = output.size();
    output.resize(output_index + digit_count); // Reserve space for digits

    for (int i = digit_count - 1; i >= 0; --i, value /= 10)
    {
        output[output_index + i] = '0' + (value % 10); // Append digit
    }
};

namespace TSWShared
{
    struct lat_lon
    {
        float lat;
        float lon;
        float kph;
    };

    struct tsw_date_time
    {
        int year;
        int month;
        int day;
        int hour;
        int minute;
        int second;
        int ticks;

        [[nodiscard]] std::wstring to_iso8601() const
        {
            std::wstring output;
            append_zero_padded_integer(output, year, 4);
            output.append(L"-");
            append_zero_padded_integer(output, month, 2);
            output.append(L"-");
            append_zero_padded_integer(output, day, 2);
            output.append(L"T");
            append_zero_padded_integer(output, hour, 2);
            output.append(L"-");
            append_zero_padded_integer(output, minute, 2);
            output.append(L"-");
            append_zero_padded_integer(output, second, 2);
            output.append(L"Z");

            return output;
        }

        [[nodiscard]] std::wstring to_string_no_time() const
        {
            std::wstring output;
            append_zero_padded_integer(output, year, 4);
            output.append(L"-");
            append_zero_padded_integer(output, month, 2);
            output.append(L"-");
            append_zero_padded_integer(output, day, 2);

            return output;
        }
    };

    constexpr int64_t ticks_per_day = 864000000000;
    constexpr int64_t ticks_per_second = 10000000;
    constexpr int64_t ticks_per_minute = 600000000;
    constexpr int64_t ticks_per_hour = 36000000000;
    constexpr int64_t ticks_per_year = 31536000000000;
    constexpr int64_t ticks_per_month = 2628000000000;

    struct ue_datetime
    {
        Unreal::int64 ticks;
    };

    class TSWHelper
    {
        Unreal::UObject* player_controller_cached_ = nullptr;
        Unreal::UObject* kismet_library_cached_ = nullptr;

    protected:
        static TSWHelper* singleton_;

        TSWHelper()
        {
        }

    public:
        TSWHelper(TSWHelper& other) = delete;
        void operator=(const TSWHelper&) = delete;
        static TSWHelper* get_instance();

        lat_lon get_current_position_in_game(bool with_speed = false);
        static tsw_date_time get_world_date_time(Unreal::UObject* world_context_object);
        static Unreal::AActor* get_camera_actor();
        static Unreal::UFunction* get_function_by_name(const StringType& name,
                                                       Unreal::UObject* input_object);
        static Unreal::UFunction* get_function_by_name(const StringType& name, Unreal::UClass* input_class);
        Unreal::UObject* get_kismet_library_cached();
        static Unreal::UObject* get_kismet_library();
        Unreal::UObject* get_player_controller();
        Unreal::UObject* get_player_pawn();
        static Unreal::UFunction* get_function_by_name_in_chain(const StringType& name, Unreal::UClass* input_object);
        static Unreal::UFunction* get_function_by_name_in_chain(const StringType& name, Unreal::UObject* input_object);
        static float get_actor_velocity_in_kph(Unreal::AActor* actor);
        static void execute_console_command(Unreal::FString command, Unreal::UObject* player_controller,
                                            Unreal::UFunction* kismet_static_function,
                                            Unreal::UObject* kismet_lib);
        static void listen_for_console_command(std::function<void()> callback);
        static bool is_valid_u_object(Unreal::UObject* object);
        static void inject_set_date();
        static float calculate_distance_in_miles_lat_lon(const lat_lon p1, const lat_lon p2);
        static tsw_date_time ue_to_tsw_date_time(const ue_datetime& ue_date_time);
        static std::tuple<int, int, int> get_current_utc_date();


        static double get_julian_day(const ue_datetime& ue_date_time)
        {
            return static_cast<double>(1721425.5 + ue_date_time.ticks / ticks_per_day);
        }
    };
}
