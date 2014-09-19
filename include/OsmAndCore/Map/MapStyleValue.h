#ifndef _OSMAND_CORE_MAP_STYLE_VALUE_H_
#define _OSMAND_CORE_MAP_STYLE_VALUE_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QString>

#include <OsmAndCore.h>
#include <OsmAndCore/Map/MapCommonTypes.h>

namespace OsmAnd
{
    struct OSMAND_CORE_API MapStyleValue Q_DECL_FINAL
    {
        MapStyleValue();
        ~MapStyleValue();

        bool isComplex;

        template<typename T> struct ComplexData
        {
            T dip;
            T px;

            inline T evaluate(const float densityFactor) const
            {
                return dip*densityFactor + px;
            }
        };

        union {
            union {
                float asFloat;
                int32_t asInt;
                uint32_t asUInt;

                double asDouble;
                int64_t asInt64;
                uint64_t asUInt64;
            } asSimple;

            union {
                ComplexData<float> asFloat;
                ComplexData<int32_t> asInt;
                ComplexData<uint32_t> asUInt;
            } asComplex;
        };

        static bool parse(const QString& input, const MapStyleValueDataType dataType, const bool isComplex, MapStyleValue& outValue);

        static inline MapStyleValue fromSimpleUInt(const uint32_t value)
        {
            MapStyleValue mapStyleValue;
            mapStyleValue.asSimple.asUInt = value;
            return mapStyleValue;
        };

        QString toString(const MapStyleValueDataType dataType) const;
    };
}

#endif // !defined(_OSMAND_CORE_MAP_STYLE_VALUE_H_)
