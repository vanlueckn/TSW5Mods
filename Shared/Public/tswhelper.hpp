#pragma once

namespace TSWShared
{
    struct lat_lon
    {
        float lat;
        float lon;
    };

    class TSWHelper
    {
        Unreal::UObject* player_controller_cached_ = nullptr;
        Unreal::UObject* location_library_cached_ = nullptr;
        Unreal::UFunction* cached_get_lat_long_ = nullptr;

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
        static Unreal::AActor* get_camera_actor();
        static Unreal::UFunction* get_function_by_name(const StringType& name,
                                                       Unreal::UObject* input_object);
        static Unreal::UFunction* get_function_by_name(const StringType& name, Unreal::UClass* input_class);
        Unreal::UObject* get_kismet_library_cached();
        static Unreal::UObject* get_kismet_library();
        Unreal::UObject* get_location_library_cached();
        static Unreal::UObject* get_location_library();
        static Unreal::UFunction* get_function_by_name_in_chain(const StringType& name, Unreal::UClass* input_object);
        static void execute_console_command(Unreal::FString command, Unreal::UObject* player_controller,
                                            Unreal::UFunction* kismet_static_function,
                                            Unreal::UObject* kismet_lib);
        static bool is_valid_u_object(Unreal::UObject* object);
        static float calculate_distance_in_miles_lat_lon(const lat_lon p1, const lat_lon p2);
    };
}
