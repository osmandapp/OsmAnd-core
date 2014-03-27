#ifndef _OSMAND_CORE_LAMBDA_QUERY_CONTROLLER_H_
#define _OSMAND_CORE_LAMBDA_QUERY_CONTROLLER_H_

#include <OsmAndCore/stdlib_common.h>
#include <functional>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/IQueryController.h>

namespace OsmAnd
{
    class OSMAND_CORE_API LambdaQueryController : public IQueryController
    {
        Q_DISABLE_COPY(LambdaQueryController);
    public:
        typedef std::function<bool ()> Signature;
    private:
    protected:
        const Signature _function;
    public:
        LambdaQueryController(Signature function);
        virtual ~LambdaQueryController();

        virtual bool isAborted() const;
    };
}

#endif // !defined(_OSMAND_CORE_LAMBDA_QUERY_CONTROLLER_H_)
