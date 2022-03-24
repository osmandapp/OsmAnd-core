#ifndef _OSMAND_CORE_WEATHER_DATA_CONVERTER_H_
#define _OSMAND_CORE_WEATHER_DATA_CONVERTER_H_

#include <OsmAndCore/stdlib_common.h>

#include <OsmAndCore/QtExtensions.h>
#include <QHash>
#include <QList>
#include <QString>
#include <QDir>
#include <QDateTime>

#include <OsmAndCore.h>
#include <OsmAndCore/CommonTypes.h>
#include <OsmAndCore/LatLon.h>
#include <OsmAndCore/Metrics.h>
#include <OsmAndCore/PrivateImplementation.h>
#include <OsmAndCore/IQueryController.h>
#include <OsmAndCore/Map/GeoCommonTypes.h>

namespace OsmAnd
{
    class OSMAND_CORE_API WeatherDataConverter
    {
        Q_DISABLE_COPY_AND_MOVE(WeatherDataConverter);

    public:
        class Temperature {
        public:
            enum class Unit {
                C,
                F,
            };
            
        public:
            Temperature(const Unit& unit, const double value);
            virtual ~Temperature();
            
            const Unit unit;
            const double value;

            double toUnit(const Unit& unit) const;
            static Unit unitFromString(const QString& unitString);
        };
        
        class Speed {
        public:
            enum class Unit {
                KNOTS,
                METERS_PER_SECOND,
                KILOMETERS_PER_HOUR,
                MILES_PER_HOUR,
            };
            
        private:
            double knotsToUnit(const double valueKnots, const Unit& otherUnit) const;
            double mpsToUnit(const double valueMps, const Unit& otherUnit) const;
            double kmhToUnit(const double valueKmh, const Unit& otherUnit) const;
            double mphToUnit(const double valueMph, const Unit& otherUnit) const;
            
        public:
            Speed(const Unit& unit, const double value);
            virtual ~Speed();
            
            const Unit unit;
            const double value;

            double toUnit(const Unit& unit) const;
            static Unit unitFromString(const QString& unitString);
        };
        
        class Pressure {
        public:
            enum class Unit {
                PASCAL,
                HECTOPASCAL,
                INCHES_HG,
                MM_HG,
            };
            
        public:
            Pressure(const Unit& unit, const double value);
            virtual ~Pressure();
            
            const Unit unit;
            const double value;

            double toUnit(const Unit& unit) const;
            static Unit unitFromString(const QString& unitString);
        };
        
        class Precipitation {
        public:
            enum class Unit {
                MM,
                INCHES,
                KG_M2_S,
            };
            
        public:
            Precipitation(const Unit& unit, const double value);
            virtual ~Precipitation();
            
            const Unit unit;
            const double value;

            double toUnit(const Unit& unit) const;
            static Unit unitFromString(const QString& unitString);
        };
        
    private:
    protected:
    public:
        WeatherDataConverter();
        virtual ~WeatherDataConverter();
        
    };
}

#endif // !defined(_OSMAND_CORE_WEATHER_DATA_CONVERTER_H_)
