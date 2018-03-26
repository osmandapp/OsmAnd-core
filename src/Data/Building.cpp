#include "Building.h"

#include "Street.h"
#include "StreetGroup.h"
#include "Utilities.h"

OsmAnd::Building::Building(const std::shared_ptr<const Street>& street_)
    : Address(street_ -> obfSection, AddressType::Building)
    , street(street_)
    , streetGroup(street->streetGroup)
    , interpolation(Interpolation::Disabled)
{
}

OsmAnd::Building::Building(const std::shared_ptr<const StreetGroup>& streetGroup_)
    : Address(streetGroup_ -> obfSection, AddressType::Building)
    , street(nullptr)
    , streetGroup(streetGroup_)
    , interpolation(Interpolation::Disabled)
{
}

OsmAnd::Building::~Building()
{
}

QString OsmAnd::Building::toString() const
{
    return nativeName;
}

float OsmAnd::Building::evaluateInterpolation(const QString& hno) const
{
    if (interpolation != Interpolation::Disabled || interpolationInterval > 0 || nativeName.contains('-'))
    {
        int num = OsmAnd::Utilities::extractFirstInteger(hno);
        QString fname = nativeName;
        int numB = OsmAnd::Utilities::extractFirstInteger(fname);
        int numT = numB;
        if (num >= numB)
        {
            QString sname = interpolationNativeName;
            if (fname.contains('-') && sname.isNull())
            {
                int l = fname.indexOf('-');
                sname = fname.mid(l + 1, fname.length() - (l + 1));
            }
            if (!sname.isNull())
            {
                numT = OsmAnd::Utilities::extractFirstInteger(sname);
                if (numT < num)
                    return -1;
            }
            if (interpolation == Interpolation::Even && num % 2 == 1)
                return -1;

            if (interpolation == Interpolation::Odd && num % 2 == 0)
                return -1;

            if (interpolationInterval != 0 && (num - numB) % interpolationInterval != 0)
                return -1;
        }
        else
        {
            return -1;
        }
        if (numT > numB)
            return ((float)num - numB) / (((float)numT - numB));
        
        return 0;
    }
    return -1;
}

bool OsmAnd::Building::belongsToInterpolation(const QString& hno) const
{
    return evaluateInterpolation(hno) >= 0;
}

QString OsmAnd::Building::getInterpolationName(double coeff) const
{
    if (!interpolationNativeName.isEmpty())
    {
        int fi = Utilities::extractFirstInteger(nativeName);
        int si = Utilities::extractFirstInteger(interpolationNativeName);
        if (si != 0 && fi != 0)
        {
            int num = (int) (fi + (si - fi) * coeff);
            if (interpolation == Interpolation::Even || interpolation == Interpolation::Odd)
            {
                if (num % 2 == (interpolation == Interpolation::Even ? 1 : 0))
                    num--;
            }
            else if (interpolationInterval > 0)
            {
                int intv = interpolationInterval;
                if ((num - fi) % intv != 0) {
                    num = ((num - fi) / intv) * intv + fi;
                }
            }
            return QString::number(num);
        }
    }
    return QString();
}

