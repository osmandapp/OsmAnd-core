#ifndef _OSMAND_CORE_FUNCTOR_QUERY_CONTROLLER_H_
#define _OSMAND_CORE_FUNCTOR_QUERY_CONTROLLER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/IQueryController.h>

namespace OsmAnd
{
    class OSMAND_CORE_API FunctorQueryController : public IQueryController
    {
        Q_DISABLE_COPY_AND_MOVE(FunctorQueryController);

    public:
        OSMAND_CALLABLE(Callback, bool, const FunctorQueryController* queryController);

    private:
    protected:
        const Callback _callback;
    public:
        FunctorQueryController(const Callback callback);
        virtual ~FunctorQueryController();

        virtual bool isAborted() const Q_DECL_OVERRIDE;
    };
}

#endif // !defined(_OSMAND_CORE_FUNCTOR_QUERY_CONTROLLER_H_)
