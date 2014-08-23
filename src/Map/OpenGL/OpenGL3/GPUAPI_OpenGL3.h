#ifndef _OSMAND_CORE_GPU_API_OPENGL3_H_
#define _OSMAND_CORE_GPU_API_OPENGL3_H_

#include "stdlib_common.h"
#include <array>

#include "QtExtensions.h"
#include <QString>
#include <QStringList>
#include <QMutex>

#include <glm/glm.hpp>

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "OpenGL/GPUAPI_OpenGL.h"

namespace OsmAnd 
{
    class GPUAPI_OpenGL3 : public GPUAPI_OpenGL
    {
        Q_DISABLE_COPY_AND_MOVE(GPUAPI_OpenGL3);
    private:
        void preprocessShader(QString& code);

        bool _isSupported_GREMEDY_string_marker;
        bool _isSupported_samplerObjects;
        bool _isSupported_textureStorage2D;

        std::array< GLuint, SamplerTypesCount > _textureSamplers;
        QHash<GLenum, SamplerType> _textureBlocksSamplers;

        // Groups emulation for gDEBugger
        QStringList _gdebuggerGroupsStack;
        QMutex _gdebuggerGroupsStackMutex;

        virtual void glPushGroupMarkerEXT_wrapper(GLsizei length, const GLchar* marker);
        virtual void glPopGroupMarkerEXT_wrapper();
    protected:
    public:
        GPUAPI_OpenGL3();
        virtual ~GPUAPI_OpenGL3();

        virtual bool initialize();
        virtual bool release();

        const bool& isSupported_GREMEDY_string_marker;
        const bool& isSupported_samplerObjects;
        const bool& isSupported_textureStorage2D;

        virtual GLenum validateResult();

        virtual TextureFormat getTextureFormat(const std::shared_ptr< const MapTiledData >& tile);
        virtual TextureFormat getTextureFormat(const std::shared_ptr< const RasterMapSymbol >& symbol);
        virtual SourceFormat getSourceFormat(const std::shared_ptr< const MapTiledData >& tile);
        virtual SourceFormat getSourceFormat(const std::shared_ptr< const RasterMapSymbol >& symbol);
        virtual void allocateTexture2D(GLenum target, GLsizei levels, GLsizei width, GLsizei height, const TextureFormat format);
        virtual void uploadDataToTexture2D(GLenum target, GLint level,
            GLint xoffset, GLint yoffset, GLsizei width, GLsizei height,
            const GLvoid *data, GLsizei dataRowLengthInElements, GLsizei elementSize,
            const SourceFormat sourceFormat);
        virtual void setMipMapLevelsLimit(GLenum target, const uint32_t mipmapLevelsCount);

        virtual void glGenVertexArrays_wrapper(GLsizei n, GLuint* arrays);
        virtual void glBindVertexArray_wrapper(GLuint array);
        virtual void glDeleteVertexArrays_wrapper(GLsizei n, const GLuint* arrays);

        virtual void preprocessVertexShader(QString& code);
        virtual void preprocessFragmentShader(QString& code);
        virtual void optimizeVertexShader(QString& code);
        virtual void optimizeFragmentShader(QString& code);

        virtual void setTextureBlockSampler(const GLenum textureBlock, const SamplerType samplerType);
        virtual void applyTextureBlockToTexture(const GLenum texture, const GLenum textureBlock);

        virtual void pushDebugGroupMarker(const QString& title);
        virtual void popDebugGroupMarker();
    };
}

#endif // !defined(_OSMAND_CORE_GPU_API_OPENGL3_H_)
