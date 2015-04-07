#ifndef _OSMAND_CORE_QT_COMMON_H_
#define _OSMAND_CORE_QT_COMMON_H_

#include <cassert>
#include <memory>
#include <iostream>
#include <algorithm>
#include <functional>
#include <iterator>
#include <type_traits>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QHash>
#include <QMap>
#include <QList>
#include <QVector>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>

namespace OsmAnd
{
    template<typename KEY, typename VALUE>
    inline Q_DECL_CONSTEXPR QMutableHashIterator<KEY, VALUE> mutableIteratorOf(QHash<KEY, VALUE>& container)
    {
        return QMutableHashIterator<KEY, VALUE>(container);
    }

    template<typename KEY, typename VALUE>
    inline Q_DECL_CONSTEXPR QMutableMapIterator<KEY, VALUE> mutableIteratorOf(QMap<KEY, VALUE>& container)
    {
        return QMutableMapIterator<KEY, VALUE>(container);
    }

    template<typename T>
    inline Q_DECL_CONSTEXPR QMutableListIterator<T> mutableIteratorOf(QList<T>& container)
    {
        return QMutableListIterator<T>(container);
    }

    template<typename T>
    inline Q_DECL_CONSTEXPR QMutableVectorIterator<T> mutableIteratorOf(QVector<T>& container)
    {
        return QMutableVectorIterator<T>(container);
    }

    template<typename KEY, typename VALUE>
    inline Q_DECL_CONSTEXPR QHashIterator<KEY, VALUE> iteratorOf(const QHash<KEY, VALUE>& container)
    {
        return QHashIterator<KEY, VALUE>(container);
    }

    template<typename KEY, typename VALUE>
    inline Q_DECL_CONSTEXPR QMapIterator<KEY, VALUE> iteratorOf(const QMap<KEY, VALUE>& container)
    {
        return QMapIterator<KEY, VALUE>(container);
    }

    template<typename T>
    inline Q_DECL_CONSTEXPR QListIterator<T> iteratorOf(const QList<T>& container)
    {
        return QListIterator<T>(container);
    }

    template<typename T>
    inline Q_DECL_CONSTEXPR QVectorIterator<T> iteratorOf(const QVector<T>& container)
    {
        return QVectorIterator<T>(container);
    }

    template<typename KEY, typename VALUE>
    inline QHash< KEY, VALUE > hashFrom(const QList<VALUE>& input, const std::function<KEY(const VALUE& item)> keyGetter)
    {
        QHash< KEY, VALUE > result;

        for (const auto& item : input)
            result.insert(keyGetter(item), item);

        return result;
    }

    template<typename KEY, typename VALUE>
    inline QHash< KEY, VALUE > hashFrom(const QVector<VALUE>& input, const std::function<KEY(const VALUE& item)> keyGetter)
    {
        QHash< KEY, VALUE > result;

        for (const auto& item : input)
            result.insert(keyGetter(item), item);

        return result;
    }

    template<typename KEY, typename VALUE>
    inline QHash< KEY, VALUE > hashFrom(const QMap<KEY, VALUE>& input)
    {
        QHash< KEY, VALUE > result;

        const auto citEnd = input.cend();
        for (auto citEntry = input.cbegin(); citEntry != citEnd; ++citEntry)
            result.insert(citEntry.key(), citEntry.value());

        return result;
    }

    template<typename KEY, typename VALUE>
    inline QHash< KEY, VALUE > mapFrom(const QList<VALUE>& input, const std::function<KEY(const VALUE& item)> keyGetter)
    {
        QMap< KEY, VALUE > result;

        for (const auto& item : input)
            result.insert(keyGetter(item), item);

        return result;
    }

    template<typename KEY, typename VALUE>
    inline QMap< KEY, VALUE > mapFrom(const QVector<VALUE>& input, const std::function<KEY(const VALUE& item)> keyGetter)
    {
        QMap< KEY, VALUE > result;

        for (const auto& item : input)
            result.insert(keyGetter(item), item);

        return result;
    }

    template<typename KEY, typename VALUE>
    inline QMap< KEY, VALUE > mapFrom(const QHash<KEY, VALUE>& input)
    {
        QMap< KEY, VALUE > result;

        const auto citEnd = input.cend();
        for (auto citEntry = input.cbegin(); citEntry != citEnd; ++citEntry)
            result.insert(citEntry.key(), citEntry.value());

        return result;
    }

    template<typename ITEM>
    inline QVector<ITEM> vectorFrom(const QSet<ITEM>& input)
    {
        QVector<ITEM> result;
        result.reserve(input.size());

        for (const auto& item : input)
            result.push_back(item);

        return result;
    }

    template<typename T>
    inline auto detachedOf(const T& input)
        -> typename std::enable_if <
            std::is_same<decltype(input.detach()), void()>::value && !std::is_same<decltype(std::begin(input)), void()>::value,
            T > ::type
    {
        auto copy = input;
        copy.detach();
        for (auto& value : copy)
            value = detachedOf(value);
        return copy;
    }

    template<typename T>
    inline auto detachedOf(const T& input)
        -> typename std::enable_if< std::is_same<decltype(input.detach()), void()>::value, T>::type
    {
        auto copy = input;
        copy.detach();
        return copy;
    }

    template<typename T>
    inline Q_DECL_CONSTEXPR T detachedOf(const T& input)
    {
        return input;
    }

    template<typename KEY, typename VALUE>
    inline unsigned int mergeNonExistent(QHash<KEY, VALUE>& inOutHash, const QHash<KEY, VALUE>& hashToMerge)
    {
        unsigned int entriesMerged = 0;

        auto itEntryToMerge = iteratorOf(hashToMerge);
        while (itEntryToMerge.hasNext())
        {
            const auto& entryToMerge = itEntryToMerge.next();
            const auto entryKey = entryToMerge.key();
            const auto entryValue = entryToMerge.value();

            if (inOutHash.contains(entryKey))
                continue;

            inOutHash.insert(entryKey, entryValue);
            entriesMerged++;
        }

        return entriesMerged;
    }

    template<typename KEY, typename VALUE>
    inline unsigned int mergeNonExistent(QMap<KEY, VALUE>& inOutHash, const QMap<KEY, VALUE>& hashToMerge)
    {
        unsigned int entriesMerged = 0;

        auto itEntryToMerge = iteratorOf(hashToMerge);
        while (itEntryToMerge.hasNext())
        {
            const auto& entryToMerge = itEntryToMerge.next();
            const auto entryKey = entryToMerge.key();
            const auto entryValue = entryToMerge.value();

            if (inOutHash.contains(entryKey))
                continue;

            inOutHash.insert(entryKey, entryValue);
            entriesMerged++;
        }

        return entriesMerged;
    }

    template<typename KEY, typename VALUE>
    inline unsigned int mergeOverwriting(QHash<KEY, VALUE>& inOutHash, const QHash<KEY, VALUE>& hashToMerge)
    {
        auto itEntryToMerge = iteratorOf(hashToMerge);
        while (itEntryToMerge.hasNext())
        {
            const auto& entryToMerge = itEntryToMerge.next();
            const auto entryKey = entryToMerge.key();
            const auto entryValue = entryToMerge.value();

            inOutHash.insert(entryKey, entryValue);
        }

        return hashToMerge.size();
    }

    template<typename KEY, typename VALUE>
    inline unsigned int mergeOverwriting(QMap<KEY, VALUE>& inOutHash, const QMap<KEY, VALUE>& hashToMerge)
    {
        auto itEntryToMerge = iteratorOf(hashToMerge);
        while (itEntryToMerge.hasNext())
        {
            const auto& entryToMerge = itEntryToMerge.next();
            const auto entryKey = entryToMerge.key();
            const auto entryValue = entryToMerge.value();

            inOutHash.insert(entryKey, entryValue);
        }

        return hashToMerge.size();
    }

    template<typename KEY_OUT, typename VALUE_OUT, typename KEY_IN, typename VALUE_IN>
    inline QHash<KEY_OUT, VALUE_OUT> copyAs(const QHash<KEY_IN, VALUE_IN>& input)
    {
        QHash<KEY_OUT, VALUE_OUT> copy;
        for (const auto& inputEntry : rangeOf(input))
            copy.insertMulti(inputEntry.key(), inputEntry.value());
        return copy;
    }

    template<typename KEY_OUT, typename VALUE_OUT, typename KEY_IN, typename VALUE_IN>
    inline QMap<KEY_OUT, VALUE_OUT> copyAs(const QMap<KEY_IN, VALUE_IN>& input)
    {
        QMap<KEY_OUT, VALUE_OUT> copy;
        for (const auto& inputEntry : rangeOf(input))
            copy.insertMulti(inputEntry.key(), inputEntry.value());
        return copy;
    }

    template<typename CONTAINER>
    inline auto qMaxElement(CONTAINER container) -> decltype(std::begin(container))
    {
        auto itElement = std::begin(container);
        auto itMaxElement = itElement;
        const auto itEnd = std::end(container);
        for (++itElement; itElement != itEnd; ++itElement)
        {
            if (*itElement > *itMaxElement)
                itMaxElement = itElement;
        }

        return itMaxElement;
    }

    template<typename OUTPUT_CONTAINER, typename INPUT_CONTAINER>
    inline OUTPUT_CONTAINER qTransform(
        const INPUT_CONTAINER& input,
        const std::function<OUTPUT_CONTAINER(typename std::iterator_traits<decltype(std::begin(input))>::reference)> transform)
    {
        OUTPUT_CONTAINER output;

        const auto itEnd = std::end(input);
        for (auto itItem = std::begin(input); itItem != itEnd; ++itItem)
            output = output + transform(*itItem);

        return output;
    }
}

#endif // !defined(_OSMAND_CORE_QT_COMMON_H_)
