#include "MapStyleValue.h"

#include "Utilities.h"

OsmAnd::MapStyleValue::MapStyleValue()
    : isComplex(false)
{
    asSimple.asUInt64 = 0;
}

OsmAnd::MapStyleValue::~MapStyleValue()
{

}

bool OsmAnd::MapStyleValue::parse(const QString& input, const MapStyleValueDataType dataType, const bool isComplex, MapStyleValue& output)
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
                    output.asComplex.asInt.dip = Utilities::parseArbitraryInt(input, -1);
                    output.asComplex.asInt.px = 0.0;
                }
                else
                {
                    // "dip:px" format
                    const auto& complexValue = input.split(QLatin1Char(':'), QString::KeepEmptyParts);

                    output.asComplex.asInt.dip = Utilities::parseArbitraryInt(complexValue[0], 0);
                    output.asComplex.asInt.px = Utilities::parseArbitraryInt(complexValue[1], 0);
                }
            }
            else
            {
                assert(!input.contains(':'));
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
                    output.asComplex.asFloat.dip = Utilities::parseArbitraryFloat(input, -1.0f);
                    output.asComplex.asFloat.px = 0.0f;
                }
                else
                {
                    // 'dip:px' format
                    const auto& complexValue = input.split(':', QString::KeepEmptyParts);

                    output.asComplex.asFloat.dip = Utilities::parseArbitraryFloat(complexValue[0], 0);
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
