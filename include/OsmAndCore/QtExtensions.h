#ifndef _OSMAND_CORE_QT_EXTENSIONS_H_
#define _OSMAND_CORE_QT_EXTENSIONS_H_

#include <memory>
#include <type_traits>

#if !defined(SWIG)
#   if defined(QGLOBAL_H)
#       error <OsmAndCore/QtExtensions.h> must be included before any Qt header
#   endif // QGLOBAL_H
#endif // !defined(SWIG)

#include <QtGlobal>

#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/SmartPOD.h>
#include <OsmAndCore/Data/DataCommonTypes.h>
#include <OsmAndCore/Map/MapCommonTypes.h>

#if !defined(SWIG)
template<typename T>
inline auto qHash(
    const T& value) Q_DECL_NOTHROW -> typename std::enable_if< std::is_same<decltype(value.qHash()), uint(uint)>::value, uint>::type;

template<typename T>
inline auto qHash(
    const T& value) Q_DECL_NOTHROW -> typename std::enable_if< std::is_same<decltype(value.operator uint64_t()), uint64_t()>::value, uint>::type;

template<typename T>
inline auto qHash(
    const T& value) Q_DECL_NOTHROW -> typename std::enable_if< std::is_same<decltype(value.operator int64_t()), int64_t()>::value, uint>::type;

template<typename T>
inline uint qHash(const std::shared_ptr<T>& value) Q_DECL_NOTHROW;

template<typename T>
inline auto qHash(
    const T value) Q_DECL_NOTHROW -> typename std::enable_if< std::is_enum<T>::value && !std::is_convertible<T, int>::value, uint>::type;

template<typename T, T DEFAULT_VALUE>
inline uint qHash(const OsmAnd::SmartPOD<T, DEFAULT_VALUE>& value) Q_DECL_NOTHROW;
#endif // !defined(SWIG)

#include <QHash>

#if !defined(SWIG)
template<typename T>
inline auto qHash(
    const T& value) Q_DECL_NOTHROW -> typename std::enable_if< std::is_same<decltype(value.qHash()), uint()>::value, uint>::type
{
    return value.qHash();
}

template<typename T>
inline auto qHash(
    const T& value) Q_DECL_NOTHROW -> typename std::enable_if< std::is_same<decltype(value.operator uint64_t()), uint64_t()>::value, uint>::type
{
    return ::qHash(static_cast<uint64_t>(value));
}

template<typename T>
inline auto qHash(
    const T& value) Q_DECL_NOTHROW -> typename std::enable_if< std::is_same<decltype(value.operator int64_t()), int64_t()>::value, uint>::type
{
    return ::qHash(static_cast<int64_t>(value));
}

template<typename T>
inline uint qHash(const std::shared_ptr<T>& value) Q_DECL_NOTHROW
{
    return ::qHash(value.get());
}

template<typename T>
inline auto qHash(
    const T value) Q_DECL_NOTHROW -> typename std::enable_if< std::is_enum<T>::value && !std::is_convertible<T, int>::value, uint>::type
{
    return ::qHash(static_cast<typename std::underlying_type<T>::type>(value));
}

template<typename T, T DEFAULT_VALUE>
inline uint qHash(const OsmAnd::SmartPOD<T, DEFAULT_VALUE>& value) Q_DECL_NOTHROW
{
    return ::qHash(static_cast<T>(value));
}
#endif // !defined(SWIG)

#endif // !defined(_OSMAND_CORE_QT_EXTENSIONS_H_)
