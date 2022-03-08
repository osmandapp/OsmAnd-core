#include "WeatherDataConverter.h"

OsmAnd::WeatherDataConverter::Temperature::Temperature(
    const Unit& unit_,
    const double value_)
    : unit(unit_)
    , value(value_)
{
}

OsmAnd::WeatherDataConverter::Temperature::~Temperature()
{
}

double OsmAnd::WeatherDataConverter::Temperature::toUnit(const Unit& unit_) const
{
    switch (unit_)
    {
        case Unit::C:
            return unit == Unit::C ? value : (value * 1.8 + 32);
        case Unit::F:
            return unit == Unit::F ? value : (value * 1.8 + 32);
    }
    return 0.0;
}

OsmAnd::WeatherDataConverter::Speed::Speed(
    const Unit& unit_,
    const double value_)
    : unit(unit_)
    , value(value_)
{
}

OsmAnd::WeatherDataConverter::Speed::~Speed()
{
}

double OsmAnd::WeatherDataConverter::Speed::toUnit(const Unit& unit_) const
{
    switch (unit)
    {
        case Unit::KNOTS:
            return knotsToUnit(value, unit_);
        case Unit::METERS_PER_SECOND:
            return mpsToUnit(value, unit_);
        case Unit::KILOMETERS_PER_HOUR:
            return kmhToUnit(value, unit_);
        case Unit::MILES_PER_HOUR:
            return mphToUnit(value, unit_);
        default:
            return 0.0;
    }
}

double OsmAnd::WeatherDataConverter::Speed::knotsToUnit(const double valueKnots, const Unit& otherUnit) const
{
    switch (otherUnit)
    {
        case Unit::KNOTS:
            return valueKnots;
        case Unit::METERS_PER_SECOND:
            return (valueKnots * 0.514444);
        case Unit::KILOMETERS_PER_HOUR:
            return (valueKnots * 1.852);
        case Unit::MILES_PER_HOUR:
            return (valueKnots * 1.150779);
        default:
            return 0.0;
    }
}

double OsmAnd::WeatherDataConverter::Speed::mpsToUnit(const double valueMps, const Unit& otherUnit) const
{
    switch (otherUnit)
    {
        case Unit::KNOTS:
            return valueMps * 1.943844;
        case Unit::METERS_PER_SECOND:
            return valueMps;
        case Unit::KILOMETERS_PER_HOUR:
            return valueMps * 3.6;
        case Unit::MILES_PER_HOUR:
            return valueMps * 2.236936;
        default:
            return 0.0;
    }
}

double OsmAnd::WeatherDataConverter::Speed::kmhToUnit(const double valueKmh, const Unit& otherUnit) const
{
    switch (otherUnit)
    {
        case Unit::KNOTS:
            return valueKmh / 1.852;
        case Unit::METERS_PER_SECOND:
            return valueKmh / 3.6;
        case Unit::KILOMETERS_PER_HOUR:
            return valueKmh;
        case Unit::MILES_PER_HOUR:
            return valueKmh * 0.621371;
        default:
            return 0.0;
    }
}

double OsmAnd::WeatherDataConverter::Speed::mphToUnit(const double valueMph, const Unit& otherUnit) const
{
    switch (otherUnit)
    {
        case Unit::KNOTS:
            return valueMph * 0.868976;
        case Unit::METERS_PER_SECOND:
            return valueMph * 0.44704;
        case Unit::KILOMETERS_PER_HOUR:
            return valueMph * 1.609344;
        case Unit::MILES_PER_HOUR:
            return valueMph;
        default:
            return 0.0;
    }
}

OsmAnd::WeatherDataConverter::Pressure::Pressure(
    const Unit& unit_,
    const double value_)
    : unit(unit_)
    , value(value_)
{
}

OsmAnd::WeatherDataConverter::Pressure::~Pressure()
{
}

double OsmAnd::WeatherDataConverter::Pressure::toUnit(const Unit& unit_) const
{
    static const auto hpaPerInHg = 33.8639;
    static const auto hpaPerMmHg = 1.3332;
    static const auto mmPerInch = 25.4;
    switch (unit)
    {
        case Unit::HECTOPASCAL:
            switch (unit_)
            {
                case Unit::HECTOPASCAL: return value;
                case Unit::INCHES_HG: return value / hpaPerInHg;
                case Unit::MM_HG: return value / hpaPerMmHg;
            }
            break;
            
        case Unit::INCHES_HG:
            switch (unit_)
            {
                case Unit::HECTOPASCAL: return value * hpaPerInHg;
                case Unit::INCHES_HG: return value;
                case Unit::MM_HG: return value * mmPerInch;
            }
            break;

        case Unit::MM_HG:
            switch (unit_)
            {
                case Unit::HECTOPASCAL: return value * hpaPerMmHg;
                case Unit::INCHES_HG: return value / mmPerInch;
                case Unit::MM_HG: return value;
            }
            break;
    }
    return 0.0;
}

OsmAnd::WeatherDataConverter::Precipitation::Precipitation(
    const Unit& unit_,
    const double value_)
    : unit(unit_)
    , value(value_)
{
}

OsmAnd::WeatherDataConverter::Precipitation::~Precipitation()
{
}

double OsmAnd::WeatherDataConverter::Precipitation::toUnit(const Unit& unit_) const
{
    if (unit == unit_)
        return value;
    
    static const auto mmPerInch = 25.4;
    if (unit == Unit::MM && unit_ == Unit::INCHES)
        return value / mmPerInch;
    
    if (unit == Unit::INCHES && unit_ == Unit::MM)
        return value * mmPerInch;
    
    return 0.0;
}

OsmAnd::WeatherDataConverter::WeatherDataConverter()
{
}

OsmAnd::WeatherDataConverter::~WeatherDataConverter()
{
}

