#ifndef _OSMAND_CORE_IN_AREA_SEARCH_ENGINE_P_H_
#define _OSMAND_CORE_IN_AREA_SEARCH_ENGINE_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QList>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "BaseSearchEngine.h"
#include "BaseSearchEngine_P.h"

namespace OsmAnd
{
    class InAreaSearchEngine;
    class InAreaSearchEngine_P Q_DECL_FINAL : public BaseSearchEngine_P
    {
        Q_DISABLE_COPY_AND_MOVE(InAreaSearchEngine_P);

    private:
    protected:
        InAreaSearchEngine_P(InAreaSearchEngine* const owner);
    public:
        virtual ~InAreaSearchEngine_P();

        ImplementationInterface<InAreaSearchEngine> owner;

    friend class OsmAnd::InAreaSearchEngine;
    };
}

#endif // !defined(_OSMAND_CORE_IN_AREA_SEARCH_ENGINE_P_H_)
