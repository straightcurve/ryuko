local enable_exceptions = true

set_toolset("cxx", "clang")
set_toolset("ld", "clang++")

add_rules("plugin.compile_commands.autoupdate", { outputdir = "." })
add_rules("mode.debug", "mode.release")

target("ryuko")
set_kind("static")

add_cxxflags("-std=c++20", "-msse2", "-fPIC")

if enable_exceptions then
    add_cxxflags("-fexceptions")
    add_defines("USE_EXCEPTIONS")
end

if is_mode("debug") then
    add_cxxflags("-g", "-DDEBUG", "-Werror=return-type")
end

set_pcxxheader("src/ryuko/pch.hpp")
add_includedirs("src")
add_files("src/ryuko/**.cpp")
