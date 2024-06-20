#ifndef _OSMAND_CORE_MODEL_3D_MAP_SYMBOL_H_
#define _OSMAND_CORE_MODEL_3D_MAP_SYMBOL_H_

#include "OnSurfaceVectorMapSymbol.h"

namespace OsmAnd
{
    class OSMAND_CORE_API Model3DMapSymbol : public OnSurfaceVectorMapSymbol
    {
        Q_DISABLE_COPY_AND_MOVE(Model3DMapSymbol);

    private:
    protected:
    public:
        Model3DMapSymbol(const std::shared_ptr<MapSymbolsGroup>& group);
        virtual ~Model3DMapSymbol();

        Model3D::BBox bbox;
        int maxSizeInPixels;
        FColorARGB mainColor;
    };
}

#endif // !defined(_OSMAND_CORE_MODEL_3D_MAP_SYMBOL_H_)