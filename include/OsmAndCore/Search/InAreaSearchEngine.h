#ifndef _OSMAND_CORE_IN_AREA_SEARCH_ENGINE_H_
#define _OSMAND_CORE_IN_AREA_SEARCH_ENGINE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QList>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/Search/BaseSearchEngine.h>

namespace OsmAnd
{
    class ISearchSession;

    class InAreaSearchEngine_P;
    class OSMAND_CORE_API InAreaSearchEngine Q_DECL_FINAL : public BaseSearchEngine
    {
        Q_DISABLE_COPY_AND_MOVE(InAreaSearchEngine);

    private:
        PrivateImplementation<InAreaSearchEngine_P> _p;
    protected:
    public:
        InAreaSearchEngine();
        virtual ~InAreaSearchEngine();

        virtual std::shared_ptr<ISearchSession> createSession() const;
    };
}

#endif // !defined(_OSMAND_CORE_IN_AREA_SEARCH_ENGINE_H_)
