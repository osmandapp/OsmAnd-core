#include "MapStyleConstantValue.h"

#include "Utilities.h"

OsmAnd::MapStyleConstantValue::MapStyleConstantValue()
    : isComplex(false)
{
    asSimple.asUInt64 = 0;
}

OsmAnd::MapStyleConstantValue::~MapStyleConstantValue()
{

}

bool OsmAnd::MapStyleConstantValue::parse(
    const QString& input,
    const MapStyleValueDataType dataType,
    const bool isComplex,
    MapStyleConstantValue& output)
{
    switch (dataType)
    {
        case MapStyleValueDataType::Boolean:
        {
            output.asSimple.asInt = (input == QLatin1String("true")) ? 1 : 0;
            return true;
        }

        case MapStyleValueDataType::Integer:
        {
            if (isComplex)
            {
                output.isComplex = true;
                if (!input.contains(QLatin1Char(':')))
                {
                    output.asComplex.asInt.pt = Utilities::parseArbitraryInt(input, -1);
                    output.asComplex.asInt.px = 0.0;
                }
                else
                {
                    // "pt:px" format
                    const auto& complexValue = input.split(QLatin1Char(':'), Qt::KeepEmptyParts);

                    output.asComplex.asInt.pt = Utilities::parseArbitraryInt(complexValue[0], 0);
                    output.asComplex.asInt.px = Utilities::parseArbitraryInt(complexValue[1], 0);
                }
            }
            else
            {
                //assert(!input.contains(':'));
                output.asSimple.asInt = Utilities::parseArbitraryInt(input, -1);
            }
            return true;
        }

        case MapStyleValueDataType::Float:
        {
            if (isComplex)
            {
                output.isComplex = true;
                if (!input.contains(':'))
                {
                    output.asComplex.asFloat.pt = Utilities::parseArbitraryFloat(input, -1.0f);
                    output.asComplex.asFloat.px = 0.0f;
                }
                else
                {
                    // 'pt:px' format
                    const auto& complexValue = input.split(':', Qt::KeepEmptyParts);

                    output.asComplex.asFloat.pt = Utilities::parseArbitraryFloat(complexValue[0], 0);
                    output.asComplex.asFloat.px = Utilities::parseArbitraryFloat(complexValue[1], 0);
                }
            }
            else
            {
                assert(!input.contains(':'));
                output.asSimple.asFloat = Utilities::parseArbitraryFloat(input, -1.0f);
            }
            return true;
        }

        case MapStyleValueDataType::String:
        {
            assert(false);
            return false;
        }

        case MapStyleValueDataType::Color:
        {
            bool ok;
            const auto color = Utilities::parseColor(input, ColorARGB(), &ok);
            if (!ok)
                return false;
            output.asSimple.asUInt = color.argb;
            return true;
        }

        default:
            return false;
    }
}

QString OsmAnd::MapStyleConstantValue::toString(const MapStyleValueDataType dataType) const
{
    switch (dataType)
    {
        case MapStyleValueDataType::Boolean:
        {
            return QString(QLatin1String("boolean(%1)"))
                .arg((asSimple.asUInt == 0) ? QLatin1String("false") : QLatin1String("true"));
        }

        case MapStyleValueDataType::Integer:
        {
            if (isComplex)
            {
                return QString(QLatin1String("integer(%1pt:%2px)"))
                    .arg(QString::number(asComplex.asInt.pt))
                    .arg(QString::number(asComplex.asInt.px));
            }
            else
            {
                return QString(QLatin1String("integer(%1)"))
                    .arg(QString::number(asSimple.asInt));
            }
        }

        case MapStyleValueDataType::Float:
        {
            if (isComplex)
            {
                return QString(QLatin1String("float(%1pt:%2px)"))
                    .arg(QString::number(asComplex.asFloat.pt))
                    .arg(QString::number(asComplex.asFloat.px));
            }
            else
            {
                return QString(QLatin1String("float(%1)"))
                    .arg(QString::number(asSimple.asFloat));
            }
        }

        case MapStyleValueDataType::String:
        {
            return QString(QLatin1String("string(#%1)"))
                .arg(QString::number(asSimple.asInt));
        }

        case MapStyleValueDataType::Color:
        {
            return QString(QLatin1String("color(#%1)"))
                .arg(QString::number(ColorARGB(asSimple.asUInt).argb, 16).right(6));
        }
    }

    return {};
}
