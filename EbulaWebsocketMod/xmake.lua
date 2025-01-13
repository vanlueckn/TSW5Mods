local projectName = "EbulaWebsocketMod"
add_requires("uwebsockets")

target(projectName)
    add_rules("ue4ss.mod")
    add_deps("TSWShared")
    add_headerfiles("Public/*.hpp")
    add_includedirs("Public")
    add_files("Private/*.cpp")
    add_syslinks("winhttp")
    add_packages("uwebsockets")