# CLAUDE.md — Photo Editor

C++ desktop image processing application. ImGui docking UI, GLSL shader-based filters, OpenGL 4.6 rendering.

## Build

```bash
xmake build -y        # auto-detect MSVC 2022
xmake run -y          # build + launch
xmake f -p windows -c # reconfigure (if toolchain changes)
```

Requirements: MSVC 2022, xmake 2.9+. All other deps (glfw, glad, imgui, stb) are vendored or xrepo-cached.

## Project architecture

```
main.cpp → App::run() → GLContext::beginFrame → UIManager::render → GLContext::endFrame
                            │                         │
                            │                     ┌───┴───┐
                            │                   ImagePanel │ ImagePanel
                            │                   (Original) │ (Result)
                            │                       │         │
                            │                  Filter Config panel
                            │
                       App owns: Texture (original + result)
                                 Filter chain (std::vector<FilterBase>)
                                 Undo/redo history
```

### Layer map

Each module uses `include/` for headers and `src/` for sources:

| Layer | Dir | Role |
|---|---|---|
| Entry | `src/main.cpp` | Creates App, calls run |
| Application | `src/app.cpp/.h` | State, filter orchestration, undo/redo |
| UI | `src/ui/{include,src}/` | ImGui DockSpace layout, ImagePanel, menus |
| Processing | `src/processing/{include,src}/` | FilterBase interface, registry, FBO ping-pong pipeline |
| Filters | `src/processing/{include,src}/filters/` | Concrete GLSL shader filter implementations |
| Rendering | `src/rendering/{include,src}/` | GLContext, Texture, Shader, FBO, FullscreenQuad |
| Utils | `src/utils/{include,src}/` | stb_image I/O, Windows file dialog |
| Vendored | `vendor/` | ImGui docking src, glad, stb headers |

Include paths (from `xmake.lua`): `src`, `src/*/include` — all project headers accessible by bare name or sub-path (e.g. `#include "filter_base.h"`, `#include "filters/color_filters.h"`).

### Data flow: applying a filter

1. User clicks Filter menu → `App::applyFilter(key)`
2. Push current result to undo history (FBO readback → pixel buffer)
3. Clear previous filter chain, add new single filter
4. `FilterPipeline::apply()`: for each filter, bind FBO, compile/cache shader, draw fullscreen quad with fragment shader
5. `glCopyImageSubData` GPU-to-GPU copy from pipeline output to `m_resultTexture`
6. UI updates: `m_uiManager->setResultTexture()`, `showFilterConfig()`

### Key design decisions

- **Single-filter mode**: each new filter replaces the previous one, always applied to original image (not stacked chain)
- **GLSL shaders embedded in C++**: each filter cpp has its fragment shader as a raw string literal; compiled shaders cached globally by filter key
- **FBO ping-pong**: two FBOs alternate as render targets for multi-pass filters (e.g. separable Gaussian blur)
- **No INI persistence**: `io.IniFilename = nullptr`; DockBuilder sets layout fresh each launch
- **Dock layout**: `setupDockSpace()` runs DockBuilder BEFORE `ImGui::DockSpace()` (critical ordering)

## Adding a new filter

1. Create header in `src/processing/include/filters/` extending `FilterBase` (see existing for pattern)
2. Create source in `src/processing/src/filters/` with the implementation
3. Implement: `name()`, `category()`, `key()`, `fragmentShaderSource()`, `clone()`
4. Optional: `hasConfigUI()` + `renderConfigUI()` for parameter sliders; `setUniforms(Shader&)` to pass values to shader
5. Register in `App::App()` (app.cpp): `reg.registerFilter("category.key", []{ return std::make_unique<MyFilter>(); });`
6. Add `#include "filters/my_filter.h"` — cpp file picked up by `add_files("src/**.cpp")` automatically

## Filter categories (for menu grouping)

Defined by `category()` return value. Current: `"Color"`, `"Blur"`, `"Edge Detect"`, `"Enhance"`. Menu auto-generates from registry.

## Common issues

- **`glGetTexImage` not available**: core profile 4.6 removed it. Use `readTexturePixels()` (FBO + `glReadPixels`) or `glCopyImageSubData`.
- **DockBuilder not applying**: must run BEFORE `ImGui::DockSpace()`.
- **Shader compile fails**: check `glGetShaderInfoLog` output (logged to stderr). Ensure `#version 460 core`.
- **APIENTRY warning (C4005)**: glad defines it, then Windows SDK redefines it. Harmless, can ignore.
