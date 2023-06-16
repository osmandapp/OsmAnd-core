#ifndef _OSMAND_CORE_VECTOR_LINES_COLLECTION_P_H_
#define _OSMAND_CORE_VECTOR_LINES_COLLECTION_P_H_

#include "stdlib_common.h"
#include <functional>

#include "QtExtensions.h"
#include <QHash>
#include <QList>
#include <QVector>
#include <QReadWriteLock>

#include "OsmAndCore.h"
#include "PrivateImplementation.h"
#include "CommonTypes.h"
#include "IMapKeyedSymbolsProvider.h"
#include "VectorLine.h"
#include "VectorLineBuilder.h"

namespace OsmAnd
{
    class VectorLineBuilder;
    class VectorLineBuilder_P;

    class VectorLinesCollection;
    class VectorLinesCollection_P Q_DECL_FINAL
    {
        Q_DISABLE_COPY_AND_MOVE(VectorLinesCollection_P);

    private:
    protected:
        VectorLinesCollection_P(VectorLinesCollection* const owner);

        mutable QReadWriteLock _linesLock;
        QHash< IMapKeyedSymbolsProvider::Key, std::shared_ptr<VectorLine> > _lines;

        bool addLine(const std::shared_ptr<VectorLine> line);
    public:
        virtual ~VectorLinesCollection_P();

        ImplementationInterface<VectorLinesCollection> owner;

        QList< std::shared_ptr<VectorLine> > getLines() const;
        bool removeLine(const std::shared_ptr<VectorLine>& line);
        void removeAllLines();

        QList<IMapKeyedSymbolsProvider::Key> getProvidedDataKeys() const;
        bool obtainData(
            const IMapDataProvider::Request& request,
            std::shared_ptr<IMapDataProvider::Data>& outData);

    friend class OsmAnd::VectorLinesCollection;
    friend class OsmAnd::VectorLineBuilder;
    friend class OsmAnd::VectorLineBuilder_P;
    };
}

#endif // !defined(_OSMAND_CORE_VECTOR_LINES_COLLECTION_P_H_)
