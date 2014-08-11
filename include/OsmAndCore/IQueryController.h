#ifndef _OSMAND_CORE_I_QUERY_CONTROLLER_H_
#define _OSMAND_CORE_I_QUERY_CONTROLLER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>

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
}

#endif // !defined(_OSMAND_CORE_I_QUERY_CONTROLLER_H_)
