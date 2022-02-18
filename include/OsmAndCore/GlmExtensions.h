#ifndef _OSMAND_CORE_GLM_EXTENSIONS_H_
#define _OSMAND_CORE_GLM_EXTENSIONS_H_

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <OsmAndCore/restore_internal_warnings.h>

namespace glm_extensions
{
    using namespace glm;

    template<typename T, typename U, precision P>
    GLM_FUNC_QUALIFIER tvec3<T, P> project(const tvec3<T, P>& obj, const tmat4x4<T, P>& modelProjection, const tvec4<U, P>& viewport)
    {
        tvec4<T, P> tmp = tvec4<T, P>(obj, T(1));
        tmp = modelProjection * tmp;

        tmp /= tmp.w;
        tmp = tmp * static_cast<T>(0.5) + static_cast<T>(0.5);
        tmp[0] = tmp[0] * T(viewport[2]) + T(viewport[0]);
        tmp[1] = tmp[1] * T(viewport[3]) + T(viewport[1]);

        return tvec3<T, P>(tmp);
    }
}

#endif // !defined(_OSMAND_CORE_GLM_EXTENSIONS_H_)
