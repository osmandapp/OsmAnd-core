#ifndef _OSMAND_CORE_QT_COMMON_H_
#define _OSMAND_CORE_QT_COMMON_H_

#include <cassert>
#include <memory>
#include <iostream>
#include <algorithm>
#include <functional>

#include <OsmAndCore/QtExtensions.h>
#include <QHash>
#include <QList>
#include <QVector>

namespace OsmAnd
{
    template<typename KEY, typename VALUE>
    Q_DECL_CONSTEXPR QMutableHashIterator<KEY, VALUE> mutableIteratorOf(QHash<KEY, VALUE>& container)
    {
        return QMutableHashIterator<KEY, VALUE>(container);
    }

    template<typename T>
    Q_DECL_CONSTEXPR QMutableListIterator<T> mutableIteratorOf(QList<T>& container)
    {
        return QMutableListIterator<T>(container);
    }

    template<typename T>
    Q_DECL_CONSTEXPR QMutableVectorIterator<T> mutableIteratorOf(QVector<T>& container)
    {
        return QMutableVectorIterator<T>(container);
    }

    template<typename KEY, typename VALUE>
    QHash< KEY, VALUE > hashFrom(const QList<VALUE>& input, const std::function<KEY(const VALUE& item)> keyGetter)
    {
        QHash< KEY, VALUE > result;

        for (const auto& item : input)
            result.insert(keyGetter(item), item);

        return result;
    }

    template<typename KEY, typename VALUE>
    QHash< KEY, VALUE > hashFrom(const QVector<VALUE>& input, const std::function<KEY(const VALUE& item)> keyGetter)
    {
        QHash< KEY, VALUE > result;

        for (const auto& item : input)
            result.insert(keyGetter(item), item);

        return result;
    }
}

#endif // !defined(_OSMAND_CORE_QT_COMMON_H_)
