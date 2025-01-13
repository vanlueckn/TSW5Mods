local projectName = "TSWShared"
set_runtimes("MD")

target(projectName)
    set_kind("static")
    add_headerfiles("Public/*.hpp")
    add_includedirs("Public",{public=true})
    add_files("Private/*.cpp")
    add_syslinks("winhttp")