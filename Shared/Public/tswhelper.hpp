#pragma once

namespace TSWShared
{
    struct lat_lon
    {
        float lat;
        float lon;
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
        Unreal::UObject* location_library_cached_ = nullptr;
        Unreal::UObject* function_library_cached_ = nullptr;
        Unreal::UFunction* cached_get_lat_long_ = nullptr;
        Unreal::UFunction* cached_get_world_date_time_ = nullptr;

    protected:
        static TSWHelper* singleton_;
        
        TSWHelper()
        {
        }
        
    public:
        TSWHelper(TSWHelper &other) = delete;
        void operator=(const TSWHelper &) = delete;
        static TSWHelper *get_instance();
        
        lat_lon get_current_position_in_game();
        tsw_date_time get_world_date_time(Unreal::UObject* world_context_object, bool local_time);
        static Unreal::AActor* get_camera_actor();
        static Unreal::UFunction* get_function_by_name(const StringType& name,
                                                       Unreal::UObject* input_object);
        static Unreal::UFunction* get_function_by_name(const StringType& name, Unreal::UClass* input_class);
        Unreal::UObject* get_kismet_library_cached();
        static Unreal::UObject* get_kismet_library();
        Unreal::UObject* get_location_library_cached();
        static Unreal::UObject* get_location_library();
        Unreal::UObject* get_function_library_cached();
        static Unreal::UObject* get_function_library();
        static Unreal::UFunction* get_function_by_name_in_chain(const StringType& name, Unreal::UClass* input_object);
        static void execute_console_command(Unreal::FString command, Unreal::UObject* player_controller,
                                            Unreal::UFunction* kismet_static_function,
                                            Unreal::UObject* kismet_lib);
        static bool is_valid_u_object(Unreal::UObject* object);
        static float calculate_distance_in_miles_lat_lon(const lat_lon p1, const lat_lon p2);

        static bool cache_functions(const Unreal::UFunction* func, const wchar_t* func_name, Unreal::UObject* func_lib);
        static tsw_date_time ue_to_tsw_date_time(const ue_datetime& ue_date_time);


        static double GetJulianDay(const ue_datetime& ue_date_time)
        {
            return static_cast<double>(1721425.5 + ue_date_time.ticks / ticks_per_day);
        }
        
    };
}
