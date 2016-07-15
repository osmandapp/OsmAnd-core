#ifndef _OSMAND_CORE_SIMPLE_QUERY_CONTROLLER_H_
#define _OSMAND_CORE_SIMPLE_QUERY_CONTROLLER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QAtomicInt>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/IQueryController.h>

namespace OsmAnd
{
    class OSMAND_CORE_API SimpleQueryController : public IQueryController
    {
        Q_DISABLE_COPY_AND_MOVE(SimpleQueryController)

    private:
    protected:
        QAtomicInt _isAborted;
    public:
        SimpleQueryController();
        virtual ~SimpleQueryController();

        void abort();
        virtual bool isAborted() const Q_DECL_OVERRIDE;
    };
}

#endif // !defined(_OSMAND_CORE_SIMPLE_QUERY_CONTROLLER_H_)
