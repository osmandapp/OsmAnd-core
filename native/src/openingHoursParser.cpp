#include "openingHoursParser.h"

#include <stdlib.h>
#include <set>
#include <cstring>

#include "Logging.h"

static const int LOW_TIME_LIMIT = 120;
static const int WITHOUT_TIME_LIMIT = -1;
static const int CURRENT_DAY_TIME_LIMIT = -2;

static StringsHolder stringsHolder;

inline std::tm ohp_localtime(const std::time_t& time)
{
    std::tm tm_snapshot;
#if (defined(WIN32) || defined(_WIN32) || defined(__WIN32__))
    localtime_s(&tm_snapshot, &time);
#else
    localtime_r(&time, &tm_snapshot); // POSIX
#endif
    return tm_snapshot;
}

template < typename T > std::string ohp_to_string( const T& n )
{
    std::ostringstream stm ;
    stm << n ;
    return stm.str() ;
}

std::string ohp_to_lowercase(const std::string& in)
{
    std::string out(in);
    for (uint i = 0; i < in.length(); i++) {
        out[i] = std::tolower(in[i]);
    }
    return out;
}

std::vector<std::string> ohp_split_string( const std::string& str, const std::string& delimiters)
{
    std::vector<std::string> tokens;
    std::string::size_type pos, lastPos = 0, length = str.length();
    
    while (lastPos < length + 1) {
        pos = str.find_first_of(delimiters, lastPos);
        if (pos == std::string::npos) {
            pos = length;
        }
        if (pos != lastPos)
            tokens.push_back(str.substr(lastPos, pos - lastPos));
        
        lastPos = pos + 1;
    }
    return tokens;
}

const static char* ohp_trim_chars = " \t\n\r\f\v";

std::string ohp_rtrim(const std::string& in, const char* t = ohp_trim_chars) {
    std::string s(in);
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

std::string ohp_ltrim(const std::string& in, const char* t = ohp_trim_chars) {
    std::string s(in);
    s.erase(0, s.find_first_not_of(t));
    return s;
}

std::string ohp_trim(const std::string& in, const char* t = ohp_trim_chars) {
    return ohp_ltrim(ohp_rtrim(in, t), t);
}

std::vector<std::string> getTwoLettersStringArray(const std::vector<std::string>& strings)
{
    std::vector<std::string> newStrings;
    for (const auto& s : strings)
    {
        std::string str = s;
        if (str.length() > 0)
            str[0] = std::toupper(str[0]);
        
        if (s.length() > 3)
            newStrings.push_back(str.substr(0, 3));
        else
            newStrings.push_back(str);
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

    std::string hs = ohp_to_string(h);
    b << hs;
    b << ":";
    if (t < 10)
        b << "0";

    std::string ts = ohp_to_string(t);
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
    
    additionalStrings["off"] = "off";
    additionalStrings["is_open"] = "Open";
    additionalStrings["is_open_24_7"] = "Open 24/7";
    additionalStrings["will_open_at"] = "Will open at";
    additionalStrings["open_from"] = "Open from";
    additionalStrings["will_close_at"] = "Will close at";
    additionalStrings["open_till"] = "Open till";
    additionalStrings["will_open_tomorrow_at"] = "Will open tomorrow at";
    additionalStrings["will_open_on"] = "Will open on";
}

StringsHolder::~StringsHolder()
{
}

/**
 * Set additional localized strings like "off", etc.
 */
void StringsHolder::setAdditionalString(const std::string& key, const std::string& value)
{
    additionalStrings[key] = value;
}

OpeningHoursParser::BasicOpeningHourRule::BasicOpeningHourRule()
: _sequenceIndex(0)
{
    init();
}

OpeningHoursParser::BasicOpeningHourRule::BasicOpeningHourRule(int sequenceIndex)
: _sequenceIndex(sequenceIndex)
{
    init();
}

OpeningHoursParser::BasicOpeningHourRule::~BasicOpeningHourRule()
{
}

void OpeningHoursParser::BasicOpeningHourRule::init()
{
    _hasDays = false;
    for (int i = 0; i < 7; i++)
        _days.push_back(false);
    
    for (int i = 0; i < 12; i++)
        _months.push_back(false);
    
    _hasDayMonths = false;
    for (int i = 0; i < 12; i++)
    {
        std::vector<bool> days;
        for (int k = 0; k < 31; k++)
            days.push_back(false);

        _dayMonths.push_back(days);
    }
    
    _publicHoliday = false;
    _schoolHoliday = false;
    _easter = false;
    _off = false;
}

int OpeningHoursParser::BasicOpeningHourRule::getSequenceIndex() const
{
    return _sequenceIndex;
}

std::vector<bool>& OpeningHoursParser::BasicOpeningHourRule::getDays()
{
    return _days;
}

std::vector<bool>& OpeningHoursParser::BasicOpeningHourRule::getMonths()
{
    return _months;
}

std::vector<std::vector<bool>>& OpeningHoursParser::BasicOpeningHourRule::getDayMonths()
{
    return _dayMonths;
}

std::vector<bool>& OpeningHoursParser::BasicOpeningHourRule::getDayMonths(int month)
{
    return _dayMonths[month];
}

bool OpeningHoursParser::BasicOpeningHourRule::hasDays() const
{
    return _hasDays;
}

void OpeningHoursParser::BasicOpeningHourRule::setHasDays(bool value)
{
    _hasDays = value;
}

bool OpeningHoursParser::BasicOpeningHourRule::hasDayMonths() const
{
    return _hasDayMonths;
}

void OpeningHoursParser::BasicOpeningHourRule::setHasDayMonths(bool value)
{
    _hasDayMonths = value;
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

std::string OpeningHoursParser::BasicOpeningHourRule::getComment() const
{
    return _comment;
}

void OpeningHoursParser::BasicOpeningHourRule::setComment(std::string comment)
{
    _comment = comment;
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
    
    if (opened24_7)
    {
        if (!_startTimes.empty())
        {
            for (int i = 0; i < _startTimes.size(); i++)
            {
                int startTime = _startTimes[i];
                int endTime = _endTimes[i];
                if (startTime == 0 && endTime / 60 == 24)
                    return true;
            }
        }
        else
        {
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
            
            b << (arrayNames.empty() ? ohp_to_string(i + 1) : arrayNames[i]);
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

    bool allDays = !_hasDayMonths;
    if (!allDays)
    {
        bool dash = false;
        bool first = true;
        int monthAdded = -1;
        int excludedMonthEnd = -1;
        int excludedDayEnd = -1;
        int excludedMonthStart = -1;
        int excludedDayStart = -1;
        if (_dayMonths[0][0] && _dayMonths[11][30])
        {
            int prevMonth = 0;
            int prevDay = 0;
            for (int month = 0; month < _dayMonths.size(); month++)
            {
                for (int day = 0; day < _dayMonths[month].size(); day++)
                {
                    if (day == 1)
                        prevMonth = month;
                    
                    if (!_dayMonths[month][day])
                    {
                        excludedMonthEnd = prevMonth;
                        excludedDayEnd = prevDay;
                        break;
                    }
                    prevDay = day;
                }
                if (excludedDayEnd != -1)
                    break;
            }
            prevMonth = _dayMonths.size() - 1;
            prevDay = _dayMonths[prevMonth].size() - 1;
            for (int month = _dayMonths.size() - 1; month >= 0; month--)
            {
                for (int day = _dayMonths[month].size() - 1; day >= 0; day--) {
                    if (day == _dayMonths[month].size() - 2)
                        prevMonth = month;
                    
                    if (!_dayMonths[month][day])
                    {
                        excludedMonthStart = prevMonth;
                        excludedDayStart = prevDay;
                        break;
                    }
                    prevDay = day;
                }
                if (excludedDayStart != -1)
                    break;
            }
        }
        for (int month = 0; month < _dayMonths.size(); month++)
        {
            for (int day = 0; day < _dayMonths[month].size(); day++)
            {
                if (excludedDayStart != -1 && excludedDayEnd != -1)
                {
                    if (month < excludedMonthEnd || (month == excludedMonthEnd && day <= excludedDayEnd))
                        continue;
                    else if (month > excludedMonthStart || (month == excludedMonthStart && day >= excludedDayStart))
                        continue;
                }
                if (_dayMonths[month][day])
                {
                    if (day == 0 && dash)
                        continue;
                    
                    if (day > 0 && _dayMonths[month][day - 1]
                        && ((day < _dayMonths[month].size() - 1 && _dayMonths[month][day + 1]) || (day == _dayMonths[month].size() - 1 && month < _dayMonths.size() - 1 && _dayMonths[month + 1][0])))
                    {
                        if (!dash)
                        {
                            dash = true;
                            if (!first)
                                b << "-";
                        }
                        continue;
                    }
                    if (first)
                    {
                        first = false;
                    }
                    else if (!dash)
                    {
                        b << ", ";
                        monthAdded = -1;
                    }
                    if (monthAdded != month)
                    {
                        b << monthNames[month] << " ";
                        monthAdded = month;
                    }
                    b << ohp_to_string(day + 1);
                    dash = false;
                }
            }
        }
        if (excludedDayStart != -1 && excludedDayEnd != -1)
        {
            if (first)
                first = false;
            else if (!dash)
                b << ", ";
            
            b << monthNames[excludedMonthStart] << " " << ohp_to_string(excludedDayStart + 1);
            b << "-";
            b << monthNames[excludedMonthEnd] << " " << ohp_to_string(excludedDayEnd + 1);
        }
        if (!first)
            b << " ";
    }
    else if (!allMonths)
    {
        addArray(_months, monthNames, b);
    }
    
    // Day
    appendDaysString(b, dayNames);
    // Time
    if (_startTimes.empty())
    {
        if (isOpened24_7())
        {
            b.str("");
            b << "24/7 ";
        }

        if (_off)
            b << stringsHolder.additionalStrings["off"];
    }
    else
    {
        if (isOpened24_7())
        {
            b.str("");
            b << "24/7";
        }
        else
        {
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
        }
        if (_off)
            b << " " << stringsHolder.additionalStrings["off"];
    }
    if (!_comment.empty())
    {
        std::string str = b.str();
        if (str.length() > 0)
        {
            if (str[str.length() - 1] != ' ')
                b << " ";
            
            b << "- " << _comment;
        }
        else
        {
            b << _comment;
        }
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
        if (opening != _off)
        {
            if (startTime < endTime || endTime == -1)
            {
                if (_days[d] && !checkAnotherDay)
                {
                    int diff = startTime - time;
                    if (limit == WITHOUT_TIME_LIMIT || (time <= startTime && (diff <= limit || limit == CURRENT_DAY_TIME_LIMIT)))
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
                
                if (limit == WITHOUT_TIME_LIMIT || ((diff != -1 && diff <= limit) || limit == CURRENT_DAY_TIME_LIMIT))
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
                    if ((limit == WITHOUT_TIME_LIMIT && diff >= 0) || (time <= endTime && diff <= limit))
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
                    diff = endTime - time;
                
                if (limit == WITHOUT_TIME_LIMIT || (diff != -1 && diff <= limit))
                {
                    formatTime(endTime, sb);
                    break;
                }
            }
        }
    }
    std::string res = sb.str();
    if (res.length() > 0 && !_comment.empty())
        res = res + " - " + _comment;
    
    return res;
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
    bool thisDay = _hasDays || _hasDayMonths;
    if (thisDay && _hasDayMonths) {
        thisDay = _dayMonths[month][dmonth];
    }
    if (thisDay && _hasDays) {
        thisDay = _days[day];
    }
    // potential error for Dec 31 12:00-01:00
    bool previousDay = _hasDays || _hasDayMonths;
    if (previousDay && _hasDayMonths && dmonth > 0) {
        previousDay = _dayMonths[month][dmonth - 1];
    }
    if (previousDay && _hasDays) {
        previousDay = _days[previous];
    }
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
    if (thisDay && _startTimes.empty() && !_off)
        return 1;
    else if (thisDay && (_startTimes.empty() || !_off))
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

bool OpeningHoursParser::BasicOpeningHourRule::hasOverlapTimes(const tm& dateTime, const std::shared_ptr<OpeningHoursRule>& r) const
{
    if (_off)
        return true;
    
    if (r != nullptr && r->contains(dateTime))
    {
        const auto& rule = std::static_pointer_cast<BasicOpeningHourRule>(r);
        if (_startTimes.size() > 0 && rule->getStartTimes().size() > 0)
        {
            for (int i = 0; i < _startTimes.size(); i++)
            {
                int startTime = _startTimes[i];
                int endTime = _endTimes[i];
                if (endTime == -1)
                    endTime = 24 * 60;
                else if (startTime >= endTime)
                    endTime = 24 * 60 + endTime;
                
                const auto& rStartTimes = rule->getStartTimes();
                const auto& rEndTimes = rule->getEndTimes();
                for (int k = 0; k < rStartTimes.size(); k++)
                {
                    int rStartTime = rStartTimes[k];
                    int rEndTime = rEndTimes[k];
                    if (rEndTime == -1)
                        rEndTime = 24 * 60;
                    else if (rStartTime >= rEndTime)
                        rEndTime = 24 * 60 + rEndTime;
                    
                    if ((rStartTime >= startTime && rStartTime < endTime) || (startTime >= rStartTime && startTime < rEndTime))
                    {
                        return true;
                    }
                }
            }
        }
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

bool OpeningHoursParser::UnparseableRule::hasOverlapTimes(const tm& dateTime, const std::shared_ptr<OpeningHoursRule>& r) const
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

int OpeningHoursParser::UnparseableRule::getSequenceIndex() const
{
    return 0;
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

OpeningHoursParser::OpeningHours::Info::Info()
: opened(false), opened24_7(false)
{
}

OpeningHoursParser::OpeningHours::Info::~Info()
{
}

std::string OpeningHoursParser::OpeningHours::Info::getInfo()
{
    if (opened24_7)
    {
        if (!ruleString.empty())
            return stringsHolder.additionalStrings["is_open"] + " " + ruleString;
        else
            return stringsHolder.additionalStrings["is_open_24_7"];
    }
    else if (!nearToOpeningTime.empty())
    {
        return stringsHolder.additionalStrings["will_open_at"] + " " + nearToOpeningTime;
    }
    else if (!openingTime.empty())
    {
        return stringsHolder.additionalStrings["open_from"] + " " + openingTime;
    }
    else if (!nearToClosingTime.empty())
    {
        return stringsHolder.additionalStrings["will_close_at"] + " " + nearToClosingTime;
    }
    else if (!closingTime.empty())
    {
        return stringsHolder.additionalStrings["open_till"] + " " + closingTime;
    }
    else if (!openingTomorrow.empty())
    {
        return stringsHolder.additionalStrings["will_open_tomorrow_at"] + " " + openingTomorrow;
    }
    else if (!openingDay.empty())
    {
        return stringsHolder.additionalStrings["will_open_on"] + " " + openingDay + ".";
    }
    else if (!ruleString.empty())
    {
        return ruleString;
    }
    else
    {
        return "";
    }
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

std::vector<std::shared_ptr<OpeningHoursParser::OpeningHours::Info>> OpeningHoursParser::OpeningHours::getInfo()
{
    time_t rawtime;
    time (&rawtime);
    const auto& dateTime = ohp_localtime(rawtime);
    return getInfo(dateTime);
}

std::vector<std::shared_ptr<OpeningHoursParser::OpeningHours::Info>> OpeningHoursParser::OpeningHours::getInfo(const tm& dateTime)
{
    std::vector<std::shared_ptr<Info>> res;
    for (int i = 0; i < _sequenceCount; i++)
    {
        auto info = getInfo(dateTime, i);
        res.push_back(info);
    }
    return res;
}

std::shared_ptr<OpeningHoursParser::OpeningHours::Info> OpeningHoursParser::OpeningHours::getCombinedInfo()
{
    time_t rawtime;
    time (&rawtime);
    const auto& dateTime = ohp_localtime(rawtime);
    return getCombinedInfo(dateTime);
}

std::shared_ptr<OpeningHoursParser::OpeningHours::Info> OpeningHoursParser::OpeningHours::getCombinedInfo(const tm& dateTime)
{
    return getInfo(dateTime, ALL_SEQUENCES);
}

std::shared_ptr<OpeningHoursParser::OpeningHours::Info> OpeningHoursParser::OpeningHours::getInfo(const tm& dateTime, int sequenceIndex) const
{
    auto info = std::make_shared<Info>();
    bool opened = isOpenedForTimeV2(dateTime, sequenceIndex);
    info->opened = opened;
    info->ruleString = getCurrentRuleTime(dateTime, sequenceIndex);
    if (opened) {
        info->opened24_7 = isOpened24_7(sequenceIndex);
        info->closingTime = getClosingTime(dateTime, sequenceIndex);
        info->nearToClosingTime = getNearToClosingTime(dateTime, sequenceIndex);
    } else {
        info->openingTime = getOpeningTime(dateTime, sequenceIndex);
        info->nearToOpeningTime = getNearToOpeningTime(dateTime, sequenceIndex);
        info->openingTomorrow = getOpeningTomorrow(dateTime, sequenceIndex);
        info->openingDay = getOpeningDay(dateTime, sequenceIndex);
    }
    return info;
}

void OpeningHoursParser::OpeningHours::addRule(std::shared_ptr<OpeningHoursRule> r)
{
    _rules.push_back(r);
}

void OpeningHoursParser::OpeningHours::addRules(std::vector<std::shared_ptr<OpeningHoursRule>>& rules)
{
    _rules.insert(_rules.end(), rules.begin(), rules.end());
}

std::vector<std::shared_ptr<OpeningHoursParser::OpeningHoursRule>> OpeningHoursParser::OpeningHours::getRules() const
{
    return _rules;
}

std::vector<std::shared_ptr<OpeningHoursParser::OpeningHoursRule>> OpeningHoursParser::OpeningHours::getRules(int sequenceIndex) const
{
    if (sequenceIndex == ALL_SEQUENCES)
    {
        return _rules;
    }
    else
    {
        std::vector<std::shared_ptr<OpeningHoursParser::OpeningHoursRule>> sequenceRules;
        for (const auto r : _rules)
        {
            if (r->getSequenceIndex() == sequenceIndex)
                sequenceRules.push_back(r);
        }
        return sequenceRules;
    }
}

int OpeningHoursParser::OpeningHours::getSequenceCount() const
{
    return _sequenceCount;
}

void OpeningHoursParser::OpeningHours::setSequenceCount(int sequenceCount)
{
    _sequenceCount = sequenceCount;
}

bool OpeningHoursParser::OpeningHours::isOpenedForTimeV2(const tm& dateTime, int sequenceIndex) const
{
    // make exception for overlapping times i.e.
    // (1) Mo 14:00-16:00; Tu off
    // (2) Mo 14:00-02:00; Tu off
    // in (2) we need to check first rule even though it is against specification
    const auto& rules = getRules(sequenceIndex);
    bool overlap = false;
    for (int i = (int)rules.size() - 1; i >= 0 ; i--)
    {
        const auto r = rules[i];
        if (r->hasOverlapTimes())
        {
            overlap = true;
            break;
        }
    }
    // start from the most specific rule
    for (int i = (int)rules.size() - 1; i >= 0 ; i--)
    {
        bool checkNext = false;
        const auto rule = rules[i];
        if (rule->contains(dateTime))
        {
            if (i > 0)
                checkNext = !rule->hasOverlapTimes(dateTime, rules[i - 1]);
        
            bool open = rule->isOpenedForTime(dateTime);
            if (open || (!overlap && !checkNext)) {
                return open;
            }
        }
    }
    return false;
}

bool OpeningHoursParser::OpeningHours::isOpened() const
{
    time_t rawtime;
    time (&rawtime);
    const auto& dateTime = ohp_localtime(rawtime);
    
    return isOpenedForTimeV2(dateTime, ALL_SEQUENCES);
}

bool OpeningHoursParser::OpeningHours::isOpenedForTime(const tm& dateTime) const
{
    return isOpenedForTimeV2(dateTime, ALL_SEQUENCES);
}

bool OpeningHoursParser::OpeningHours::isOpened24_7(int sequenceIndex) const
{
    bool opened24_7 = false;
    const auto& rules = getRules(sequenceIndex);
    for (const auto r : rules)
    {
        opened24_7 = r->isOpened24_7();
    }
    return opened24_7;
}

std::string OpeningHoursParser::OpeningHours::getNearToOpeningTime(const tm& dateTime, int sequenceIndex) const
{
    return getTime(dateTime, LOW_TIME_LIMIT, true, sequenceIndex);
}

std::string OpeningHoursParser::OpeningHours::getOpeningTime(const tm& dateTime, int sequenceIndex) const
{
    return getTime(dateTime, CURRENT_DAY_TIME_LIMIT, true, sequenceIndex);
}

std::string OpeningHoursParser::OpeningHours::getNearToClosingTime(const tm& dateTime, int sequenceIndex) const
{
    return getTime(dateTime, LOW_TIME_LIMIT, false, sequenceIndex);
}

std::string OpeningHoursParser::OpeningHours::getClosingTime(const tm& dateTime, int sequenceIndex) const
{
    return getTime(dateTime, WITHOUT_TIME_LIMIT, false, sequenceIndex);
}

std::string OpeningHoursParser::OpeningHours::getOpeningTomorrow(const tm& dateTime, int sequenceIndex) const
{
    struct tm cal;
    memcpy(&cal, &dateTime, sizeof(cal));
    
    std::string openingTime("");
    cal.tm_mday += 1;
    std::mktime(&cal);
    
    const auto& rules = getRules(sequenceIndex);
    for (const auto r : rules)
    {
        if (r->containsDay(cal) && r->containsMonth(cal))
            openingTime = r->getTime(cal, false, WITHOUT_TIME_LIMIT, true);
    }
    return openingTime;
}

std::string OpeningHoursParser::OpeningHours::getOpeningDay(const tm& dateTime, int sequenceIndex) const
{
    struct tm cal;
    memcpy(&cal, &dateTime, sizeof(cal));
    
    const auto& rules = getRules(sequenceIndex);
    std::string openingTime("");
    for (int i = 0; i < 7; i++)
    {
        cal.tm_mday += 1;
        std::mktime(&cal);

        for (const auto r : rules)
        {
            if (r->containsDay(cal) && r->containsMonth(cal))
                openingTime = r->getTime(cal, false, WITHOUT_TIME_LIMIT, true);
        }
        
        if (!openingTime.empty())
        {
            openingTime += " " + stringsHolder.localDaysStr[cal.tm_wday];
            break;
        }

        if (!openingTime.empty())
            break;
    }
    return openingTime;
}

std::string OpeningHoursParser::OpeningHours::getTime(const tm& dateTime, int limit, bool opening, int sequenceIndex) const
{
    std::string time = getTimeDay(dateTime, limit, opening, sequenceIndex);
    if (time.empty())
        time = getTimeAnotherDay(dateTime, limit, opening, sequenceIndex);
    
    return time;
}

std::string OpeningHoursParser::OpeningHours::getTimeDay(const tm& dateTime, int limit, bool opening, int sequenceIndex) const
{
    std::string atTime = "";
    const auto& rules = getRules(sequenceIndex);
    std::shared_ptr<OpeningHoursRule> prevRule = nullptr;
    for (const auto r : rules)
    {
        if (r->containsDay(dateTime) && r->containsMonth(dateTime))
        {
            if (atTime.length() > 0 && prevRule != nullptr && !r->hasOverlapTimes(dateTime, prevRule))
                return atTime;
            else
                atTime = r->getTime(dateTime, false, limit, opening);
        }
        
        prevRule = r;
    }
    return atTime;
}

std::string OpeningHoursParser::OpeningHours::getTimeAnotherDay(const tm& dateTime, int limit, bool opening, int sequenceIndex) const
{
    std::string atTime = "";
    const auto& rules = getRules(sequenceIndex);
    for (const auto r : rules)
    {
        if (((opening && r->containsPreviousDay(dateTime)) || (!opening && r->containsNextDay(dateTime))) && r->containsMonth(dateTime))
            atTime = r->getTime(dateTime, true, limit, opening);
    }
    return atTime;
}

std::string OpeningHoursParser::OpeningHours::getCurrentRuleTime(const tm& dateTime, int sequenceIndex) const
{
    // make exception for overlapping times i.e.
    // (1) Mo 14:00-16:00; Tu off
    // (2) Mo 14:00-02:00; Tu off
    // in (2) we need to check first rule even though it is against specification
    std::string ruleClosed;
    bool overlap = false;
    const auto& rules = getRules(sequenceIndex);
    for (int i = (int)rules.size() - 1; i >= 0; i--)
    {
        const auto r = rules[i];
        if (r->hasOverlapTimes())
        {
            overlap = true;
            break;
        }
    }
    // start from the most specific rule
    for (int i = (int)rules.size() - 1; i >= 0; i--)
    {
        bool checkNext = false;
        const auto rule = rules[i];
        if (rule->contains(dateTime))
        {
            if (i > 0)
                checkNext = !rule->hasOverlapTimes(dateTime, rules[i - 1]);
            
            bool open = rule->isOpenedForTime(dateTime);
            if (open || (!overlap && !checkNext))
                return rule->toLocalRuleString();
            else
                ruleClosed = rule->toLocalRuleString();
        }
    }
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

OpeningHoursParser::Token::Token(TokenType tokenType, int mainNumber_)
: mainNumber(mainNumber_)
{
    type = tokenType;
    text = ohp_to_string(mainNumber);
}

OpeningHoursParser::Token::~Token()
{
}


std::string OpeningHoursParser::Token::toString() const
{
    if (parent != nullptr)
        return parent->text + " [" + ohp_to_string((int)parent->type) + "] (" + text + " [" + ohp_to_string((int)type) + "]) ";
    else
        return text + " [" + ohp_to_string((int)type) + "] ";
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

std::vector<std::vector<std::string>> OpeningHoursParser::splitSequences(const std::string& format)
{
    std::vector<std::vector<std::string>> res;
    const auto& sequences = ohp_split_string(format, "||");
    for (auto seq : sequences)
    {
        seq = ohp_trim(seq);
        if (seq.length() == 0)
            continue;
        
        std::vector<std::string> rules;
        bool comment = false;
        std::stringstream sb;
        for (int i = 0; i < seq.length(); i++)
        {
            auto c = seq[i];
            if (c == '"')
            {
                comment = !comment;
                sb << c;
            }
            else if (c == ';' && !comment)
            {
                std::string s = sb.str();
                if (s.length() > 0)
                {
                    s = ohp_trim(s);
                    if (s.length() > 0)
                        rules.push_back(s);
                    
                    sb.str("");
                }
            }
            else
            {
                sb << c;
            }
        }
        std::string s = sb.str();
        if (s.length() > 0)
        {
            rules.push_back(s);
            sb.str("");
        }
        res.push_back(rules);
    }
    return res;
}

void OpeningHoursParser::parseRuleV2(const std::string& rl, int sequenceIndex, std::vector<std::shared_ptr<OpeningHoursRule>>& rules)
{
    std::string r(rl);
    std::string comment("");
    std::size_t q1Index = r.find('"');
    if (q1Index != std::string::npos)
    {
        int q2Index = r.find('"', q1Index + 1);
        if (q2Index != std::string::npos)
        {
            comment = r.substr(q1Index + 1, q2Index - (q1Index + 1));
            std::string a = r.substr(0, q1Index);
            std::string b("");
            if (r.length() > q2Index + 1)
                b = r.substr(q2Index + 1);
            
            r = a + b;
        }
    }
    r = ohp_to_lowercase(r);
    r = ohp_trim(r);
    
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
    
    auto basic = std::make_shared<BasicOpeningHourRule>(sequenceIndex);
    basic->setComment(comment);
    auto& days = basic->getDays();
    auto& months = basic->getMonths();
    auto& dayMonths = basic->getDayMonths();
    if ("24/7" == localRuleString)
    {
        std::fill(days.begin(), days.end(), true);
        basic->setHasDays(true);
        std::fill(months.begin(), months.end(), true);
        basic->addTimeRange(0, 24 * 60);
        rules.push_back(basic);
        return;
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
            std::string wrd = ohp_trim(localRuleString.substr(startWord, i - startWord));
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
    buildRule(basic, tokens, rules);
}

void OpeningHoursParser::buildRule(std::shared_ptr<BasicOpeningHourRule>& basic, std::vector<std::shared_ptr<Token>>& tokens, std::vector<std::shared_ptr<OpeningHoursRule>>& rules)
{
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
    TokenType currentParseParent = TokenType::TOKEN_UNKNOWN;
    std::vector<std::shared_ptr<std::vector<std::shared_ptr<Token>>>> listOfPairs;
    std::set<TokenType> presentTokens;
    
    auto currentPair = std::shared_ptr<std::vector<std::shared_ptr<Token>>>(new std::vector<std::shared_ptr<Token>>({ nullptr, nullptr }));
    listOfPairs.push_back(currentPair);
    std::shared_ptr<Token> prevToken;
    int indexP = 0;
    for (int i = 0; i <= tokens.size(); i++)
    {
        const auto& t = i == tokens.size() ? nullptr : tokens[i];
        if (i == 0 && t != nullptr && t->type == TokenType::TOKEN_UNKNOWN)
        {
            // skip rule if the first token unknown
            return;
        }
        if (t == nullptr || getTokenTypeOrd(t->type) > getTokenTypeOrd(currentParse))
        {
            presentTokens.insert(currentParse);
            if (currentParse == TokenType::TOKEN_MONTH || currentParse == TokenType::TOKEN_DAY_MONTH || currentParse == TokenType::TOKEN_DAY_WEEK || currentParse == TokenType::TOKEN_HOLIDAY)
            {
                bool tokenDayMonth = currentParse == TokenType::TOKEN_DAY_MONTH;
                std::vector<bool>* array = (currentParse == TokenType::TOKEN_MONTH) ? &basic->getMonths() : tokenDayMonth ? NULL : &basic->getDays();
                for (auto pair : listOfPairs)
                {
                    if (pair->at(0) != nullptr && pair->at(1) != nullptr)
                    {
                        auto& firstMonthToken = pair->at(0)->parent;
                        auto& lastMonthToken = pair->at(1)->parent;
                        if (tokenDayMonth && firstMonthToken != nullptr)
                        {
                            if (lastMonthToken != nullptr && lastMonthToken->mainNumber != firstMonthToken->mainNumber)
                            {
                                auto p = std::shared_ptr<std::vector<std::shared_ptr<Token>>>(new std::vector<std::shared_ptr<Token>>({ firstMonthToken, lastMonthToken }));
                                fillRuleArray(&basic->getMonths(), p);
                                
                                auto t1 = std::make_shared<Token>(TokenType::TOKEN_DAY_MONTH, pair->at(0)->mainNumber);
                                auto t2 = std::make_shared<Token>(TokenType::TOKEN_DAY_MONTH, 30);
                                p.reset(new std::vector<std::shared_ptr<Token>>({ t1, t2 }));
                                array = &basic->getDayMonths(firstMonthToken->mainNumber);
                                fillRuleArray(array, p);
                                
                                t1 = std::make_shared<Token>(TokenType::TOKEN_DAY_MONTH, 0);
                                t2 = std::make_shared<Token>(TokenType::TOKEN_DAY_MONTH, pair->at(1)->mainNumber);
                                p.reset(new std::vector<std::shared_ptr<Token>>({ t1, t2 }));
                                array = &basic->getDayMonths(lastMonthToken->mainNumber);
                                fillRuleArray(array, p);
                                
                                if (firstMonthToken->mainNumber <= lastMonthToken->mainNumber)
                                {
                                    for (int month = firstMonthToken->mainNumber + 1; month < lastMonthToken->mainNumber; month++)
                                    {
                                        auto& dayMonths = basic->getDayMonths(month);
                                        std::fill(dayMonths.begin(), dayMonths.end(), true);
                                    }
                                }
                                else
                                {
                                    for (int month = firstMonthToken->mainNumber + 1; month < 12; month++)
                                    {
                                        auto& dayMonths = basic->getDayMonths(month);
                                        std::fill(dayMonths.begin(), dayMonths.end(), true);
                                    }
                                    for (int month = 0; month < lastMonthToken->mainNumber; month++)
                                    {
                                        auto& dayMonths = basic->getDayMonths(month);
                                        std::fill(dayMonths.begin(), dayMonths.end(), true);
                                    }
                                }
                            }
                            else
                            {
                                array = &basic->getDayMonths(firstMonthToken->mainNumber);
                                fillRuleArray(array, pair);
                            }
                        }
                        else if (array != NULL)
                        {
                            fillRuleArray(array, pair);
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
                            auto& firstMonthToken = pair->at(0)->parent;
                            if (tokenDayMonth && firstMonthToken != nullptr)
                                array = &basic->getDayMonths(firstMonthToken->mainNumber);
                            
                            if (array != NULL)
                                array->at(pair->at(0)->mainNumber) = true;
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
            indexP = 0;
            listOfPairs.push_back(currentPair);
            currentPair->insert(currentPair->begin() + indexP++, t);
            if (t != nullptr)
            {
                currentParse = t->type;
                currentParseParent = currentParse;
                if (t->type == TokenType::TOKEN_DAY_MONTH && prevToken != nullptr && prevToken->type == TokenType::TOKEN_MONTH)
                {
                    t->parent = prevToken;
                    currentParseParent = prevToken->type;
                }
            }
        }
        else if (getTokenTypeOrd(t->type) < getTokenTypeOrd(currentParseParent) && indexP == 0 && tokens.size() > i)
        {
            auto newRule = std::make_shared<BasicOpeningHourRule>(basic->getSequenceIndex());
            newRule->setComment(basic->getComment());
            std::vector<std::shared_ptr<Token>> nextTokens(tokens.begin() + i, tokens.end());
            buildRule(newRule, nextTokens, rules);
            std::vector<std::shared_ptr<Token>> currentTokens(tokens.begin(), tokens.begin() + (i + 1));
            tokens = currentTokens;
        }
        else if (t->type == TokenType::TOKEN_COMMA)
        {
            if (tokens.size() > i + 1 && tokens[i + 1] != nullptr && getTokenTypeOrd(tokens[i + 1]->type) < getTokenTypeOrd(currentParseParent))
            {
                indexP = 0;
            }
            else
            {
                currentPair = std::shared_ptr<std::vector<std::shared_ptr<Token>>>(new std::vector<std::shared_ptr<Token>>({ nullptr, nullptr }));
                indexP = 0;
                listOfPairs.push_back(currentPair);
            }
        }
        else if (t->type == TokenType::TOKEN_DASH)
        {
        }
        else if (getTokenTypeOrd(t->type) == getTokenTypeOrd(currentParse))
        {
            if (indexP < 2)
            {
                currentPair->insert(currentPair->begin() + indexP++, t);
                if (t->type == TokenType::TOKEN_DAY_MONTH && prevToken != nullptr && prevToken->type == TokenType::TOKEN_MONTH)
                    t->parent = prevToken;
            }
        }
        prevToken = t;
    }
    if (presentTokens.find(TokenType::TOKEN_MONTH) == presentTokens.end())
    {
        auto& months = basic->getMonths();
        std::fill(months.begin(), months.end(), true);
    }
    else
    {
        if (presentTokens.find(TokenType::TOKEN_DAY_MONTH) != presentTokens.end())
            basic->setHasDayMonths(true);
    }
    if (presentTokens.find(TokenType::TOKEN_DAY_WEEK) == presentTokens.end() && presentTokens.find(TokenType::TOKEN_HOLIDAY) == presentTokens.end() && presentTokens.find(TokenType::TOKEN_DAY_MONTH) == presentTokens.end())
    {
        auto& days = basic->getDays();
        std::fill(days.begin(), days.end(), true);
        basic->setHasDays(true);
    }
    else if (presentTokens.find(TokenType::TOKEN_DAY_WEEK) != presentTokens.end())
    {
        basic->setHasDays(true);
    }
    rules.insert(rules.begin(), basic);
}

void OpeningHoursParser::fillRuleArray(std::vector<bool>* array, const std::shared_ptr<std::vector<std::shared_ptr<Token>>>& pair)
{
    if (pair->at(0)->mainNumber <= pair->at(1)->mainNumber)
    {
        for (int j = pair->at(0)->mainNumber; j <= pair->at(1)->mainNumber && j < array->size(); j++)
            array->at(j) = true;
    }
    else
    {
        // overflow
        for (int j = pair->at(0)->mainNumber; j < array->size(); j++)
            array->at(j) = true;
            
        for (int j = 0; j <= pair->at(1)->mainNumber; j++)
            array->at(j) = true;
    }
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
void OpeningHoursParser::parseRules(const std::string& rl, int sequenceIndex, std::vector<std::shared_ptr<OpeningHoursRule>>& rules)
{
    return parseRuleV2(rl, sequenceIndex, rules);
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
    
    auto rs = std::make_shared<OpeningHours>();
    rs->setOriginal(format);

    // split the OSM string in multiple rules
    const auto& sequences = splitSequences(format);
    for (int i = 0; i < sequences.size(); i++)
    {
        const auto& rules = sequences[i];
        std::vector<std::shared_ptr<OpeningHoursRule>> basicRules;
        for (const auto& r : rules)
        {
            // check if valid
            std::vector<std::shared_ptr<OpeningHoursRule>> rList;
            parseRules(r, i, rList);
            for (const auto rule : rList)
            {
                if (typeid(*rule) == typeid(BasicOpeningHourRule))
                    basicRules.push_back(rule);
            }
        }
        std::string basicRuleComment("");
        if (sequences.size() > 1)
        {
            for (const auto br : basicRules)
            {
                const auto& bRule = std::static_pointer_cast<BasicOpeningHourRule>(br);
                if (!bRule->getComment().empty())
                {
                    basicRuleComment = bRule->getComment();
                    break;
                }
            }
        }
        if (!basicRuleComment.empty())
        {
            for (const auto br : basicRules)
            {
                const auto& bRule = std::static_pointer_cast<BasicOpeningHourRule>(br);
                bRule->setComment(basicRuleComment);
            }
        }
        rs->addRules(basicRules);
    }
    rs->setSequenceCount(sequences.size());
    return rs->getRules().size() > 0 ? rs : nullptr;
}

/**
 * parse time string
 *
 * @param time     the time in the format "dd.MM.yyyy HH:mm"
 */
bool OpeningHoursParser::parseTime(const std::string& time, tm& dateTime)
{
    if (time.length() == 16)
    {
        auto day = time.substr(0, 2);
        auto month = time.substr(3, 2);
        auto year = time.substr(6, 4);
        auto hour = time.substr(11, 2);
        auto min = time.substr(14, 2);
        
        dateTime.tm_mday = atoi(day.c_str());
        dateTime.tm_mon = atoi(month.c_str()) - 1;
        dateTime.tm_year = atoi(year.c_str()) - 1900;
        dateTime.tm_hour = atoi(hour.c_str());
        dateTime.tm_min = atoi(min.c_str());
        dateTime.tm_sec = 0;
        
        std::mktime(&dateTime);

        return true;
    }
    else
    {
        return false;
    }
}

void OpeningHoursParser::setAdditionalString(const std::string& key, const std::string& value)
{
    stringsHolder.setAdditionalString(key, value);
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
    tm dateTime = {0};
    if (!OpeningHoursParser::parseTime(time, dateTime))
    {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "!!! Cannot parse date: %s", time.c_str());
        return;
    }
    
    bool calculated = hours->isOpenedForTimeV2(dateTime, OpeningHours::ALL_SEQUENCES);
    auto currentRuleTime = hours->getCurrentRuleTime(dateTime, OpeningHours::ALL_SEQUENCES);
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning,
                      "%sok: Expected %s: %s = %s (rule %s)",
                      (calculated != expected) ? "NOT " : "", time.c_str(), expected ? "true" : "false", calculated ? "true" : "false", currentRuleTime.c_str());

    if (calculated != expected) {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "BUG!!!");
        throw;
    }
}

/**
 * test if the calculated opening hours are what you expect
 *
 * @param time        the time to test in the format "dd.MM.yyyy HH:mm"
 * @param hours       the OpeningHours object
 * @param expected    the expected string in format:
 *                         "Open from HH:mm"     - open in 5 hours
 *                         "Will open at HH:mm"  - open in 2 hours
 *                         "Open till HH:mm"     - close in 5 hours
 *                         "Will close at HH:mm" - close in 2 hours
 *                         "Will open on HH:mm (Mo,Tu,We,Th,Fr,Sa,Su)" - open in >5 hours
 *                         "Open 24/7"           - open 24/7
 */
void OpeningHoursParser::testInfo(const std::string& time, const std::shared_ptr<OpeningHours>& hours, const std::string& expected)
{
    testInfo(time, hours, expected, OpeningHours::ALL_SEQUENCES);
}

/**
 * test if the calculated opening hours are what you expect
 *
 * @param time        the time to test in the format "dd.MM.yyyy HH:mm"
 * @param hours       the OpeningHours object
 * @param expected    the expected string in format:
 *                         "Open from HH:mm"     - open in 5 hours
 *                         "Will open at HH:mm"  - open in 2 hours
 *                         "Open till HH:mm"     - close in 5 hours
 *                         "Will close at HH:mm" - close in 2 hours
 *                         "Will open on HH:mm (Mo,Tu,We,Th,Fr,Sa,Su)" - open in >5 hours
 *                         "Open 24/7"           - open 24/7
 *
 * @param sequenceIndex sequence index of rules separated by ||
 */
void OpeningHoursParser::testInfo(const std::string& time, const std::shared_ptr<OpeningHours>& hours, const std::string& expected, int sequenceIndex)
{
    tm dateTime = {0};
    if (!OpeningHoursParser::parseTime(time, dateTime))
    {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "!!! Cannot parse date: %s", time.c_str());
        return;
    }
    
    std::string description("");
    if (sequenceIndex == OpeningHours::ALL_SEQUENCES)
    {
        const auto& info = hours->getCombinedInfo(dateTime);
        description = info->getInfo();
    }
    else
    {
        const auto& infos = hours->getInfo(dateTime);
        const auto& info = infos[sequenceIndex];
        description = info->getInfo();
    }

    auto currentRuleTime = hours->getCurrentRuleTime(dateTime, OpeningHours::ALL_SEQUENCES);
    
    bool result = ohp_to_lowercase(expected) == ohp_to_lowercase(description);
    
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning,
                      "%sok: Expected %s (%s): %s (rule %s)",
                      (!result) ? "NOT " : "", time.c_str(), expected.c_str(), description.c_str(), currentRuleTime.c_str());

    if (!result) {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "BUG!!!");
        throw;
    }
}

void OpeningHoursParser::testParsedAndAssembledCorrectly(const std::string& timeString, const std::shared_ptr<OpeningHours>& hours)
{
    auto assembledString = hours->toString();
    bool isCorrect = ohp_to_lowercase(assembledString) == ohp_to_lowercase(timeString);
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning,
                      "%sok: Expected: \"%s\" got: \"%s\"",
                      (!isCorrect ? "NOT " : ""), timeString.c_str(), assembledString.c_str());
    if (!isCorrect) {
        OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "BUG!!!");
        throw;
    }
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

    auto rs = std::make_shared<OpeningHours>();
    rs->setOriginal(format);
    const auto& sequences = splitSequences(format);
    for (int i = sequences.size() - 1; i >= 0; i--)
    {
        const auto& rules = sequences[i];
        for (auto r : rules)
        {
            r = ohp_trim(r);
            if (r.length() == 0)
                continue;
            
            // check if valid
            std::vector<std::shared_ptr<OpeningHoursRule>> rList;
            parseRules(r, i, rList);
            rs->addRules(rList);
        }
    }
    rs->setSequenceCount(sequences.size());
    return rs;
}

std::vector<std::shared_ptr<OpeningHoursParser::OpeningHours::Info>> OpeningHoursParser::getInfo(const std::string& format)
{
    const auto& openingHours = OpeningHoursParser::parseOpenedHours(format);
    if (openingHours == nullptr)
        return {};
    else
        return openingHours->getInfo();
}

void OpeningHoursParser::runTest()
{
    // 0. not properly supported
    // hours = parseOpenedHours("Mo-Su (sunrise-00:30)-(sunset+00:30)");
    
    auto hours = parseOpenedHours("Apr 05-Oct 24: Fr 08:00-16:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("26.08.2018 15:00", hours, false);
    testOpened("29.03.2019 15:00", hours, false);
    testOpened("05.04.2019 11:00", hours, true);
    
    hours = parseOpenedHours("Oct 24-Apr 05: Fr 08:00-16:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("26.08.2018 15:00", hours, false);
    testOpened("29.03.2019 15:00", hours, true);
    testOpened("26.04.2019 11:00", hours, false);
    
    hours = parseOpenedHours("Oct 24-Apr 05, Jun 10-Jun 20, Jul 6-12: Fr 08:00-16:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("26.08.2018 15:00", hours, false);
    testOpened("02.01.2019 15:00", hours, false);
    testOpened("29.03.2019 15:00", hours, true);
    testOpened("26.04.2019 11:00", hours, false);
    
    hours = parseOpenedHours("Apr 05-24: Fr 08:00-16:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("12.10.2018 11:00", hours, false);
    testOpened("12.04.2019 15:00", hours, true);
    testOpened("27.04.2019 15:00", hours, false);
    
    hours = parseOpenedHours("Apr 5: Fr 08:00-16:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("05.04.2019 15:00", hours, true);
    testOpened("06.04.2019 15:00", hours, false);
    
    hours = parseOpenedHours("Apr 24-05: Fr 08:00-16:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("12.10.2018 11:00", hours, false);
    testOpened("12.04.2018 15:00", hours, false);
    
    hours = parseOpenedHours("Apr: Fr 08:00-16:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("12.10.2018 11:00", hours, false);
    testOpened("12.04.2019 15:00", hours, true);
    
    hours = parseOpenedHours("Apr-Oct: Fr 08:00-16:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("09.11.2018 11:00", hours, false);
    testOpened("12.10.2018 11:00", hours, true);
    testOpened("24.08.2018 15:00", hours, true);
    testOpened("09.03.2018 15:00", hours, false);
    
    hours = parseOpenedHours("Apr, Oct: Fr 08:00-16:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("09.11.2018 11:00", hours, false);
    testOpened("12.10.2018 11:00", hours, true);
    testOpened("24.08.2018 15:00", hours, false);
    testOpened("12.04.2019 15:00", hours, true);
    
    // Test basic case
    hours = parseOpenedHours("Mo-Fr 08:30-14:40");
    testOpened("09.08.2012 11:00", hours, true);
    testOpened("09.08.2012 16:00", hours, false);
    hours = parseOpenedHours("mo-fr 07:00-19:00; sa 12:00-18:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    
    std::string string = "Mo-Fr 11:30-15:00, 17:30-23:00; Sa, Su, PH 11:30-23:00";
    hours = parseOpenedHours(string);
    testParsedAndAssembledCorrectly(string, hours);
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("07.09.2015 14:54", hours, true); // monday
    testOpened("07.09.2015 15:05", hours, false);
    testOpened("06.09.2015 16:05", hours, true);
    
    
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
    
    // test time off (not days)
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

    // test open from/till
    hours = parseOpenedHours("Mo-Fr 08:30-17:00; 12:00-12:40 off;");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testInfo("15.01.2018 09:00", hours, "Open till 12:00");
    testInfo("15.01.2018 11:00", hours, "Will close at 12:00");
    testInfo("15.01.2018 12:00", hours, "Will open at 12:40");
    
    hours = parseOpenedHours("Mo-Fr: 9:00-13:00, 14:00-18:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testInfo("15.01.2018 08:00", hours, "Will open at 09:00");
    testInfo("15.01.2018 09:00", hours, "Open till 13:00");
    testInfo("15.01.2018 12:00", hours, "Will close at 13:00");
    testInfo("15.01.2018 13:10", hours, "Will open at 14:00");
    testInfo("15.01.2018 14:00", hours, "Open till 18:00");
    testInfo("15.01.2018 16:00", hours, "Will close at 18:00");
    testInfo("15.01.2018 18:10", hours, "Will open tomorrow at 09:00");
    
    hours = parseOpenedHours("Mo-Sa 02:00-10:00; Th off");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testInfo("15.01.2018 23:00", hours, "Will open tomorrow at 02:00");
    
    hours = parseOpenedHours("Mo-Sa 23:00-02:00; Th off");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testInfo("15.01.2018 22:00", hours, "Will open at 23:00");
    testInfo("15.01.2018 23:00", hours, "Open till 02:00");
    testInfo("16.01.2018 00:30", hours, "Will close at 02:00");
    testInfo("16.01.2018 02:00", hours, "Open from 23:00");
    
    hours = parseOpenedHours("Mo-Sa 08:30-17:00; Th off");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testInfo("17.01.2018 20:00", hours, "Will open on 08:30 Fri.");
    testInfo("18.01.2018 05:00", hours, "Will open tomorrow at 08:30");
    testInfo("20.01.2018 05:00", hours, "Open from 08:30");
    testInfo("21.01.2018 05:00", hours, "Will open tomorrow at 08:30");
    testInfo("22.01.2018 02:00", hours, "Open from 08:30");
    testInfo("22.01.2018 04:00", hours, "Open from 08:30");
    testInfo("22.01.2018 07:00", hours, "Will open at 08:30");
    testInfo("23.01.2018 10:00", hours, "Open till 17:00");
    testInfo("23.01.2018 16:00", hours, "Will close at 17:00");
    
    hours = parseOpenedHours("24/7");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testInfo("24.01.2018 02:00", hours, "Open 24/7");
    
    hours = parseOpenedHours("Mo-Su 07:00-23:00, Fr 08:00-20:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("15.01.2018 06:45", hours, false);
    testOpened("15.01.2018 07:45", hours, true);
    testOpened("15.01.2018 23:45", hours, false);
    testOpened("19.01.2018 07:45", hours, false);
    testOpened("19.01.2018 08:45", hours, true);
    testOpened("19.01.2018 20:45", hours, false);
    
    // test fallback case
    hours = parseOpenedHours("07:00-01:00 open \"Restaurant\" || Mo 00:00-04:00,07:00-04:00; Tu-Th 07:00-04:00; Fr 07:00-24:00; Sa,Su 00:00-24:00 open \"McDrive\"");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("22.01.2018 00:30", hours, true);
    testOpened("22.01.2018 08:00", hours, true);
    testOpened("22.01.2018 03:30", hours, true);
    testOpened("22.01.2018 05:00", hours, false);
    testOpened("23.01.2018 05:00", hours, false);
    testOpened("27.01.2018 05:00", hours, true);
    testOpened("28.01.2018 05:00", hours, true);
    
    testInfo("22.01.2018 05:00", hours, "Will open at 07:00 - Restaurant", 0);
    testInfo("26.01.2018 00:00", hours, "Will close at 01:00 - Restaurant", 0);
    testInfo("22.01.2018 05:00", hours, "Will open at 07:00 - McDrive", 1);
    testInfo("22.01.2018 00:00", hours, "Open till 04:00 - McDrive", 1);
    testInfo("22.01.2018 02:00", hours, "Will close at 04:00 - McDrive", 1);
    testInfo("27.01.2018 02:00", hours, "Open till 24:00 - McDrive", 1);
    
    hours = parseOpenedHours("07:00-03:00 open \"Restaurant\" || 24/7 open \"McDrive\"");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("22.01.2018 02:00", hours, true);
    testOpened("22.01.2018 17:00", hours, true);
    testInfo("22.01.2018 05:00", hours, "Will open at 07:00 - Restaurant", 0);
    testInfo("22.01.2018 04:00", hours, "Open 24/7 - McDrive", 1);
    
    hours = parseOpenedHours("Mo-Fr 12:00-15:00, Tu-Fr 17:00-23:00, Sa 12:00-23:00, Su 14:00-23:00");
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "%s", hours->toString().c_str());
    testOpened("16.02.2018 14:00", hours, true);
    testOpened("16.02.2018 16:00", hours, false);
    testOpened("16.02.2018 17:00", hours, true);
    testInfo("16.02.2018 09:45", hours, "Open from 12:00");
    testInfo("16.02.2018 12:00", hours, "Open till 15:00");
    testInfo("16.02.2018 14:00", hours, "Will close at 15:00");
    testInfo("16.02.2018 16:00", hours, "Will open at 17:00");
    testInfo("16.02.2018 18:00", hours, "Open till 23:00");
    
    hours = parseOpenedHours("Mo-Fr 10:00-21:00; Sa 12:00-23:00; PH \"Wird auf der Homepage bekannt gegeben.\"");
    testParsedAndAssembledCorrectly("Mo-Fr 10:00-21:00; Sa 12:00-23:00; PH - Wird auf der Homepage bekannt gegeben.", hours);
    
    OsmAnd::LogPrintf(OsmAnd::LogSeverityLevel::Warning, "OpeningHoursParser test done");
}

