#ifndef _OSMAND_CORE_I_QUERY_CONTROLLER_H_
#define _OSMAND_CORE_I_QUERY_CONTROLLER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonSWIG.h>

namespace OsmAnd
{
    class OSMAND_CORE_API IQueryController
    {
        Q_DISABLE_COPY_AND_MOVE(IQueryController);
    private:
    protected:
        IQueryController();
    public:
        virtual ~IQueryController();

        virtual bool isAborted() const = 0;
    };

    SWIG_EMIT_DIRECTOR_BEGIN(IQueryController);
        SWIG_EMIT_DIRECTOR_CONST_METHOD_NO_ARGS(
            bool,
            isAborted);
    SWIG_EMIT_DIRECTOR_END(IQueryController);
}

#endif // !defined(_OSMAND_CORE_I_QUERY_CONTROLLER_H_)
