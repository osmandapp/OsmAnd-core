#ifndef _OSMAND_CORE_ATLAS_MAP_RENDERER_OPENGL_H_
#define _OSMAND_CORE_ATLAS_MAP_RENDERER_OPENGL_H_

#include "stdlib_common.h"

#include "QtExtensions.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "OpenGL_Common/AtlasMapRenderer_OpenGL_Common.h"

namespace OsmAnd
{
    class GPUAPI;

    class AtlasMapRenderer_OpenGL : public AtlasMapRenderer_OpenGL_Common
    {
        Q_DISABLE_COPY(AtlasMapRenderer_OpenGL);
    private:
    protected:
        virtual GPUAPI* allocateGPUAPI();
    public:
        AtlasMapRenderer_OpenGL();
        virtual ~AtlasMapRenderer_OpenGL();
    };

}

#endif // !defined(_OSMAND_CORE_ATLAS_MAP_RENDERER_OPENGL_H_)
