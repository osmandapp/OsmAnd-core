#ifndef _OSMAND_CORE_METRICS_H_
#define _OSMAND_CORE_METRICS_H_

#include <OsmAndCore/stdlib_common.h>
#include <typeinfo>

#include <OsmAndCore/QtExtensions.h>
#include <OsmAndCore/ignore_warnings_on_external_includes.h>
#include <QString>
#include <QList>
#include <OsmAndCore/restore_internal_warnings.h>

#include <OsmAndCore.h>
#include <OsmAndCore/Common.h>
#include <OsmAndCore/Ref.h>

#define EMIT_METRIC_FIELD(type, name, measurement)                                                                              \
    type name
#define RESET_METRIC_FIELD(type, name, measurement)                                                                             \
    name = 0
#define PRINT_METRIC_FIELD(type, name, measurement)                                                                             \
    output +=                                                                                                                   \
        (output.isEmpty() ? QString() : QString(QLatin1String("\n"))) +                                                         \
        prefix +                                                                                                                \
        QString(QLatin1String(#name " = %1" measurement)).arg(name)

namespace OsmAnd
{
    struct OSMAND_CORE_API Metric
    {
        Metric();
        virtual ~Metric();
        virtual void reset();

        QList< Ref<Metric> > submetrics;

        void addSubmetric(const std::shared_ptr<Metric>& submetric);

        template<typename T>
        std::shared_ptr<T> addSubmetricOfType()
        {
            const std::shared_ptr<T> newSubmetric(new T());
            addSubmetric(newSubmetric);
            return newSubmetric;
        }

        template<typename T>
        std::shared_ptr<T> findSubmetricOfType(const bool recursive = false)
        {
            for (auto& submetric : submetrics)
            {
                if (const auto typedSubmetric = std::dynamic_pointer_cast<T>(submetric.shared_ptr()))
                    return typedSubmetric;
            }

            if (recursive)
            {
                for (auto& submetric : submetrics)
                {
                    if (const auto typedSubmetric = submetric->findSubmetricOfType<T>(true))
                        return typedSubmetric;
                }
            }

            return nullptr;
        }

        template<typename T>
        std::shared_ptr<const T> findSubmetricOfType(const bool recursive = false) const
        {
            for (const auto& submetric : submetrics)
            {
                if (const auto typedSubmetric = std::dynamic_pointer_cast<const T>(submetric.shared_ptr()))
                    return typedSubmetric;
            }

            if (recursive)
            {
                for (auto& submetric : submetrics)
                {
                    if (const auto typedSubmetric = submetric->findSubmetricOfType<const T>(true))
                        return typedSubmetric;
                }
            }

            return nullptr;
        }

        void addOrReplaceSubmetric(const std::shared_ptr<Metric>& submetric);

        std::shared_ptr<Metric> findSubmetricOfExactType(const std::type_info& requestedType);
        template<typename T>
        std::shared_ptr<T> findSubmetricOfExactType()
        {
            return std::dynamic_pointer_cast<T>(findSubmetricOfExactType(typeid(T)));
        }

        std::shared_ptr<const Metric> findSubmetricOfExactType(const std::type_info& requestedType) const;
        template<typename T>
        std::shared_ptr<const T> findSubmetricOfExactType() const
        {
            return std::dynamic_pointer_cast<const T>(findSubmetricOfExactType(typeid(T)));
        }

        template<typename T>
        std::shared_ptr<T> findOrAddSubmetricOfType()
        {
            if (const auto submetric = findSubmetricOfExactType<T>())
                return submetric;

            return addSubmetricOfType<T>();
        }

        virtual QString toString(const bool shortFormat = false, const QString& prefix = {}) const;
    };
}

#endif // !defined(_OSMAND_CORE_METRICS_H_)
