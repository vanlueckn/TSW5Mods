#include <DynamicOutput/DynamicOutput.hpp>
#include <Mod/CppUserModBase.hpp>

using namespace RC;
using namespace RC::Unreal;

class EbulaWebsocketMod final : public CppUserModBase
{
private:
    bool initialized_ = false;
public:
    EbulaWebsocketMod() : CppUserModBase()
    {
        ModName = STR("EbulaWebsocketMod");
        ModVersion = STR("1.0");
        ModDescription = STR("Ebula Websocket Mod");
        ModAuthors = STR("Luex");
        // Do not change this unless you want to target a UE4SS version
        // other than the one you're currently building with somehow.
        // ModIntendedSDKVersion = STR("2.6");

        Output::send<LogLevel::Verbose>(STR("EbulaWebsocketMod Code running\n"));
    }

    ~EbulaWebsocketMod() override
    {
    }

    auto on_update() -> void override
    {
        if (!initialized_) return;
    }

    auto on_unreal_init() -> void override
    {
        initialized_ = true;
    }
};

#define EBULA_WEBSOCKET_API __declspec(dllexport)

extern "C" {
EBULA_WEBSOCKET_API CppUserModBase* start_mod()
{
    return new EbulaWebsocketMod();
}

EBULA_WEBSOCKET_API void uninstall_mod(CppUserModBase* mod) { delete mod; }
}
