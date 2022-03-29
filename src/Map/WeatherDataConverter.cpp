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

OsmAnd::WeatherDataConverter::Temperature::Unit OsmAnd::WeatherDataConverter::Temperature::unitFromString(const QString& unitString)
{
    if (unitString.toLower() == QStringLiteral("c"))
        return Unit::C;
    else if (unitString.toLower() == QStringLiteral("f"))
        return Unit::F;

    return Unit::C;
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

OsmAnd::WeatherDataConverter::Speed::Unit OsmAnd::WeatherDataConverter::Speed::unitFromString(const QString& unitString)
{
    if (unitString.toLower() == QStringLiteral("kt"))
        return Unit::KNOTS;
    else if (unitString.toLower() == QStringLiteral("m/s"))
        return Unit::METERS_PER_SECOND;
    else if (unitString.toLower() == QStringLiteral("km/h"))
        return Unit::KILOMETERS_PER_HOUR;
    else if (unitString.toLower() == QStringLiteral("mph"))
        return Unit::MILES_PER_HOUR;

    return Unit::METERS_PER_SECOND;
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
    static const auto hpaPerPa = 0.01;
    static const auto hpaPerInHg = 33.8639;
    static const auto hpaPerMmHg = 1.3332;
    static const auto mmPerInch = 25.4;
    switch (unit)
    {
        case Unit::PASCAL:
            switch (unit_)
            {
                case Unit::PASCAL: return value;
                case Unit::HECTOPASCAL: return value * hpaPerPa;
                case Unit::INCHES_HG: return value * hpaPerPa / hpaPerInHg;
                case Unit::MM_HG: return value * hpaPerPa / hpaPerMmHg;
            }
            break;

        case Unit::HECTOPASCAL:
            switch (unit_)
            {
                case Unit::PASCAL: return value / hpaPerPa;
                case Unit::HECTOPASCAL: return value;
                case Unit::INCHES_HG: return value / hpaPerInHg;
                case Unit::MM_HG: return value / hpaPerMmHg;
            }
            break;
            
        case Unit::INCHES_HG:
            switch (unit_)
            {
                case Unit::PASCAL: return value * hpaPerInHg / hpaPerPa;
                case Unit::HECTOPASCAL: return value * hpaPerInHg;
                case Unit::INCHES_HG: return value;
                case Unit::MM_HG: return value * mmPerInch;
            }
            break;

        case Unit::MM_HG:
            switch (unit_)
            {
                case Unit::PASCAL: return value * hpaPerMmHg / hpaPerPa;
                case Unit::HECTOPASCAL: return value * hpaPerMmHg;
                case Unit::INCHES_HG: return value / mmPerInch;
                case Unit::MM_HG: return value;
            }
            break;
    }
    return 0.0;
}

OsmAnd::WeatherDataConverter::Pressure::Unit OsmAnd::WeatherDataConverter::Pressure::unitFromString(const QString& unitString)
{
    if (unitString.toLower() == QStringLiteral("pa"))
        return Unit::PASCAL;
    else if (unitString.toLower() == QStringLiteral("hpa"))
        return Unit::HECTOPASCAL;
    else if (unitString.toLower() == QStringLiteral("inhg"))
        return Unit::INCHES_HG;
    else if (unitString.toLower() == QStringLiteral("mmhg"))
        return Unit::MM_HG;

    return Unit::PASCAL;
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
    static const auto mmPerInch = 25.4;
    static const auto secPerHour = 3600.0;
    switch (unit)
    {
        case Unit::MM:
            switch (unit_)
            {
                case Unit::MM: return value;
                case Unit::INCHES: return value / mmPerInch;
                case Unit::KG_M2_S: return value / secPerHour;
            }
            break;

        case Unit::INCHES:
            switch (unit_)
            {
                case Unit::MM: return value * mmPerInch;
                case Unit::INCHES: return value;
                case Unit::KG_M2_S: return value * mmPerInch / secPerHour;
            }
            break;
            
        case Unit::KG_M2_S:
            switch (unit_)
            {
                case Unit::MM: return value * secPerHour;
                case Unit::INCHES: return value * secPerHour / mmPerInch;
                case Unit::KG_M2_S: return value;
            }
            break;
    }
    return 0.0;
}

OsmAnd::WeatherDataConverter::Precipitation::Unit OsmAnd::WeatherDataConverter::Precipitation::unitFromString(const QString& unitString)
{
    if (unitString.toLower() == QStringLiteral("in"))
        return Unit::INCHES;
    else if (unitString.toLower() == QStringLiteral("mm"))
        return Unit::MM;
    else if (unitString.toLower() == QStringLiteral("kg/(m^2 s)"))
        return Unit::KG_M2_S;

    return Unit::MM;
}

OsmAnd::WeatherDataConverter::WeatherDataConverter()
{
}

OsmAnd::WeatherDataConverter::~WeatherDataConverter()
{
}

