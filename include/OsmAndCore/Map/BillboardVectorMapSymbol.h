#ifndef _OSMAND_CORE_BILLBOARD_VECTOR_MAP_SYMBOL_H_
#define _OSMAND_CORE_BILLBOARD_VECTOR_MAP_SYMBOL_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Map/VectorMapSymbol.h>
#include <OsmAndCore/Map/IBillboardMapSymbol.h>

namespace OsmAnd
{
    class OSMAND_CORE_API BillboardVectorMapSymbol
        : public VectorMapSymbol
        , public IBillboardMapSymbol
    {
        Q_DISABLE_COPY_AND_MOVE(BillboardVectorMapSymbol);

    private:
    protected:
    public:
        BillboardVectorMapSymbol(
            const std::shared_ptr<MapSymbolsGroup>& group);
        virtual ~BillboardVectorMapSymbol();

        PointI offset;
        virtual PointI getOffset() const;
        virtual void setOffset(const PointI offset);

        PointI position31;
        virtual PointI getPosition31() const;
        virtual void setPosition31(const PointI position);
    };
}

#endif // !defined(_OSMAND_CORE_BILLBOARD_VECTOR_MAP_SYMBOL_H_)
