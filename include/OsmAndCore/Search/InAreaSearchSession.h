#ifndef _OSMAND_CORE_IN_AREA_SEARCH_SESSION_H_
#define _OSMAND_CORE_IN_AREA_SEARCH_SESSION_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <QLinkedList>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Callable.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Search/BaseSearchSession.h>

namespace OsmAnd
{
    class InAreaSearchEngine;

    class InAreaSearchSession_P;
    class OSMAND_CORE_API InAreaSearchSession Q_DECL_FINAL : public BaseSearchSession
    {
        Q_DISABLE_COPY_AND_MOVE(InAreaSearchSession);

    private:
        PrivateImplementation<InAreaSearchSession_P> _p;
    protected:
        InAreaSearchSession(const QList< std::shared_ptr<const ISearchEngine::IDataSource> >& dataSources);
    public:
        virtual ~InAreaSearchSession();

        void setArea(const AreaI64& area);
        AreaI64 getArea() const;

    friend class OsmAnd::InAreaSearchEngine;
    };
}

#endif // !defined(_OSMAND_CORE_IN_AREA_SEARCH_SESSION_H_)
