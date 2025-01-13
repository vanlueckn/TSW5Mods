#include <DynamicOutput/DynamicOutput.hpp>
#include <Mod/CppUserModBase.hpp>
#include <core/SkCanvas.h>
#include <core/SkGraphics.h>
#include <core/SkSurface.h>
#include <core/SkBitmap.h>


using namespace RC;
using namespace RC::Unreal;

class EbulaDisplayMod final : public CppUserModBase
{
private:
    bool initialized_ = false;
public:
    EbulaDisplayMod() : CppUserModBase()
    {
        ModName = STR("EbulaDisplayMod");
        
        ModVersion = STR("1.0");
        ModDescription = STR("Ebula Display Mod");
        ModAuthors = STR("ClutchFred");
        // Do not change this unless you want to target a UE4SS version
        // other than the one you're currently building with somehow.
        // ModIntendedSDKVersion = STR("2.6");

        Output::send<LogLevel::Verbose>(STR("EbulaDisplayMod Code running\n"));

        SkImageInfo info = SkImageInfo::MakeN32Premul(100, 100);
        sk_sp<SkSurface> surface = SkSurface::makeSurface(info);

        SkCanvas* canvas = surface->getCanvas();

        canvas->clear(SK_ColorGREEN);

        SkPaint paint;
        paint.setColor(SK_ColorRED);
        paint.setAntiAlias(true);

        canvas->drawCircle(50, 50, 40, paint);

        SkBitmap bitmap;
        surface->readPixels(info, bitmap.getPixels(), info.minRowBytes(), 0, 0);
        
        
    }

    ~EbulaDisplayMod() override
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

#define EBULA_DISPLAY_MOD_API __declspec(dllexport)

extern "C" {
    EBULA_DISPLAY_MOD_API CppUserModBase* start_mod()
    {
        return new EbulaDisplayMod();
    }

    EBULA_DISPLAY_MOD_API void uninstall_mod(CppUserModBase* mod) { delete mod; }
}
