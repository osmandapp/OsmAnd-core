#ifndef _OSMAND_CORE_QT_EXTENSIONS_H_
#define _OSMAND_CORE_QT_EXTENSIONS_H_

#include <memory>

#ifndef SWIG
#   if defined(QGLOBAL_H)
#       error <OsmAndCore/QtExtensions.h> must be included before any Qt header
#   endif // QGLOBAL_H
#endif // !SWIG

#include <QtGlobal>

#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/DataTypes.h>
#include <OsmAndCore/Map/MapTypes.h>

#include <QTypeInfo>
#ifndef SWIG
Q_DECLARE_TYPEINFO(OsmAnd::PointI, Q_PRIMITIVE_TYPE | Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(OsmAnd::PointI64, Q_PRIMITIVE_TYPE | Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(OsmAnd::PointF, Q_PRIMITIVE_TYPE | Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(OsmAnd::PointD, Q_PRIMITIVE_TYPE | Q_MOVABLE_TYPE);

Q_DECLARE_TYPEINFO(OsmAnd::TileId, Q_PRIMITIVE_TYPE | Q_PRIMITIVE_TYPE);

Q_DECLARE_TYPEINFO(OsmAnd::FColorRGB, Q_PRIMITIVE_TYPE | Q_MOVABLE_TYPE);

static_assert(QTypeInfo<OsmAnd::PointI>::isComplex == 0, "QTypeInfo<OsmAnd::PointI>::isComplex must be 0");
static_assert(QTypeInfo<OsmAnd::PointI64>::isComplex == 0, "QTypeInfo<OsmAnd::PointI64>::isComplex must be 0");
static_assert(QTypeInfo<OsmAnd::PointF>::isComplex == 0, "QTypeInfo<OsmAnd::PointF>::isComplex must be 0");
static_assert(QTypeInfo<OsmAnd::PointD>::isComplex == 0, "QTypeInfo<OsmAnd::PointD>::isComplex must be 0");

static_assert(QTypeInfo<OsmAnd::TileId>::isComplex == 0, "QTypeInfo<OsmAnd::TileId>::isComplex must be 0");

static_assert(QTypeInfo<OsmAnd::FColorRGB>::isComplex == 0, "QTypeInfo<OsmAnd::FColorRGB>::isComplex must be 0");
#endif //!SWIG

#ifndef SWIG
inline uint qHash(const OsmAnd::TileId value, uint seed = 0) Q_DECL_NOTHROW;

template<typename T>
inline uint qHash(const std::shared_ptr<T>& value, uint seed = 0) Q_DECL_NOTHROW;

inline uint qHash(const OsmAnd::ObfAddressBlockType value, uint seed = 0) Q_DECL_NOTHROW;
#endif //!SWIG

#include <QHash>

#ifndef SWIG
inline uint qHash(const OsmAnd::TileId value, uint seed) Q_DECL_NOTHROW
{
    return ::qHash(value.id, seed);
}

template<typename T>
inline uint qHash(const std::shared_ptr<T>& value, uint seed) Q_DECL_NOTHROW
{
    return ::qHash(value.get(), seed);
}

inline uint qHash(const OsmAnd::ObfAddressBlockType value, uint seed) Q_DECL_NOTHROW
{
    return ::qHash(static_cast<int>(value), seed);
}
#endif //!SWIG

#endif // !defined(_OSMAND_CORE_QT_EXTENSIONS_H_)
