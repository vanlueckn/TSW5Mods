#include <Unreal/UEngine.hpp>
#include <Unreal/UClass.hpp>
#include <Helpers/Casting.hpp>

namespace RC::Unreal
{
    IMPLEMENT_EXTERNAL_OBJECT_CLASS(UEngine);

    Function<UEngine::LoadMapSignature> UEngine::LoadMapInternal;
    std::unordered_map<File::StringType, uint32_t> UEngine::VTableLayoutMap;
}