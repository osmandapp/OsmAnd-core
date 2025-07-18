#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_MODEL_3D_OPENGL_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_MODEL_3D_OPENGL_H_

#include "AtlasMapRendererSymbolsStage_OpenGL.h"
#include "AtlasMapRendererSymbolsStageModel3D.h"

namespace OsmAnd
{
    class AtlasMapRendererSymbolsStageModel3D_OpenGL : public AtlasMapRendererSymbolsStageModel3D
    {
    private:
        OsmAnd::GPUAPI_OpenGL* getGPUAPI() const;
        OsmAnd::AtlasMapRenderer_OpenGL* getRenderer() const;
        OsmAnd::AtlasMapRendererSymbolsStage_OpenGL* getSymbolsStage() const;

    protected:
        struct Model3DProgram
        {
            GLname id;
            QByteArray binaryCache;
            GLenum cacheFormat;

            // Vertex data
            struct
            {
                // Input data
                struct
                {
                    GLlocation vertexPosition;
                    GLlocation vertexNormal;
                    GLlocation vertexColor;
                } in;

                // Params
                struct
                {
                    // Per-model data
                    GLlocation mModel;
                    GLlocation mainColor;

                    // Common data
                    GLlocation mPerspectiveProjectionView;
                    GLlocation resultScale;
                } param;
            } vs;
            // Vertex data
            struct
            {
                // Params
                struct
                {
                    // Common data
                    GLlocation cameraPosition;
                } param;
            } fs;
        } _program;

        // Fullscreen quad program for compositing
        struct QuadProgram
        {
            GLname id;
            QByteArray binaryCache;
            GLenum cacheFormat;

            // Vertex data
            struct
            {
                // Input data
                struct
                {
                    GLlocation vertexPosition;
                    GLlocation vertexTexCoord;
                } in;
            } vs;
            // Fragment data
            struct
            {
                // Params
                struct
                {
                    GLlocation texture;
                } param;
            } fs;
        } _quadProgram;

        // Fullscreen quad VAO/VBO
        GLuint _quadVAO;
        GLuint _quadVBO;

        struct MultisampleFramebuffer
        {
            GLuint framebuffer;
            GLuint colorRenderbuffer;
            GLuint depthRenderbuffer;
            GLuint resolveTexture;
            bool initialized;
            int width;
            int height;
            int samples;
        } _multisampleFramebuffer;

    public:
        AtlasMapRendererSymbolsStageModel3D_OpenGL(AtlasMapRendererSymbolsStage_OpenGL* const symbolsStage);
        virtual ~AtlasMapRendererSymbolsStageModel3D_OpenGL();

        virtual bool initialize() override;
        virtual bool render(const std::shared_ptr<const RenderableModel3DSymbol>& renderable, AlphaChannelType& currentAlphaChannelType) override;
        virtual bool release(const bool gpuContextLost) override;

    private:
        bool createMultisampleFramebuffer(int width, int height);
        void releaseMultisampleFramebuffer();
        bool resizeMultisampleFramebuffer(int width, int height);
        bool renderModel3D(const std::shared_ptr<const RenderableModel3DSymbol>& renderable, AlphaChannelType& currentAlphaChannelType);
        bool renderToMultisampleFramebuffer(const std::shared_ptr<const RenderableModel3DSymbol>& renderable, AlphaChannelType& currentAlphaChannelType);
        void resolveMultisampleFramebuffer();
        void compositeResolvedTexture();
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_SYMBOLS_STAGE_MODEL_3D_OPENGL_H_)
