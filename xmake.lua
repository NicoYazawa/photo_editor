set_project("PhotoEditor")
set_version("0.1.0")
set_languages("c++17")
add_rules("mode.debug", "mode.release")

-- Only need glfw from xrepo (already cached)
add_requires("glfw")

target("photo_editor")
    set_kind("binary")

    -- App sources
    add_files("src/**.cpp")

    -- Vendor: ImGui (docking branch)
    add_includedirs("vendor/imgui")
    add_includedirs("vendor/imgui/backends")
    add_includedirs("vendor/imgui/misc/cpp")
    add_files("vendor/imgui/imgui.cpp")
    add_files("vendor/imgui/imgui_draw.cpp")
    add_files("vendor/imgui/imgui_tables.cpp")
    add_files("vendor/imgui/imgui_widgets.cpp")
    add_files("vendor/imgui/backends/imgui_impl_glfw.cpp")
    add_files("vendor/imgui/backends/imgui_impl_opengl3.cpp")
    add_files("vendor/imgui/misc/cpp/imgui_stdlib.cpp")

    -- Vendor: glad (pre-generated)
    add_includedirs("vendor/glad/include")
    add_files("vendor/glad/src/glad.c")

    -- Vendor: stb_image (header-only)
    add_includedirs("vendor")

    -- Packages
    add_packages("glfw")

    -- Defines
    add_defines("GLFW_INCLUDE_NONE")
    add_defines("IMGUI_IMPL_OPENGL_LOADER_GLAD")
    add_defines("WIN32_LEAN_AND_MEAN")
    add_defines("NOMINMAX")

    if is_mode("debug") then
        set_symbols("debug")
        set_optimize("none")
    else
        set_symbols("hidden")
        set_optimize("fastest")
    end
