#ifndef _OSMAND_CORE_IN_AREA_SEARCH_SESSION_P_H_
#define _OSMAND_CORE_IN_AREA_SEARCH_SESSION_P_H_

#include "stdlib_common.h"

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QList>
#include <QLinkedList>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "Callable.h"
#include "PrivateImplementation.h"
#include "BaseSearchSession_P.h"

namespace OsmAnd
{
    class InAreaSearchSession;
    class InAreaSearchSession_P Q_DECL_FINAL : public BaseSearchSession_P
    {
        Q_DISABLE_COPY_AND_MOVE(InAreaSearchSession_P);

    private:
    protected:
        InAreaSearchSession_P(InAreaSearchSession* const owner);
    public:
        virtual ~InAreaSearchSession_P();

        ImplementationInterface<InAreaSearchSession> owner;

        void setArea(const AreaI64& area);
        AreaI64 getArea() const;

    friend class OsmAnd::InAreaSearchSession;
    };
}

#endif // !defined(_OSMAND_CORE_IN_AREA_SEARCH_SESSION_P_H_)
