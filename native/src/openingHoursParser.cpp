#include "openingHoursParser.h"

#include <time.h>
#include <set>
#include <iomanip>

#include "CommonCollections.h"
#include "commonOsmAndCore.h"
#include "Logging.h"

static const int LOW_TIME_LIMIT = 120;
static const int HIGH_TIME_LIMIT = 300;
static const int WITHOUT_TIME_LIMIT = -1;

static StringsHolder stringsHolder;

std::vector<std::string> getTwoLettersStringArray(const std::vector<std::string>& strings)
{
    std::vector<std::string> newStrings;
    newStrings.push_back("");
    for (const auto& s : strings)
    {
        if (s.length() > 2)
            newStrings.push_back(s.substr(0, 2));
        else
            newStrings.push_back(s);
    }
    return newStrings;
}

int getDayIndex(int i)
{
    switch (i)
    {
        case 0: return 1;
        case 1: return 2;
        case 2: return 3;
        case 3: return 4;
        case 4: return 5;
        case 5: return 6;
        case 6: return 0;
        default: return -1;
    }
}

void formatTime(int h, int t, std::stringstream& b)
{
    if (h < 10)
        b << "0";

    std::string hs = std::to_string(h);
    b << hs;
    b << ":";
    if (t < 10)
        b << "0";

    std::string ts = std::to_string(t);
    b << ts;
}

void formatTime(int minutes, std::stringstream& b)
{
    int hour = minutes / 60;
    int time = minutes - hour * 60;
    formatTime(hour, time, b);
}

StringsHolder::StringsHolder()
{
    daysStr = std::vector<std::string>{
        "Su",
        "Mo",
        "Tu",
        "We",
        "Th",
        "Fr",
        "Sa"
    };
    
    monthsStr = std::vector<std::string>{
        "Jan",
        "Feb",
        "Mar",
        "Apr",
        "May",
        "Jun",
        "Jul",
        "Aug",
        "Sep",
        "Oct",
        "Nov",
        "Dec"
    };

    tm time;
    std::vector<std::string> localDaysStr_;
    for (int i = 0; i < 7; i++)
    {
        char mbstr[100];
        time.tm_wday = i;
        std::strftime(mbstr, sizeof(mbstr), "%a", &time);

        localDaysStr_.push_back(std::string(mbstr));
    }

    localDaysStr = getTwoLettersStringArray(localDaysStr_);
    
    for (int i = 0; i < 12; i++)
    {
        char mbstr[100];
        time.tm_mon = i;
        std::strftime(mbstr, sizeof(mbstr), "%b", &time);

        localMothsStr.push_back(std::string(mbstr));
    }
}

StringsHolder::~StringsHolder()
{
}

OpeningHoursParser::BasicOpeningHourRule::BasicOpeningHourRule()
{
    for (int i = 0; i < 7; i++)
        _days.push_back(false);
    
    for (int i = 0; i < 12; i++)
        _months.push_back(false);
    
    for (int i = 0; i < 31; i++)
        _dayMonths.push_back(false);
    
    _publicHoliday = false;
    _schoolHoliday = false;
    _easter = false;
    _off = false;
}

OpeningHoursParser::BasicOpeningHourRule::~BasicOpeningHourRule()
{
}

std::vector<bool>& OpeningHoursParser::BasicOpeningHourRule::getDays()
{
    return _days;
}

std::vector<bool>& OpeningHoursParser::BasicOpeningHourRule::getMonths()
{
    return _months;
}

std::vector<bool>& OpeningHoursParser::BasicOpeningHourRule::getDayMonths()
{
    return _dayMonths;
}

bool OpeningHoursParser::BasicOpeningHourRule::appliesToPublicHolidays() const
{
    return _publicHoliday;
}

bool OpeningHoursParser::BasicOpeningHourRule::appliesEaster() const
{
    return _easter;
}

bool OpeningHoursParser::BasicOpeningHourRule::appliesToSchoolHolidays() const
{
    return _schoolHoliday;
}

bool OpeningHoursParser::BasicOpeningHourRule::appliesOff() const
{
    return _off;
}

void OpeningHoursParser::BasicOpeningHourRule::setPublicHolidays(bool value)
{
    _publicHoliday = value;
}

void OpeningHoursParser::BasicOpeningHourRule::setEaster(bool value)
{
    _easter = value;
}

void OpeningHoursParser::BasicOpeningHourRule::setSchoolHolidays(bool value)
{
    _schoolHoliday = value;
}

void OpeningHoursParser::BasicOpeningHourRule::setOff(bool value)
{
    _off = value;
}

void setSingleValueForArrayList(std::vector<int>& arrayList, int s)
{
    if (arrayList.size() > 0)
        arrayList.clear();

    arrayList.push_back(s);
}

void OpeningHoursParser::BasicOpeningHourRule::setStartTime(int s)
{
    setSingleValueForArrayList(_startTimes, s);
    if (_endTimes.size() != 1)
        setSingleValueForArrayList(_endTimes, 0);
}

void OpeningHoursParser::BasicOpeningHourRule::setEndTime(int e)
{
    setSingleValueForArrayList(_endTimes, e);
    if (_startTimes.size() != 1)
        setSingleValueForArrayList(_startTimes, 0);
}

void OpeningHoursParser::BasicOpeningHourRule::setStartTime(int s, int position)
{
    if (position == _startTimes.size())
    {
        _startTimes.push_back(s);
        _endTimes.push_back(0);
    }
    else
    {
        _startTimes[position] = s;
    }
}

void OpeningHoursParser::BasicOpeningHourRule::setEndTime(int s, int position)
{
    if (position == _startTimes.size())
    {
        _endTimes.push_back(s);
        _startTimes.push_back(0);
    }
    else
    {
        _endTimes[position] = s;
    }
}

int OpeningHoursParser::BasicOpeningHourRule::getStartTime() const
{
    if (_startTimes.size() == 0)
        return 0;

    return _startTimes[0];
}

int OpeningHoursParser::BasicOpeningHourRule::getStartTime(int position) const
{
    return _startTimes[position];
}

int OpeningHoursParser::BasicOpeningHourRule::getEndTime() const
{
    if (_endTimes.size() == 0)
        return 0;

    return _endTimes[0];
}

int OpeningHoursParser::BasicOpeningHourRule::getEndTime(int position) const
{
    return _endTimes[position];
}

std::vector<int> OpeningHoursParser::BasicOpeningHourRule::getStartTimes() const
{
    return _startTimes;
}

std::vector<int> OpeningHoursParser::BasicOpeningHourRule::getEndTimes() const
{
    return _endTimes;
}

bool OpeningHoursParser::BasicOpeningHourRule::containsDay(const tm& dateTime) const
{
    int i = dateTime.tm_wday + 1;
    int d = (i + 5) % 7;
    return _days[d];
}

bool OpeningHoursParser::BasicOpeningHourRule::containsNextDay(const tm& dateTime) const
{
    int i = dateTime.tm_wday + 1;
    int n = (i + 6) % 7;
    return _days[n];
}

bool OpeningHoursParser::BasicOpeningHourRule::containsPreviousDay(const tm& dateTime) const
{
    int i = dateTime.tm_wday + 1;
    int p = (i + 4) % 7;
    return _days[p];
}

bool OpeningHoursParser::BasicOpeningHourRule::containsMonth(const tm& dateTime) const
{
    int i = dateTime.tm_mon;
    return _months[i];
}

int OpeningHoursParser::BasicOpeningHourRule::getCurrentDay(const tm& dateTime) const
{
    int i = dateTime.tm_wday + 1;
    return (i + 5) % 7;
}

int OpeningHoursParser::BasicOpeningHourRule::getPreviousDay(int currentDay) const
{
    int p = currentDay - 1;
    if (p < 0)
        p += 7;
    
    return p;
}

int OpeningHoursParser::BasicOpeningHourRule::getNextDay(int currentDay) const
{
    int n = currentDay + 1;
    if (n > 6)
        n -= 7;
    
    return n;
}

int OpeningHoursParser::BasicOpeningHourRule::getCurrentTimeInMinutes(const tm& dateTime) const
{
    return dateTime.tm_hour * 60 + dateTime.tm_min;
}

bool OpeningHoursParser::BasicOpeningHourRule::isOpenedForTime(const tm& dateTime, bool checkPrevious) const
{
    int d = getCurrentDay(dateTime);
    int p = getPreviousDay(d);
    int time = getCurrentTimeInMinutes(dateTime); // Time in minutes
    for (int i = 0; i < _startTimes.size(); i++)
    {
        int startTime = _startTimes[i];
        int endTime = _endTimes[i];
        if (startTime < endTime || endTime == -1)
        {
            // one day working like 10:00-20:00 (not 20:00-04:00)
            if (_days[d] && !checkPrevious)
            {
                if (time >= startTime && (endTime == -1 || time <= endTime))
                    return !_off;
            }
        }
        else
        {
            // opening_hours includes day wrap like
            // "We 20:00-03:00" or "We 07:00-07:00"
            if (time >= startTime && _days[d] && !checkPrevious)
            {
                return !_off;
            }
            else if (time < endTime && _days[p] && checkPrevious)
            {
                // check in previous day
                return !_off;
            }
        }
    }
    return false;
}

bool OpeningHoursParser::BasicOpeningHourRule::isOpenedForTime(const tm& dateTime) const
{
    int c = calculate(dateTime);
    return c > 0;
}

bool OpeningHoursParser::BasicOpeningHourRule::contains(const tm& dateTime) const
{
    int c = calculate(dateTime);
    return c != 0;
}

bool OpeningHoursParser::BasicOpeningHourRule::isOpened24_7() const
{
    bool opened24_7 = true;
    for (int i = 0; i < 7; i++)
    {
        if (!_days[i]) {
            opened24_7 = false;
            break;
        }
    }
    
    if (opened24_7 && !_startTimes.empty())
    {
        for (int i = 0; i < _startTimes.size(); i++)
        {
            int startTime = _startTimes[i];
            int endTime = _endTimes[i];
            if (startTime == 0 && endTime / 60 == 24)
                return true;
        }
    }
    return false;
}

void OpeningHoursParser::BasicOpeningHourRule::addArray(const std::vector<bool>& array, const std::vector<std::string>& arrayNames, std::stringstream& b) const
{
    bool dash = false;
    bool first = true;
    for (int i = 0; i < array.size(); i++)
    {
        if (array[i])
        {
            if (i > 0 && array[i - 1] && i < array.size() - 1 && array[i + 1])
            {
                if (!dash)
                {
                    dash = true;
                    b << "-";
                }
                continue;
            }
            if (first)
                first = false;
            else if (!dash)
                b << ", ";
            
            b << (arrayNames.empty() ? std::to_string(i + 1) : arrayNames[i]);
            dash = false;
        }
    }
    if (!first)
        b << " ";
}

std::string OpeningHoursParser::BasicOpeningHourRule::toRuleString(const std::vector<std::string>& dayNames, const std::vector<std::string>& monthNames) const
{
    std::stringstream b;
    bool allMonths = true;
    for (int i = 0; i < _months.size(); i++)
    {
        if (!_months[i])
        {
            allMonths = false;
            break;
        }
    }
    // Month
    if (!allMonths)
        addArray(_months, monthNames, b);
    
    bool allDays = true;
    for (int i = 0; i < _dayMonths.size(); i++)
    {
        if (!_dayMonths[i])
        {
            allDays = false;
            break;
        }
    }
    if (!allDays)
        addArray(_dayMonths, std::vector<std::string>(), b);
    
    // Day
    appendDaysString(b, dayNames);
    // Time
    if (_startTimes.empty())
    {
        b << "off";
    }
    else
    {
        if (isOpened24_7())
            return "24/7";
        
        for (int i = 0; i < _startTimes.size(); i++)
        {
            int startTime = _startTimes[i];
            int endTime = _endTimes[i];
            if (i > 0)
                b << ", ";
            
            int stHour = startTime / 60;
            int stTime = startTime - stHour * 60;
            int enHour = endTime / 60;
            int enTime = endTime - enHour * 60;
            formatTime(stHour, stTime, b);
            b << "-";
            formatTime(enHour, enTime, b);
        }
        if (_off)
            b << " off";
    }
    return b.str();
}

std::string OpeningHoursParser::BasicOpeningHourRule::getTime(const tm& dateTime, bool checkAnotherDay, int limit, bool opening) const
{
    std::stringstream sb;
    int d = getCurrentDay(dateTime);
    int ad = opening ? getNextDay(d) : getPreviousDay(d);
    int time = getCurrentTimeInMinutes(dateTime);
    for (int i = 0; i < _startTimes.size(); i++)
    {
        int startTime = _startTimes[i];
        int endTime = _endTimes[i];
        if (opening)
        {
            if (startTime < endTime || endTime == -1)
            {
                if (_days[d] && !checkAnotherDay)
                {
                    int diff = startTime - time;
                    if (limit == WITHOUT_TIME_LIMIT || ((time <= startTime) && (diff <= limit)))
                    {
                        formatTime(startTime, sb);
                        break;
                    }
                }
            }
            else
            {
                int diff = -1;
                if (time <= startTime && _days[d] && !checkAnotherDay)
                    diff = startTime - time;
                else if (time > endTime && _days[ad] && checkAnotherDay)
                    diff = 24 * 60 - endTime  + time;
                
                if (limit == WITHOUT_TIME_LIMIT || ((diff != -1) && (diff <= limit)))
                {
                    formatTime(startTime, sb);
                    break;
                }
            }
        }
        else
        {
            if (startTime < endTime && endTime != -1)
            {
                if (_days[d] && !checkAnotherDay)
                {
                    int diff = endTime - time;
                    if (limit == WITHOUT_TIME_LIMIT || ((time <= endTime) && (diff <= limit)))
                    {
                        formatTime(endTime, sb);
                        break;
                    }
                }
            }
            else
            {
                int diff = -1;
                if (time <= endTime && _days[d] && !checkAnotherDay)
                    diff = 24 * 60 - time + endTime;
                else if (time < endTime && _days[ad] && checkAnotherDay)
                    diff = startTime - time;
                
                if (limit == WITHOUT_TIME_LIMIT || ((diff != -1) && (diff <= limit)))
                {
                    formatTime(endTime, sb);
                    break;
                }
            }
        }
    }
    return sb.str();
}

std::string OpeningHoursParser::BasicOpeningHourRule::toRuleString() const
{
    return toRuleString(stringsHolder.daysStr, stringsHolder.monthsStr);
}

std::string OpeningHoursParser::BasicOpeningHourRule::toLocalRuleString() const
{
    return toRuleString(stringsHolder.localDaysStr, stringsHolder.localMothsStr);
}

std::string OpeningHoursParser::BasicOpeningHourRule::toString() const
{
    return toRuleString();
}

void OpeningHoursParser::BasicOpeningHourRule::appendDaysString(std::stringstream& builder) const
{
    appendDaysString(builder, stringsHolder.daysStr);
}

void OpeningHoursParser::BasicOpeningHourRule::appendDaysString(std::stringstream& builder, const std::vector<std::string>& daysNames) const
{
    bool dash = false;
    bool first = true;
    for (int i = 0; i < 7; i++)
    {
        if (_days[i])
        {
            if (i > 0 && _days[i - 1] && i < 6 && _days[i + 1])
            {
                if (!dash)
                {
                    dash = true;
                    builder << "-";
                }
                continue;
            }
            if (first)
                first = false;
            else if (!dash)
                builder << ", ";
            
            builder << daysNames[getDayIndex(i)];
            dash = false;
        }
    }
    if (_publicHoliday)
    {
        if (!first)
            builder << ", ";
        
        builder << "PH";
        first = false;
    }
    if (_schoolHoliday)
    {
        if (!first)
            builder << ", ";
        
        builder << "SH";
        first = false;
    }
    if (_easter)
    {
        if (!first)
            builder << ", ";
        
        builder << "Easter";
        first = false;
    }
    if (!first)
        builder << " ";
}

void OpeningHoursParser::BasicOpeningHourRule::addTimeRange(int startTime, int endTime)
{
    _startTimes.push_back(startTime);
    _endTimes.push_back(endTime);
}

int OpeningHoursParser::BasicOpeningHourRule::timesSize() const
{
    return (int)_startTimes.size();
}

void OpeningHoursParser::BasicOpeningHourRule::deleteTimeRange(int position)
{
    _startTimes.erase(_startTimes.begin() + position);
    _endTimes.erase(_endTimes.begin() + position);
}

int OpeningHoursParser::BasicOpeningHourRule::calculate(const tm& dateTime) const
{
    int month = dateTime.tm_mon;
    if (!_months[month])
        return 0;
    
    int dmonth = dateTime.tm_mday - 1;
    int i = dateTime.tm_wday + 1;
    int day = (i + 5) % 7;
    int previous = (day + 6) % 7;
    bool thisDay = _days[day] || _dayMonths[dmonth];
    // potential error for Dec 31 12:00-01:00
    bool previousDay = _days[previous] || (dmonth > 0 && _dayMonths[dmonth - 1]);
    if (!thisDay && !previousDay)
        return 0;
    
    int time = dateTime.tm_hour * 60 + dateTime.tm_min; // Time in minutes
    for (i = 0; i < _startTimes.size(); i++)
    {
        int startTime = _startTimes[i];
        int endTime = _endTimes[i];
        if (startTime < endTime || endTime == -1)
        {
            // one day working like 10:00-20:00 (not 20:00-04:00)
            if (time >= startTime && (endTime == -1 || time <= endTime) && thisDay)
            {
                return _off ? -1 : 1;
            }
        }
        else
        {
            // opening_hours includes day wrap like
            // "We 20:00-03:00" or "We 07:00-07:00"
            if (time >= startTime && thisDay)
                return _off ? -1 : 1;
            else if (time < endTime && previousDay)
                return _off ? -1 : 1;
        }
    }
    if (thisDay && (_startTimes.empty() || !_off))
        return -1;
    
    return 0;
}

bool OpeningHoursParser::BasicOpeningHourRule::hasOverlapTimes() const
{
    for (int i = 0; i < _startTimes.size(); i++)
    {
        int startTime = _startTimes[i];
        int endTime = _endTimes[i];
        if (startTime >= endTime && endTime != -1)
            return true;
    }
    return false;
}

OpeningHoursParser::UnparseableRule::UnparseableRule(const std::string& ruleString)
: _ruleString(ruleString)
{
}

OpeningHoursParser::UnparseableRule::~UnparseableRule()
{
}


bool OpeningHoursParser::UnparseableRule::isOpenedForTime(const tm& dateTime, bool checkPrevious) const
{
    return false;
}

bool OpeningHoursParser::UnparseableRule::containsPreviousDay(const tm& dateTime) const
{
    return false;
}

bool OpeningHoursParser::UnparseableRule::hasOverlapTimes() const
{
    return false;
}

bool OpeningHoursParser::UnparseableRule::containsDay(const tm& dateTime) const
{
    return false;
}

bool OpeningHoursParser::UnparseableRule::containsNextDay(const tm& dateTime) const
{
    return false;
}

bool OpeningHoursParser::UnparseableRule::containsMonth(const tm& dateTime) const
{
    return false;
}

bool OpeningHoursParser::UnparseableRule::isOpened24_7() const
{
    return false;
}

std::string OpeningHoursParser::UnparseableRule::toRuleString() const
{
    return _ruleString;
}
std::string OpeningHoursParser::UnparseableRule::toLocalRuleString() const
{
    return toRuleString();
}

std::string OpeningHoursParser::UnparseableRule::toString() const
{
    return toRuleString();
}

std::string OpeningHoursParser::UnparseableRule::getTime(const tm& dateTime, bool checkAnotherDay, int limit, bool opening) const
{
    return "";
}

bool OpeningHoursParser::UnparseableRule::isOpenedForTime(const tm& dateTime) const
{
    return false;
}

bool OpeningHoursParser::UnparseableRule::contains(const tm& dateTime) const
{
    return false;
}

OpeningHoursParser::OpeningHours::OpeningHours(std::vector<std::shared_ptr<OpeningHoursRule>>& rules)
: _rules(rules)
{
}

OpeningHoursParser::OpeningHours::OpeningHours()
{
}

OpeningHoursParser::OpeningHours::~OpeningHours()
{
}

void OpeningHoursParser::OpeningHours::addRule(std::shared_ptr<OpeningHoursRule>& r)
{
    _rules.push_back(r);
}

std::vector<std::shared_ptr<OpeningHoursParser::OpeningHoursRule>>& OpeningHoursParser::OpeningHours::getRules()
{
    return _rules;
}

bool OpeningHoursParser::OpeningHours::isOpenedForTimeV2(const tm& dateTime) const
{
    // make exception for overlapping times i.e.
    // (1) Mo 14:00-16:00; Tu off
    // (2) Mo 14:00-02:00; Tu off
    // in (2) we need to check first rule even though it is against specification
    bool overlap = false;
    for (int i = (int)_rules.size() - 1; i >= 0 ; i--)
    {
        const auto r = _rules[i];
        if (r->hasOverlapTimes())
        {
            overlap = true;
            break;
        }
    }
    // start from the most specific rule
    for(int i = (int)_rules.size() - 1; i >= 0 ; i--)
    {
        const auto r = _rules[i];
        if (r->contains(dateTime))
        {
            bool open = r->isOpenedForTime(dateTime);
            if (!open && overlap)
                continue;
            else
                return open;
        }
    }
    return false;
}

bool OpeningHoursParser::OpeningHours::isOpenedForTime(const tm& dateTime) const
{
    /*
     * first check for rules that contain the current day
     * afterwards check for rules that contain the previous
     * day with overlapping times (times after midnight)
     */
    bool isOpenDay = false;
    for (const auto r : _rules)
    {
        if (r->containsDay(dateTime) && r->containsMonth(dateTime))
            isOpenDay = r->isOpenedForTime(dateTime, false);
    }
    bool isOpenPrevious = false;
    for (const auto r : _rules)
    {
        if (r->containsPreviousDay(dateTime) && r->containsMonth(dateTime))
            isOpenPrevious = r->isOpenedForTime(dateTime, true);
    }
    return isOpenDay || isOpenPrevious;
}

bool OpeningHoursParser::OpeningHours::isOpened24_7() const
{
    bool opened24_7 = false;
    for (const auto r : _rules)
    {
        opened24_7 = r->isOpened24_7();
    }
    return opened24_7;
}

std::string OpeningHoursParser::OpeningHours::getNearToOpeningTime(const tm& dateTime) const
{
    return getTime(dateTime, LOW_TIME_LIMIT, true);
}

std::string OpeningHoursParser::OpeningHours::getOpeningTime(const tm& dateTime) const
{
    return getTime(dateTime, HIGH_TIME_LIMIT, true);
}

std::string OpeningHoursParser::OpeningHours::getNearToClosingTime(const tm& dateTime) const
{
    return getTime(dateTime, LOW_TIME_LIMIT, false);
}

std::string OpeningHoursParser::OpeningHours::getClosingTime(const tm& dateTime) const
{
    return getTime(dateTime, WITHOUT_TIME_LIMIT, false);
}

std::string OpeningHoursParser::OpeningHours::getOpeningDay(const tm& dateTime) const
{
    struct tm cal;
    memcpy(&cal, &dateTime, sizeof(cal));
    
    std::string openingTime("");
    for (int i = 0; i < 7; i++)
    {
        cal.tm_mday += 1;
        for (const auto r : _rules)
        {
            if (r->containsDay(cal) && r->containsMonth(cal))
                openingTime = r->getTime(cal, false, WITHOUT_TIME_LIMIT, true);
        }
        if (!openingTime.empty())
        {
            openingTime += " " + stringsHolder.localDaysStr[cal.tm_wday];
            break;
        }
    }
    return openingTime;
}

std::string OpeningHoursParser::OpeningHours::getTime(const tm& dateTime, int limit, bool opening) const
{
    std::string time = getTimeDay(dateTime, limit, opening);
    if (time.empty())
        time = getTimeAnotherDay(dateTime, limit, opening);
    
    return time;
}

std::string OpeningHoursParser::OpeningHours::getTimeDay(const tm& dateTime, int limit, bool opening) const
{
    std::string atTime = "";
    for (const auto r : _rules)
    {
        if (r->containsDay(dateTime) && r->containsMonth(dateTime))
            atTime = r->getTime(dateTime, false, limit, opening);
    }
    return atTime;
}

std::string OpeningHoursParser::OpeningHours::getTimeAnotherDay(const tm& dateTime, int limit, bool opening) const
{
    std::string atTime = "";
    for (const auto r : _rules)
    {
        if (((opening && r->containsPreviousDay(dateTime)) || (!opening && r->containsNextDay(dateTime))) && r->containsMonth(dateTime))
            atTime = r->getTime(dateTime, true, limit, opening);
    }
    return atTime;
}

std::string OpeningHoursParser::OpeningHours::getCurrentRuleTime(const tm& dateTime) const
{
    // make exception for overlapping times i.e.
    // (1) Mo 14:00-16:00; Tu off
    // (2) Mo 14:00-02:00; Tu off
    // in (2) we need to check first rule even though it is against specification
    std::string ruleClosed;
    bool overlap = false;
    for (int i = (int)_rules.size() - 1; i >= 0; i--)
    {
        const auto r = _rules[i];
        if (r->hasOverlapTimes())
        {
            overlap = true;
            break;
        }
    }
    // start from the most specific rule
    for (int i = (int)_rules.size() - 1; i >= 0; i--)
    {
        const auto r = _rules[i];
        if (r->contains(dateTime))
        {
            bool open = r->isOpenedForTime(dateTime);
            if (!open && overlap)
                ruleClosed = r->toLocalRuleString();
            else
                return r->toLocalRuleString();
        }
    }
    return ruleClosed;
}

std::string OpeningHoursParser::OpeningHours::getCurrentRuleTimeV1(const tm& dateTime) const
{
    std::string ruleOpen;
    std::string ruleClosed;
    for (const auto r : _rules)
    {
        if (r->containsPreviousDay(dateTime) && r->containsMonth(dateTime))
        {
            if (r->isOpenedForTime(dateTime, true))
                ruleOpen = r->toLocalRuleString();
            else
                ruleClosed = r->toLocalRuleString();
        }
    }
    for (const auto r : _rules)
    {
        if (r->containsDay(dateTime) && r->containsMonth(dateTime))
        {
            if (r->isOpenedForTime(dateTime, false))
                ruleOpen = r->toLocalRuleString();
            else
                ruleClosed = r->toLocalRuleString();
        }
    }
    
    if (!ruleOpen.empty())
        return ruleOpen;

    return ruleClosed;
}

std::string OpeningHoursParser::OpeningHours::toString() const
{
    std::stringstream s;
    if (_rules.empty())
        return "";
    
    for (const auto r : _rules)
        s << r->toString() << "; ";
    
    std::string res = s.str();
    return res.substr(0, res.length() - 2);
}

std::string OpeningHoursParser::OpeningHours::toLocalString() const
{
    std::stringstream s;
    if (_rules.empty())
        return "";
    
    for (const auto r : _rules)
        s << r->toLocalRuleString() << "; ";
    
    std::string res = s.str();
    return res.substr(0, res.length() - 2);
}

void OpeningHoursParser::OpeningHours::setOriginal(std::string original)
{
    _original = original;
}

std::string OpeningHoursParser::OpeningHours::getOriginal() const
{
    return _original;
}

OpeningHoursParser::Token::Token(TokenType tokenType, const std::string& string)
: mainNumber(-1)
{
    type = tokenType;
    text = string;
    mainNumber = atoi(string.c_str());
}

OpeningHoursParser::Token::~Token()
{
}


std::string OpeningHoursParser::Token::toString() const
{
    return text + " [" + std::to_string((int)type) + "] ";
}

void replaceString(std::string& s, const std::string& oldString, const std::string& newString)
{
    size_t f = s.find(oldString);
    if (f != std::string::npos)
        s.replace(f, oldString.length(), newString);
}

void replaceStringAll(std::string& s, const std::string& oldString, const std::string& newString)
{
    size_t f;
    do
    {
        f = s.find(oldString);
        if (f != std::string::npos)
            s.replace(f, oldString.length(), newString);
    }
    while (f != std::string::npos);
}

void OpeningHoursParser::findInArray(std::shared_ptr<Token>& t, const std::vector<std::string>& list, TokenType tokenType)
{
    for (int i = 0; i < (int)list.size(); i++)
    {
        if (list[i] == t->text)
        {
            t->type = tokenType;
            t->mainNumber = i;
            break;
        }
    }
}

std::shared_ptr<OpeningHoursParser::OpeningHoursRule> OpeningHoursParser::parseRuleV2(const std::string& rl)
{
    std::string r = to_lowercase(rl);
    
    std::vector<std::string> daysStr = { "mo", "tu", "we", "th", "fr", "sa", "su" };
    std::vector<std::string> monthsStr = { "jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec" };
    std::vector<std::string> holidayStr = { "ph", "sh", "easter" };
    std::string sunrise = "07:00";
    std::string sunset = "21:00";
    std::string endOfDay = "24:00";
    replaceString(r, "(", " "); // avoid "(mo-su 17:00-20:00"
    replaceString(r, ")", " ");
    replaceStringAll(r, "sunset", sunset);
    replaceStringAll(r, "sunrise", sunrise);
    replaceStringAll(r, "+", "-" + endOfDay);
    std::string localRuleString = r;
    
    auto basic = std::make_shared<BasicOpeningHourRule>();
    auto& days = basic->getDays();
    auto& months = basic->getMonths();
    auto& dayMonths = basic->getDayMonths();
    if ("24/7" == localRuleString)
    {
        std::fill(days.begin(), days.end(), true);
        std::fill(months.begin(), months.end(), true);
        basic->addTimeRange(0, 24 * 60);
        return basic;
    }
    std::vector<std::shared_ptr<Token>> tokens;
    int startWord = 0;
    for(int i = 0; i <= localRuleString.length(); i++)
    {
        unsigned char ch = i == localRuleString.length() ? ' ' : localRuleString.at(i);
        bool delimiter = false;
        std::shared_ptr<Token> del = nullptr;
        if (std::isspace(ch))
            delimiter = true;
        else if (ch == ':')
            del = std::make_shared<Token>(TokenType::TOKEN_COLON, ":");
        else if (ch == '-')
            del = std::make_shared<Token>(TokenType::TOKEN_DASH, "-");
        else if (ch == ',')
            del = std::make_shared<Token>(TokenType::TOKEN_COMMA, ",");

        if (delimiter || del != nullptr)
        {
            std::string wrd = trim(localRuleString.substr(startWord, i - startWord));
            if (wrd.length() > 0)
                tokens.push_back(std::make_shared<Token>(TokenType::TOKEN_UNKNOWN, wrd));
            
            startWord = i + 1;
            if (del != nullptr)
                tokens.push_back(del);
        }
    }
    // recognize day of week
    for (auto& t : tokens)
    {
        if (t->type == TokenType::TOKEN_UNKNOWN)
            findInArray(t, daysStr, TokenType::TOKEN_DAY_WEEK);
        
        if (t->type == TokenType::TOKEN_UNKNOWN)
            findInArray(t, monthsStr, TokenType::TOKEN_MONTH);
        
        if (t->type == TokenType::TOKEN_UNKNOWN)
            findInArray(t, holidayStr, TokenType::TOKEN_HOLIDAY);
        
        if (t->type == TokenType::TOKEN_UNKNOWN && ("off" == t->text || "closed" == t->text))
        {
            t->type = TokenType::TOKEN_OFF_ON;
            t->mainNumber = 0;
        }
        if (t->type == TokenType::TOKEN_UNKNOWN && ("24/7" == t->text || "open" == t->text))
        {
            t->type = TokenType::TOKEN_OFF_ON;
            t->mainNumber = 1;
        }
    }
    // recognize hours minutes ( Dec 25: 08:30-20:00)
    for (int i = (int)tokens.size() - 1; i >= 0; i --)
    {
        if (tokens[i]->type == TokenType::TOKEN_COLON)
        {
            if (i > 0 && i < (int)tokens.size() - 1)
            {
                if (tokens[i - 1]->type == TokenType::TOKEN_UNKNOWN && tokens[i - 1]->mainNumber != -1 &&
                    tokens[i + 1]->type == TokenType::TOKEN_UNKNOWN && tokens[i + 1]->mainNumber != -1 ) {
                    tokens[i]->mainNumber = 60 * tokens[i - 1]->mainNumber + tokens[i + 1]->mainNumber;
                    tokens[i]->type = TokenType::TOKEN_HOUR_MINUTES;
                    tokens.erase(tokens.begin() + (i + 1));
                    tokens.erase(tokens.begin() + (i - 1));
                }
            }
        }
    }
    // recognize other numbers
    // if there is no on/off and minutes/hours
    bool hoursSpecified = false;
    for (int i = 0; i < tokens.size(); i ++)
    {
        if (tokens[i]->type == TokenType::TOKEN_HOUR_MINUTES ||
            tokens[i]->type == TokenType::TOKEN_OFF_ON)
        {
            hoursSpecified = true;
            break;
        }
    }
    for (int i = 0; i < (int)tokens.size(); i ++)
    {
        if (tokens[i]->type == TokenType::TOKEN_UNKNOWN && tokens[i]->mainNumber >= 0)
        {
            tokens[i]->type = hoursSpecified ? TokenType::TOKEN_DAY_MONTH : TokenType::TOKEN_HOUR_MINUTES;
            if (tokens[i]->type == TokenType::TOKEN_HOUR_MINUTES)
                tokens[i]->mainNumber = tokens[i]->mainNumber * 60;
            else
                tokens[i]->mainNumber = tokens[i]->mainNumber - 1;
        }
    }
    // order MONTH MONTH_DAY DAY_WEEK HOUR_MINUTE OPEN_OFF
    TokenType currentParse = TokenType::TOKEN_UNKNOWN;
    std::vector<std::shared_ptr<std::vector<std::shared_ptr<Token>>>> listOfPairs;
    std::set<TokenType> presentTokens;
    
    auto currentPair = std::shared_ptr<std::vector<std::shared_ptr<Token>>>(new std::vector<std::shared_ptr<Token>>({ nullptr, nullptr }));
    listOfPairs.push_back(currentPair);
    int indexP = 0;
    for (int i = 0; i <= tokens.size(); i++)
    {
        const auto& t = i == tokens.size() ? nullptr : tokens[i];
        if (t == nullptr || getTokenTypeOrd(t->type) > getTokenTypeOrd(currentParse))
        {
            presentTokens.insert(currentParse);
            // case tokens[i]->type.ordinal() < currentParse.ordinal() - not supported (Fr 15:00-18:00, Sa 16-18)
            if (currentParse == TokenType::TOKEN_MONTH || currentParse == TokenType::TOKEN_DAY_MONTH || currentParse == TokenType::TOKEN_DAY_WEEK || currentParse == TokenType::TOKEN_HOLIDAY)
            {
                auto& array = (currentParse == TokenType::TOKEN_MONTH) ? basic->getMonths() : (currentParse == TokenType::TOKEN_DAY_MONTH) ? basic->getDayMonths() : basic->getDays();
                for (auto pair : listOfPairs)
                {
                    if (pair->at(0) != nullptr && pair->at(1) != nullptr)
                    {
                        if (pair->at(0)->mainNumber <= pair->at(1)->mainNumber)
                        {
                            for (int j = pair->at(0)->mainNumber; j <= pair->at(1)->mainNumber && j < array.size(); j++)
                                array[j] = true;
                        }
                        else
                        {
                            // overflow
                            for (int j = pair->at(0)->mainNumber; j < array.size(); j++)
                                array[j] = true;
                            
                            for (int j = 0; j <= pair->at(1)->mainNumber; j++)
                                array[j] = true;
                        }
                    }
                    else if (pair->at(0) != nullptr)
                    {
                        if (pair->at(0)->type == TokenType::TOKEN_HOLIDAY)
                        {
                            if (pair->at(0)->mainNumber == 0)
                                basic->setPublicHolidays(true);
                            else if (pair->at(0)->mainNumber == 1)
                                basic->setSchoolHolidays(true);
                            else if(pair->at(0)->mainNumber == 2)
                                basic->setEaster(true);
                        }
                        else if (pair->at(0)->mainNumber >= 0)
                        {
                            array[pair->at(0)->mainNumber] = true;
                        }
                    }
                }
            }
            else if (currentParse == TokenType::TOKEN_HOUR_MINUTES)
            {
                for (auto pair : listOfPairs)
                {
                    if (pair->at(0) != nullptr && pair->at(1) != nullptr)
                        basic->addTimeRange(pair->at(0)->mainNumber, pair->at(1)->mainNumber);
                }
            }
            else if (currentParse == TokenType::TOKEN_OFF_ON)
            {
                auto l = listOfPairs[0];
                if (l->at(0) != nullptr && l->at(0)->mainNumber == 0)
                    basic->setOff(true);
            }
            listOfPairs.clear();

            currentPair = std::shared_ptr<std::vector<std::shared_ptr<Token>>>(new std::vector<std::shared_ptr<Token>>({ nullptr, nullptr }));
            //currentPair = { nullptr, nullptr };
            indexP = 0;
            listOfPairs.push_back(currentPair);
            currentPair->insert(currentPair->begin() + indexP++, t);
            if (t != nullptr)
                currentParse = t->type;
        }
        else if (t->type == TokenType::TOKEN_COMMA)
        {
            currentPair = std::shared_ptr<std::vector<std::shared_ptr<Token>>>(new std::vector<std::shared_ptr<Token>>({ nullptr, nullptr }));
            //currentPair = { nullptr, nullptr };
            indexP = 0;
            listOfPairs.push_back(currentPair);
        }
        else if (t->type == TokenType::TOKEN_DASH)
        {
        }
        else if (getTokenTypeOrd(t->type) == getTokenTypeOrd(currentParse))
        {
            if (indexP < 2)
                currentPair->insert(currentPair->begin() + indexP++, t);
        }
    }
    if (presentTokens.find(TokenType::TOKEN_MONTH) == presentTokens.end())
    {
        auto& months = basic->getMonths();
        std::fill(months.begin(), months.end(), true);
    }
    //        if(!presentTokens.contains(TokenType::TOKEN_DAY_MONTH)) {
    //            Arrays.fill(basic.getDayMonths(), true);
    //        }
    if (presentTokens.find(TokenType::TOKEN_DAY_WEEK) == presentTokens.end() && presentTokens.find(TokenType::TOKEN_HOLIDAY) == presentTokens.end() && presentTokens.find(TokenType::TOKEN_DAY_MONTH) == presentTokens.end())
    {
        auto& days = basic->getDays();
        std::fill(days.begin(), days.end(), true);
    }
    //        if(!presentTokens.contains(TokenType::TOKEN_HOUR_MINUTES)) {
    //            basic.addTimeRange(0, 24 * 60);
    //        }
    //        System.out.println(r + " " +  tokens);
    return basic;
}

OpeningHoursParser::OpeningHoursParser(const std::string& openingHours) : openingHours(openingHours)
{
}

OpeningHoursParser::~OpeningHoursParser()
{
}

/**
 * Parse an opening_hours string from OSM to an OpeningHours object which can be used to check
 *
 * @param r the string to parse
 * @return BasicRule if the String is successfully parsed and UnparseableRule otherwise
 */
std::shared_ptr<OpeningHoursParser::OpeningHoursRule> OpeningHoursParser::parseRule(const std::string& rl)
{
    return parseRuleV2(rl);
}

/**
 * parse OSM opening_hours string to an OpeningHours object
 *
 * @param format the string to parse
 * @return null when parsing was unsuccessful
 */
std::shared_ptr<OpeningHoursParser::OpeningHours> OpeningHoursParser::parseOpenedHours(const std::string& format)
{
    if (format.empty())
        return nullptr;
    
    // split the OSM string in multiple rules
    auto rules = split_string(format, ";");
    // FIXME: What if the semicolon is inside a quoted string?
    auto rs = std::make_shared<OpeningHours>();
    rs->setOriginal(format);
    for (auto& r : rules)
    {
        r = trim(r);
        if (r.length() == 0)
            continue;
        
        // check if valid
        auto r1 = parseRule(r);
        if (std::dynamic_pointer_cast<const BasicOpeningHourRule>(r1))
            rs->addRule(r1);
    }
    return rs;
}

/**
 * test if the calculated opening hours are what you expect
 *
 * @param time     the time to test in the format "dd.MM.yyyy HH:mm"
 * @param hours    the OpeningHours object
 * @param expected the expected state
 */
void OpeningHoursParser::testOpened(const std::string& time, const std::shared_ptr<OpeningHours>& hours, bool expected)
{
    tm date = {0};
    std::istringstream ss(time);
    ss >> get_time(&date, "%d.%m.%Y %H:%M");
    if (ss.fail())
    {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "!!! Cannot parse date: %s", time.c_str());
        return;
    }
    std::mktime(&date);
    
    bool calculated = hours->isOpenedForTimeV2(date);
    auto currentRuleTime = hours->getCurrentRuleTime(date);
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning,
                      "%sok: Expected %s: %s = %s (rule %s)",
                      (calculated != expected) ? "NOT " : "", time.c_str(), expected ? "true" : "false", calculated ? "true" : "false", currentRuleTime.c_str());

    if (calculated != expected)
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "BUG!!!");
}

void OpeningHoursParser::testParsedAndAssembledCorrectly(const std::string& timeString, const std::shared_ptr<OpeningHours>& hours)
{
    auto assembledString = hours->toString();
    bool isCorrect = to_lowercase(assembledString) == to_lowercase(timeString);
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning,
                      "%sok: Expected: \"%s\" got: \"%s\"",
                      (!isCorrect ? "NOT " : ""), timeString.c_str(), assembledString.c_str());
    if (!isCorrect)
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "BUG!!!");
}

/**
 * parse OSM opening_hours string to an OpeningHours object.
 * Does not return null when parsing unsuccessful. When parsing rule is unsuccessful,
 * such rule is stored as UnparseableRule.
 *
 * @param format the string to parse
 * @return the OpeningHours object
 */
std::shared_ptr<OpeningHoursParser::OpeningHours> OpeningHoursParser::parseOpenedHoursHandleErrors(const std::string& format)
{
    if (format.empty())
        return nullptr;

    const auto& rules = split_string(format, ";");
    auto rs = std::make_shared<OpeningHours>();
    rs->setOriginal(format);
    for (const auto& rl : rules)
    {
        const auto& r = trim(rl);
        if (r.length() == 0)
            continue;
        
        // check if valid
        auto rule = parseRule(r);
        rs->addRule(rule);
    }
    return rs;
}


void OpeningHoursParser::runTest()
{
    // 0. not supported MON DAY-MON DAY (only supported Feb 2-14 or Feb-Oct: 09:00-17:30)
    // parseOpenedHours("Feb 16-Oct 15: 09:00-18:30; Oct 16-Nov 15: 09:00-17:30; Nov 16-Feb 15: 09:00-16:30");
    
    // 1. not supported (,)
    // hours = parseOpenedHours("Mo-Su 07:00-23:00, Fr 08:00-20:00");
    
    // 2. not supported break properly
    // parseOpenedHours("Sa-Su 10:00-17:00 || \"by appointment\"");
    // comment is dropped
    
    // 3. not properly supported
    // hours = parseOpenedHours("Mo-Su (sunrise-00:30)-(sunset+00:30)");
    
    // Test basic case
    auto hours = parseOpenedHours("Mo-Fr 08:30-14:40");
    testOpened("09.08.2012 11:00", hours, true);
    testOpened("09.08.2012 16:00", hours, false);
    hours = parseOpenedHours("mo-fr 07:00-19:00; sa 12:00-18:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    
    std::string string = "Mo-Fr 11:30-15:00, 17:30-23:00; Sa, Su, PH 11:30-23:00";
    hours = parseOpenedHours(string);
    testParsedAndAssembledCorrectly(string, hours);
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("7.09.2015 14:54", hours, true); // monday
    testOpened("7.09.2015 15:05", hours, false);
    testOpened("6.09.2015 16:05", hours, true);
    
    
    // two time and date ranges
    hours = parseOpenedHours("Mo-We, Fr 08:30-14:40,15:00-19:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("08.08.2012 14:00", hours, true);
    testOpened("08.08.2012 14:50", hours, false);
    testOpened("10.08.2012 15:00", hours, true);
    
    // test exception on general schema
    hours = parseOpenedHours("Mo-Sa 08:30-14:40; Tu 08:00 - 14:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("07.08.2012 14:20", hours, false);
    testOpened("07.08.2012 08:15", hours, true); // Tuesday
    
    // test off value
    hours = parseOpenedHours("Mo-Sa 09:00-18:25; Th off");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("08.08.2012 12:00", hours, true);
    testOpened("09.08.2012 12:00", hours, false);
    
    // test 24/7
    hours = parseOpenedHours("24/7");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("08.08.2012 23:59", hours, true);
    testOpened("08.08.2012 12:23", hours, true);
    testOpened("08.08.2012 06:23", hours, true);
    
    // some people seem to use the following syntax:
    hours = parseOpenedHours("Sa-Su 24/7");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    hours = parseOpenedHours("Mo-Fr 9-19");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    hours = parseOpenedHours("09:00-17:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    hours = parseOpenedHours("sunrise-sunset");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    hours = parseOpenedHours("10:00+");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    hours = parseOpenedHours("Su-Th sunset-24:00, 04:00-sunrise; Fr-Sa sunset-sunrise");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("12.08.2012 04:00", hours, true);
    testOpened("12.08.2012 23:00", hours, true);
    testOpened("08.08.2012 12:00", hours, false);
    testOpened("08.08.2012 05:00", hours, true);
    
    // test simple day wrap
    hours = parseOpenedHours("Mo 20:00-02:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("05.05.2013 10:30", hours, false);
    testOpened("05.05.2013 23:59", hours, false);
    testOpened("06.05.2013 10:30", hours, false);
    testOpened("06.05.2013 20:30", hours, true);
    testOpened("06.05.2013 23:59", hours, true);
    testOpened("07.05.2013 00:00", hours, true);
    testOpened("07.05.2013 00:30", hours, true);
    testOpened("07.05.2013 01:59", hours, true);
    testOpened("07.05.2013 20:30", hours, false);
    
    // test maximum day wrap
    hours = parseOpenedHours("Su 10:00-10:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("05.05.2013 09:59", hours, false);
    testOpened("05.05.2013 10:00", hours, true);
    testOpened("05.05.2013 23:59", hours, true);
    testOpened("06.05.2013 00:00", hours, true);
    testOpened("06.05.2013 09:59", hours, true);
    testOpened("06.05.2013 10:00", hours, false);
    
    // test day wrap as seen on OSM
    hours = parseOpenedHours("Tu-Th 07:00-2:00; Fr 17:00-4:00; Sa 18:00-05:00; Su,Mo off");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("05.05.2013 04:59", hours, true); // sunday 05.05.2013
    testOpened("05.05.2013 05:00", hours, false);
    testOpened("05.05.2013 12:30", hours, false);
    testOpened("06.05.2013 10:30", hours, false);
    testOpened("07.05.2013 01:00", hours, false);
    testOpened("07.05.2013 20:25", hours, true);
    testOpened("07.05.2013 23:59", hours, true);
    testOpened("08.05.2013 00:00", hours, true);
    testOpened("08.05.2013 02:00", hours, false);
    
    // test day wrap as seen on OSM
    hours = parseOpenedHours("Mo-Th 09:00-03:00; Fr-Sa 09:00-04:00; Su off");
    testOpened("11.05.2015 08:59", hours, false);
    testOpened("11.05.2015 09:01", hours, true);
    testOpened("12.05.2015 02:59", hours, true);
    testOpened("12.05.2015 03:00", hours, false);
    testOpened("16.05.2015 03:59", hours, true);
    testOpened("16.05.2015 04:01", hours, false);
    testOpened("17.05.2015 01:00", hours, true);
    testOpened("17.05.2015 04:01", hours, false);
    
    hours = parseOpenedHours("Tu-Th 07:00-2:00; Fr 17:00-4:00; Sa 18:00-05:00; Su,Mo off");
    testOpened("11.05.2015 08:59", hours, false);
    testOpened("11.05.2015 09:01", hours, false);
    testOpened("12.05.2015 01:59", hours, false);
    testOpened("12.05.2015 02:59", hours, false);
    testOpened("12.05.2015 03:00", hours, false);
    testOpened("13.05.2015 01:59", hours, true);
    testOpened("13.05.2015 02:59", hours, false);
    testOpened("16.05.2015 03:59", hours, true);
    testOpened("16.05.2015 04:01", hours, false);
    testOpened("17.05.2015 01:00", hours, true);
    testOpened("17.05.2015 05:01", hours, false);
    
    // tests single month value
    hours = parseOpenedHours("May: 07:00-19:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("05.05.2013 12:00", hours, true);
    testOpened("05.05.2013 05:00", hours, false);
    testOpened("05.05.2013 21:00", hours, false);
    testOpened("05.01.2013 12:00", hours, false);
    testOpened("05.01.2013 05:00", hours, false);
    
    // tests multi month value
    hours = parseOpenedHours("Apr-Sep 8:00-22:00; Oct-Mar 10:00-18:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("05.03.2013 15:00", hours, true);
    testOpened("05.03.2013 20:00", hours, false);
    
    testOpened("05.05.2013 20:00", hours, true);
    testOpened("05.05.2013 23:00", hours, false);
    
    testOpened("05.10.2013 15:00", hours, true);
    testOpened("05.10.2013 20:00", hours, false);
    
    // Test time with breaks
    hours = parseOpenedHours("Mo-Fr: 9:00-13:00, 14:00-18:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("02.12.2015 12:00", hours, true);
    testOpened("02.12.2015 13:30", hours, false);
    testOpened("02.12.2015 16:00", hours, true);
    
    testOpened("05.12.2015 16:00", hours, false);
    
    hours = parseOpenedHours("Mo-Su 07:00-23:00; Dec 25 08:00-20:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("25.12.2015 07:00", hours, false);
    testOpened("24.12.2015 07:00", hours, true);
    testOpened("24.12.2015 22:00", hours, true);
    testOpened("25.12.2015 08:00", hours, true);
    testOpened("25.12.2015 22:00", hours, false);
    
    hours = parseOpenedHours("Mo-Su 07:00-23:00; Dec 25 off");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("25.12.2015 14:00", hours, false);
    testOpened("24.12.2015 08:00", hours, true);
    
    // easter itself as public holiday is not supported
    hours = parseOpenedHours("Mo-Su 07:00-23:00; Easter off; Dec 25 off");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("25.12.2015 14:00", hours, false);
    testOpened("24.12.2015 08:00", hours, true);
    
    // test time off (not days
    hours = parseOpenedHours("Mo-Fr 08:30-17:00; 12:00-12:40 off;");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("07.05.2017 14:00", hours, false); // Sunday
    testOpened("06.05.2017 12:15", hours, false); // Saturday
    testOpened("05.05.2017 14:00", hours, true); // Friday
    testOpened("05.05.2017 12:15", hours, false);
    testOpened("05.05.2017 12:00", hours, false);
    testOpened("05.05.2017 11:45", hours, true);
    
    // Test holidays
    std::string hoursString = "mo-fr 11:00-21:00; PH off";
    hours = parseOpenedHoursHandleErrors(hoursString);
    testParsedAndAssembledCorrectly(hoursString, hours);
}

