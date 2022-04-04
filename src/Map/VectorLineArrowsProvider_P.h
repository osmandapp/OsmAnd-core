#ifndef _OSMAND_CORE_VECTOR_LINE_ARROWS_PROVIDER_P_H_
#define _OSMAND_CORE_VECTOR_LINE_ARROWS_PROVIDER_P_H_

#include "stdlib_common.h"
#include <array>
#include <functional>

#include "QtExtensions.h"
#include "ignore_warnings_on_external_includes.h"
#include <QReadWriteLock>
#include <QList>
#include <QHash>
#include <QVector>
#include "restore_internal_warnings.h"

#include "OsmAndCore.h"
#include "CommonTypes.h"
#include "PrivateImplementation.h"
#include "IMapTiledSymbolsProvider.h"
#include "VectorLine.h"
#include "VectorLineArrowsProvider.h"
#include "QuadTree.h"

namespace OsmAnd
{
    class VectorLineArrowsProvider_P Q_DECL_FINAL
        : public std::enable_shared_from_this<VectorLineArrowsProvider_P>
    {
    public:
        typedef VectorLineArrowsProvider::SymbolsGroup SymbolsGroup;

    private:
        mutable QReadWriteLock _lock;
        
        QHash<std::shared_ptr<VectorLine>, int> _lineVersions;
        
        void generateArrowsOnPath(
            const AreaI& bbox31,
            const ZoomLevel& zoom,
            const std::shared_ptr<SymbolsGroup>& symbolsGroup,
            const std::shared_ptr<VectorLine>& vectorLine);

    protected:
        bool getVectorLineDataVersion(const std::shared_ptr<VectorLine>& vectorLine, int& version) const;

    protected:
        VectorLineArrowsProvider_P(VectorLineArrowsProvider* owner);

        struct RetainableCacheMetadata : public IMapDataProvider::RetainableCacheMetadata
        {
            RetainableCacheMetadata();
            virtual ~RetainableCacheMetadata();
        };

    public:
        virtual ~VectorLineArrowsProvider_P();

        ImplementationInterface<VectorLineArrowsProvider> owner;

        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData,
            std::shared_ptr<Metric>* const pOutMetric);

    friend class OsmAnd::VectorLineArrowsProvider;
    };
}

#endif // !defined(_OSMAND_CORE_VECTOR_LINE_ARROWS_PROVIDER_P_H_)
