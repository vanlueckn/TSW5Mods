local projectName = "TSWWeatherMod"

target(projectName)
    add_rules("ue4ss.mod")
    add_includedirs(".")
    add_files("dllmain.cpp", "restclient.cpp")
    add_syslinks("winhttp")