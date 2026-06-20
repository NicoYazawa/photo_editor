#pragma once

#include <glad/glad.h>

#include <memory>
#include <vector>

class FBO;
class FilterBase;
class FullscreenQuad;
class Shader;

class FilterPipeline
{
public:
    FilterPipeline();
    ~FilterPipeline();

    FilterPipeline(const FilterPipeline&) = delete;
    FilterPipeline& operator=(const FilterPipeline&) = delete;

    // Execute filters on input texture; returns the result texture ID
    GLuint apply(GLuint inputTexture,
                 int width, int height,
                 const std::vector<FilterBase*>& filters);

    // Get the current result texture ID
    GLuint resultTexture() const { return m_currentResult; }

private:
    void ensureFBOs(int width, int height);
    GLuint singlePass(GLuint srcTexture, FilterBase* filter,
                      int width, int height, int pass);

    std::unique_ptr<FBO> m_fboA;
    std::unique_ptr<FBO> m_fboB;
    std::unique_ptr<FullscreenQuad> m_quad;
    std::unique_ptr<Shader> m_passthroughShader;

    GLuint m_currentResult = 0;
    int m_fboWidth = 0;
    int m_fboHeight = 0;
};
