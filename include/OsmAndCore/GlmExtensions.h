#ifndef _OSMAND_CORE_GLM_EXTENSIONS_H_
#define _OSMAND_CORE_GLM_EXTENSIONS_H_

#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <OsmAndCore/restore_internal_warnings.h>

namespace glm_extensions
{
    template <typename T, typename U, glm::precision P>
    GLM_FUNC_QUALIFIER glm::tvec3<T, P> fastProject(
        glm::tvec3<T, P> const & obj,
        glm::tmat4x4<T, P> const & modelProjection,
        glm::tvec4<U, P> const & viewport)
    {
        glm::tvec4<T, P> tmp = glm::tvec4<T, P>(obj, T(1));
        tmp = modelProjection * tmp;

        tmp /= tmp.w * T(2.0);
        tmp += T(0.5);
        tmp[0] = tmp[0] * T(viewport[2]) + T(viewport[0]);
        tmp[1] = tmp[1] * T(viewport[3]) + T(viewport[1]);

        return glm::tvec3<T, P>(tmp);
    }
}

#endif // !defined(_OSMAND_CORE_GLM_EXTENSIONS_H_)
