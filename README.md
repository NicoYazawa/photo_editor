# Photo Editor

基于 C++ / ImGui / OpenGL 4.6 的桌面图像处理编辑器。所有滤镜使用 GLSL fragment shader 在 GPU 端实时处理。

## 特性

- **10 个内置滤镜**: 灰度、反色、亮度/对比度、色彩平衡、阈值、高斯模糊、均值模糊、Sobel 边缘检测、Laplacian、锐化
- **实时参数调节**: 带参数的滤镜通过滑块即时预览效果
- **撤销/重做**: 支持 20 步历史
- **DockSpace 布局**: 可拖拽调整的面板布局
- **GPU 加速**: 滤镜完全在 GPU 端执行，`glCopyImageSubData` 零拷贝传递结果

## 快速开始

```bash
# 构建
xmake build -y

# 运行
xmake run -y
```

**依赖**: MSVC 2022, xmake 2.9+

## 架构

```
┌─────────────────────────────────────────────────────────┐
│                        main.cpp                          │
│                          App                             │
│  ┌──────────────────────────────────────────────────┐   │
│  │                 UIManager                          │   │
│  │  ┌─────────┐  ┌──────────┐  ┌──────────┐        │   │
│  │  │  Menu   │  │ Original │  │  Result  │        │   │
│  │  │  Bar    │  │  Panel   │  │  Panel   │        │   │
│  │  └─────────┘  └──────────┘  └──────────┘        │   │
│  │  ┌──────────────────────────────────────┐       │   │
│  │  │         Filter Config Panel           │       │   │
│  │  └──────────────────────────────────────┘       │   │
│  │  ┌──────────────────────────────────────┐       │   │
│  │  │           Status Bar                  │       │   │
│  │  └──────────────────────────────────────┘       │   │
│  └──────────────────────────────────────────────────┘   │
│                          │                               │
│  ┌──────────────────────┴───────────────────────────┐   │
│  │              FilterPipeline                        │   │
│  │  ┌──────────┐   ┌──────────┐   ┌──────────────┐  │   │
│  │  │  Filter  │──→│  Shader  │──→│  FBO Ping-   │  │   │
│  │  │  Chain   │   │  Cache   │   │  Pong        │  │   │
│  │  └──────────┘   └──────────┘   └──────────────┘  │   │
│  └──────────────────────────────────────────────────┘   │
│                          │                               │
│  ┌──────────────────────┴───────────────────────────┐   │
│  │              Rendering Layer                       │   │
│  │  GLContext │ Texture │ Shader │ FBO │ FullscreenQ  │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

### 数据流

```
选择滤镜 → App::applyFilter()
              │
              ├─ pushHistory()       保存当前结果到撤销栈
              ├─ m_filters.clear()   清空旧滤镜链
              ├─ m_filters.push()    加入新滤镜
              │
              └─ executePipeline()
                   │
                   └─ FilterPipeline::apply()
                        │
                        ├─ getOrCompileShader()  ← s_shaderCache
                        ├─ FBO::bind()
                        ├─ shader->use()
                        ├─ setUniforms()          ← 滤镜参数
                        ├─ fullscreenQuad.draw()  ← GPU 渲染
                        └─ glCopyImageSubData()   ← GPU→GPU 拷贝
                             │
                        m_resultTexture ──→ UIManager 显示
```

### 文件结构

每个模块内部区分 `include/`（头文件）和 `src/`（源文件）：

```
src/
├── main.cpp                                 # 程序入口
├── app.cpp / app.h                          # 应用状态、滤镜调度、撤销/重做
├── ui/
│   ├── include/
│   │   ├── ui_manager.h                     # DockSpace 布局、菜单栏、状态栏
│   │   └── image_panel.h                    # 图像显示面板（缩放/平移）
│   └── src/
│       ├── ui_manager.cpp
│       └── image_panel.cpp
├── processing/
│   ├── include/
│   │   ├── filter_base.h                    # 滤镜抽象接口
│   │   ├── filter_registry.h                # 滤镜注册表（名称→工厂函数）
│   │   ├── filter_pipeline.h                # FBO ping-pong 执行管线
│   │   └── filters/
│   │       ├── color_filters.h              # 灰度、反色、亮度/对比度、色彩平衡
│   │       ├── blur_filters.h               # 高斯模糊（两 pass）、均值模糊
│   │       ├── edge_filters.h               # Sobel、Laplacian、锐化
│   │       └── threshold_filter.h           # 可调阈值
│   └── src/
│       ├── filter_pipeline.cpp
│       ├── filter_registry.cpp
│       └── filters/
│           ├── color_filters.cpp
│           ├── blur_filters.cpp
│           ├── edge_filters.cpp
│           └── threshold_filter.cpp
├── rendering/
│   ├── include/
│   │   ├── gl_context.h                     # GLFW 窗口 + OpenGL 上下文
│   │   ├── texture.h                        # GL 纹理 RAII 封装
│   │   ├── shader.h                         # Shader 编译/链接/Uniform
│   │   ├── fbo.h                            # FBO + 颜色附件
│   │   └── fullscreen_quad.h                # 全屏四边形 VAO
│   └── src/
│       ├── gl_context.cpp
│       ├── texture.cpp
│       ├── shader.cpp
│       ├── fbo.cpp
│       └── fullscreen_quad.cpp
└── utils/
    ├── include/
    │   └── image_io.h                       # stb_image 加载/保存
    └── src/
        └── image_io.cpp
vendor/
├── imgui/                                   # Dear ImGui (docking 分支)
├── glad/                                    # OpenGL 4.6 loader (预生成)
├── stb_image.h / stb_image_write.h          # 图像 I/O
xmake.lua                                    # 构建配置
```

---

## 新增滤镜指南

### Step 1: 创建滤镜头文件

在 `src/processing/include/filters/` 下创建 `my_filter.h`:

```cpp
#pragma once
#include "filter_base.h"

class MyFilter : public FilterBase
{
public:
    std::string name() const override { return "My Filter"; }
    std::string category() const override { return "Color"; }        // 菜单分组
    std::string key() const override { return "color.my_filter"; }   // 唯一键
    const char* fragmentShaderSource() const override;                // GLSL 源码
    std::unique_ptr<FilterBase> clone() const override;

    // 可选: 参数调节
    bool hasConfigUI() const override { return true; }
    void renderConfigUI() override;
    void setUniforms(Shader& shader) override;

    float myParam = 0.5f;
};
```

### Step 2: 实现滤镜

在 `src/processing/src/filters/` 下创建对应的 `.cpp` 文件:

```cpp
#include "filters/my_filter.h"
#include "shader.h"
#include <imgui.h>

static const char* kMyShader = R"(
#version 460 core
in vec2 vTexCoord; out vec4 FragColor;
uniform sampler2D uTexture;
uniform float uMyParam;

void main() {
    vec4 color = texture(uTexture, vTexCoord);
    // ... 算法逻辑 ...
    FragColor = color;
}
)";

const char* MyFilter::fragmentShaderSource() const { return kMyShader; }

void MyFilter::setUniforms(Shader& shader) {
    shader.setFloat("uMyParam", myParam);
}

void MyFilter::renderConfigUI() {
    ImGui::SliderFloat("My Param", &myParam, 0.0f, 1.0f);
}

std::unique_ptr<FilterBase> MyFilter::clone() const {
    return std::make_unique<MyFilter>(*this);
}
```

### Step 3: 注册滤镜

在 [src/app.cpp](src/app.cpp) 的构造函数中添加:

```cpp
reg.registerFilter("color.my_filter",
    []{ return std::make_unique<MyFilter>(); });
```

同时添加 `#include`:

```cpp
#include "filters/my_filter.h"
```

### Step 4: 构建

```bash
xmake build -y   # xmake 自动通过 src/**.cpp 通配符发现新文件
```

---

### 多 Pass 滤镜（如可分离模糊）

重写 `passCount()` 和 `setUniformsForPass()`:

```cpp
int passCount() const override { return 2; }

void setUniformsForPass(Shader& shader, int pass) override {
    shader.setInt("uHorizontal", pass == 0 ? 1 : 0);
    // pass 0 = 水平, pass 1 = 垂直
}
```

参考: [blur_filters.h](src/processing/include/filters/blur_filters.h)

### Shader 可用的内置 Uniform

| Uniform | 类型 | 说明 |
|---|---|---|
| `uTexture` | `sampler2D` | 输入纹理（自动绑定到纹理单元 0） |
| `uTexelSize` | `vec2` | 单个像素的 UV 尺寸 (1/width, 1/height) |
| 自定义 | — | 通过 `Shader::setFloat/setInt/setVec2` 设置 |

### 注意事项

- Shader 必须声明 `#version 460 core`
- 输入纹理坐标: `vTexCoord`，范围 [0, 1]
- 输出: `FragColor`
- 滤镜总是应用到原图上（非叠加模式），Pipeline 中只有一个滤镜
- `app.cpp` 中的 filter key 必须与 `key()` 返回值完全一致

## 技术栈

| 层 | 选型 | 版本 |
|---|---|---|
| 构建 | xmake | 2.9+ |
| 编译器 | MSVC | 2022 |
| UI | Dear ImGui | docking branch |
| 窗口/上下文 | GLFW | 3.4 |
| OpenGL 加载 | glad | 0.1.36 (pre-generated) |
| 图像 I/O | stb_image | latest |
| 图像处理 | GLSL shader | 460 core |

## License

MIT
