#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_3D_MODELS_STAGE_OPENGL_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_3D_MODELS_STAGE_OPENGL_H_

#include "AtlasMapRenderer_OpenGL.h"
#include "AtlasMapRenderer3DModelsStage.h"
#include "AtlasMapRendererStageHelper_OpenGL.h"
#include "ObjParser.h"
#include "Model3D.h"

#include <QVector>
#include <QList>

namespace OsmAnd
{
    class AtlasMapRenderer3DModelsStage_OpenGL
        : public AtlasMapRenderer3DModelsStage
        , private AtlasMapRendererStageHelper_OpenGL
    {
    private:
        QList<float> _allZoomScales;

        bool _successfulLoadModel;
        std::shared_ptr<Model3D> _model;
    protected:
        GLname _3DModelVAO;
        GLname _3DModelVBO;
        struct ThreeDimensionalModelProgram
        {
            GLname id;

            // Vertex shader
            struct {
                // Input data
                struct
                {
                    GLlocation vertexPosition;
                    GLlocation vertexColor;
                } in;

                // Params
                struct
                {
                    GLlocation mModel;
                    GLlocation mPerspectiveProjectionView;
                } params;
            } vs;

        } _program;
    public:
        AtlasMapRenderer3DModelsStage_OpenGL(AtlasMapRenderer_OpenGL* const renderer);
        virtual ~AtlasMapRenderer3DModelsStage_OpenGL();

        virtual bool initialize();
        virtual bool render(IMapRenderer_Metrics::Metric_renderFrame* const metric);
        virtual bool release(bool gpuContextLost);
    };
}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_3D_MODELS_STAGE_OPENGL_H_)