#include "Utilities.h"

QString OsmAndTools::Utilities::resolvePath(const QString& input)
{
    QString output = input;

    if (output.startsWith(QLatin1Char('"')))
        output = output.mid(1);
    if (output.endsWith(QLatin1Char('"')))
        output = output.mid(0, output.size() - 1);

    return output;
}

QString OsmAndTools::Utilities::purifyArgumentValue(const QString& input)
{
    QString output = input;

    if (output.startsWith(QLatin1Char('"')))
        output = output.mid(1);
    if (output.endsWith(QLatin1Char('"')))
        output = output.mid(0, output.size() - 1);

    return output;
}
