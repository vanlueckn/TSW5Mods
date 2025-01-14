local projectName = "TSWShared"
set_runtimes("MD")
set_languages("cxx23")

target(projectName)
    add_rules("ue4ss.base")
    add_rules("mode.release")
    set_kind("static")
    add_deps("UE4SS")
    add_headerfiles("Public/*.hpp")
    add_includedirs("Public",{public=true})
    add_files("Private/*.cpp")
    add_syslinks("winhttp")