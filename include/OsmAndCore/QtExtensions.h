#ifndef _OSMAND_CORE_QT_EXTENSIONS_H_
#define _OSMAND_CORE_QT_EXTENSIONS_H_

#include <memory>

#if !defined(SWIG)
#   if defined(QGLOBAL_H)
#       error <OsmAndCore/QtExtensions.h> must be included before any Qt header
#   endif // QGLOBAL_H
#endif // !defined(SWIG)

#include <QtGlobal>

#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/Data/DataTypes.h>
#include <OsmAndCore/Map/MapTypes.h>

#include <QTypeInfo>
#if !defined(SWIG)
Q_DECLARE_TYPEINFO(OsmAnd::PointI, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(OsmAnd::PointI64, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(OsmAnd::PointF, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(OsmAnd::PointD, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(OsmAnd::AreaI, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(OsmAnd::AreaI64, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(OsmAnd::AreaF, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(OsmAnd::AreaD, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(OsmAnd::OOBBI, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(OsmAnd::OOBBI64, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(OsmAnd::OOBBF, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(OsmAnd::OOBBD, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(OsmAnd::TileId, Q_PRIMITIVE_TYPE | Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(OsmAnd::FColorRGB, Q_PRIMITIVE_TYPE | Q_MOVABLE_TYPE);
#endif // !defined(SWIG)

#if !defined(SWIG)
inline uint qHash(const OsmAnd::TileId value, uint seed = 0) Q_DECL_NOTHROW;

template<typename T>
inline uint qHash(const std::shared_ptr<T>& value, uint seed = 0) Q_DECL_NOTHROW;

inline uint qHash(const OsmAnd::ObfAddressBlockType value, uint seed = 0) Q_DECL_NOTHROW;
#endif // !defined(SWIG)

#include <QHash>

#if !defined(SWIG)
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
#endif // !defined(SWIG)

#endif // !defined(_OSMAND_CORE_QT_EXTENSIONS_H_)
