#ifndef _OPENINGHOURSPARSER_H
#define _OPENINGHOURSPARSER_H

//  OsmAnd-java/src/net/osmand/util/OpeningHoursParser.java
//  git revision 0a298dc9c818a12c50441303601ad71d5845eec8

#include <string>
#include <vector>
#include <sstream>
#include <memory>
#include <ctime>
#include <unordered_map>

struct StringsHolder
{
    std::vector<std::string> daysStr;
    std::vector<std::string> localDaysStr;
    std::vector<std::string> monthsStr;
    std::vector<std::string> localMothsStr;
    std::unordered_map<std::string, std::string> additionalStrings;

    StringsHolder();
    ~StringsHolder();
    
    void setAdditionalString(const std::string& key, const std::string& value);
};

struct OpeningHoursParser
{
private:
    
    enum class TokenType : int
    {
        TOKEN_UNKNOWN = 0,
        TOKEN_COLON,
        TOKEN_COMMA,
        TOKEN_DASH,
        // order is important
        TOKEN_YEAR,
        TOKEN_MONTH,
        TOKEN_DAY_MONTH,
        TOKEN_HOLIDAY,
        TOKEN_DAY_WEEK,
        TOKEN_HOUR_MINUTES,
        TOKEN_OFF_ON,
        TOKEN_COMMENT
    };
    
    inline static int getTokenTypeOrd(TokenType tokenType)
    {
        switch (tokenType)
        {
            case TokenType::TOKEN_UNKNOWN:      return 0;
            case TokenType::TOKEN_COLON:        return 1;
            case TokenType::TOKEN_COMMA:        return 2;
            case TokenType::TOKEN_DASH:         return 3;
            case TokenType::TOKEN_YEAR:         return 4;
            case TokenType::TOKEN_MONTH:        return 5;
            case TokenType::TOKEN_DAY_MONTH:    return 6;
            case TokenType::TOKEN_HOLIDAY:      return 7;
            case TokenType::TOKEN_DAY_WEEK:     return 7;
            case TokenType::TOKEN_HOUR_MINUTES: return 8;
            case TokenType::TOKEN_OFF_ON:       return 9;
            case TokenType::TOKEN_COMMENT:      return 10;

            default: return 0;
        }
    }
    
    struct Token
    {
        Token(TokenType tokenType, const std::string& string);
        Token(TokenType tokenType, int mainNumber);
        virtual ~Token();
        
        int mainNumber;
        TokenType type;
        std::string text;
        std::shared_ptr<Token> parent;
        
        std::string toString() const;
    };
    
public:
    /**
     * Interface to represent a single rule
     * <p/>
     * A rule consist out of
     * - a collection of days/dates
     * - a time range
     */
    struct OpeningHoursRule
    {
        
        /**
         * Check if, for this rule, the feature is opened for time "date"
         *
         * @param date           the time to check
         * @param checkPrevious only check for overflowing times (after midnight) or don't check for it
         * @return true if the feature is open
         */
        virtual bool isOpenedForTime(const tm& dateTime, bool checkPrevious) const = 0;
        
        /**
         * Check if, for this rule, the feature is opened for time "cal"
         * @param date
         * @return true if the feature is open
         */
        virtual bool isOpenedForTime(const tm& dateTime) const = 0;
        
        /**
         * Check if the previous day before "date" is part of this rule
         *
         * @param date; the time to check
         * @return true if the previous day is part of the rule
         */
        virtual bool containsPreviousDay(const tm& dateTime) const = 0;
        
        /**
         * Check if the day of "date" is part of this rule
         *
         * @param date the time to check
         * @return true if the day is part of the rule
         */
        virtual bool containsDay(const tm& dateTime) const = 0;
        
        /**
         * Check if the next day after "cal" is part of this rule
         *
         * @param cal the time to check
         * @return true if the next day is part of the rule
         */
        virtual bool containsNextDay(const tm& dateTime) const = 0;
        
        /**
         * Check if the month of "date" is part of this rule
         *
         * @param date the time to check
         * @return true if the month is part of the rule
         */
        virtual bool containsMonth(const tm& dateTime) const = 0;
        
        /**
         * Check if the year of "date" is part of this rule
         *
         * @param cal the time to check
         * @return true if the year is part of the rule
         */
        virtual bool containsYear(const tm& dateTime) const = 0;
        
        /**
         * @return true if the rule overlap to the next day
         */
        virtual bool hasOverlapTimes() const = 0;
        
        /**
         * Check if r rule times overlap with this rule times at "cal" date.
         *
         * @param cal the date to check
         * @param r the rule to check
         * @return true if the this rule times overlap with r times
         */
        virtual bool hasOverlapTimes(const tm& dateTime, const std::shared_ptr<OpeningHoursRule>& r) const = 0;

        virtual int getSequenceIndex() const = 0;
        
        /**
         * @param date
         * @return true if rule applies for current time
         */
        virtual bool contains(const tm& dateTime) const = 0;
        
        virtual bool isOpened24_7() const = 0;

        virtual std::string getTime(const tm& dateTime, bool checkAnotherDay, int limit, bool opening) const = 0;

        virtual std::string toRuleString() const = 0;
        virtual std::string toLocalRuleString() const = 0;
        virtual std::string toString() const = 0;
    };
    
    /**
     * implementation of the basic OpeningHoursRule
     * <p/>
     * This implementation only supports month, day of weeks and numeral times, or the value "off"
     */
    struct BasicOpeningHourRule : public OpeningHoursRule
    {
    private:
        int getCurrentDay(const tm& dateTime) const;
        int getPreviousDay(int currentDay) const;
        int getNextDay(int currentDay) const;
        int getCurrentTimeInMinutes(const tm& dateTime) const;
        std::string toRuleString(const std::vector<std::string>& dayNames, const std::vector<std::string>& monthNames) const;
        void addArray(const std::vector<bool>& array, const std::vector<std::string>& arrayNames, std::stringstream& b) const;

    private:
        /**
         * represents the list on which days it is open.
         * Day number 0 is MONDAY
         */
        std::vector<bool> _days;
        bool _hasDays;
        
        /**
         * represents the list on which month it is open.
         * Day number 0 is JANUARY.
         */
        std::vector<bool> _months;
        
        /**
         * represents the list on which year / month it is open.
         */
        std::vector<int> _firstYearMonths;
        std::vector<int> _lastYearMonths;
        int _fullYears;
        int _year;
        
        /**
         * represents the list on which day it is open.
         */
        std::vector<std::vector<bool>> _dayMonths;
        
        /**
         * lists of equal size representing the start and end times
         */
        std::vector<int> _startTimes;
        std::vector<int> _endTimes;
        
        bool _publicHoliday;
        bool _schoolHoliday;
        bool _easter;
        
        /**
         * Flag that means that time is off
         */
        bool _off;
        
        /**
         * Additional information or limitation.
         * https://wiki.openstreetmap.org/wiki/Key:opening_hours/specification#explain:comment
         */
        std::string _comment;
        
        int _sequenceIndex;

        void init();
        
    public:
        BasicOpeningHourRule();
        BasicOpeningHourRule(int sequenceIndex);
        
        virtual ~BasicOpeningHourRule();
        
        int getSequenceIndex() const;
        
        /**
         * return an array representing the days of the rule
         *
         * @return the days of the rule
         */
        std::vector<bool>& getDays();
        
        /**
         * return an array representing the months of the rule
         *
         * @return the months of the rule
         */
        std::vector<bool>& getMonths();
        
        std::vector<int>& getFirstYearMonths();
        std::vector<int>& getLastYearMonths();
        int getFullYears() const;
        void setFullYears(int value);
        int getYear() const;
        void setYear(int year);

        /**
         * represents the list on which day it is open.
         */
        std::vector<bool>& getDayMonths(int month);
        bool hasDayMonths() const;

        bool hasDays() const;
        void setHasDays(bool value);

        bool appliesToPublicHolidays() const;
        bool appliesEaster() const;
        bool appliesToSchoolHolidays() const;
        bool appliesOff() const;
        
        std::string getComment() const;
        void setComment(std::string comment);
        
        void setPublicHolidays(bool value);
        void setEaster(bool value);
        void setSchoolHolidays(bool value);
        void setOff(bool value);
        
        /**
         * set a single start time, erase all previously added start times
         *
         * @param s startTime to set
         */
        void setStartTime(int s);
        
        /**
         * set a single end time, erase all previously added end times
         *
         * @param e endTime to set
         */
        void setEndTime(int e);
        
        /**
         * Set single start time. If position exceeds index of last item by one
         * then new value will be added.
         * If value is between 0 and last index, then value in the position p will be overwritten
         * with new one.
         * Else exception will be thrown.
         *
         * @param s        - value
         * @param position - position to add
         */
        void setStartTime(int s, int position);
        
        /**
         * Set single end time. If position exceeds index of last item by one
         * then new value will be added.
         * If value is between 0 and last index, then value in the position p will be overwritten
         * with new one.
         * Else exception will be thrown.
         *
         * @param s        - value
         * @param position - position to add
         */
        void setEndTime(int s, int position);
        
        /**
         * get a single start time
         *
         * @return a single start time
         */
        int getStartTime() const;
        
        /**
         * get a single start time in position
         *
         * @param position position to get value from
         * @return a single start time
         */
        int getStartTime(int position) const;
        
        /**
         * get a single end time
         *
         * @return a single end time
         */
        int getEndTime() const;
        
        /**
         * get a single end time in position
         *
         * @param position position to get value from
         * @return a single end time
         */
        int getEndTime(int position) const;
        
        /**
         * get all start times as independent list
         *
         * @return all start times
         */
        std::vector<int> getStartTimes() const;
        
        /**
         * get all end times as independent list
         *
         * @return all end times
         */
        std::vector<int> getEndTimes() const;
        
        /**
         * Check if the weekday of time "date" is part of this rule
         *
         * @param date the time to check
         * @return true if this day is part of the rule
         */
        bool containsDay(const tm& dateTime) const;
        
        bool hasOverlapTimes() const;
        
        bool hasOverlapTimes(const tm& dateTime, const std::shared_ptr<OpeningHoursRule>& r) const;

        /**
         * Check if the next weekday of time "date" is part of this rule
         *
         * @param date the time to check
         * @return true if the next day is part of the rule
         */
        bool containsNextDay(const tm& dateTime) const;
        
        /**
         * Check if the previous weekday of time "date" is part of this rule
         *
         * @param date the time to check
         * @return true if the previous day is part of the rule
         */
        bool containsPreviousDay(const tm& dateTime) const;
        
        /**
         * Check if the month of "date" is part of this rule
         *
         * @param date the time to check
         * @return true if the month is part of the rule
         */
        bool containsMonth(const tm& dateTime) const;
        
        /**
         * Check if the year of "date" is part of this rule
         *
         * @param cal the time to check
         * @return true if the year is part of the rule
         */
        bool containsYear(const tm& dateTime) const;
        
        /**
         * Check if this rule says the feature is open at time "date"
         *
         * @param date the time to check
         * @return false in all other cases, also if only day is wrong
         */
        bool isOpenedForTime(const tm& dateTime, bool checkPrevious) const;
        bool isOpenedForTime(const tm& dateTime) const;

        bool contains(const tm& dateTime) const;
        
        bool isOpened24_7() const;
        bool isOpenedEveryDay() const;

        std::string getTime(const tm& dateTime, bool checkAnotherDay, int limit, bool opening) const;

        std::string toRuleString() const;
        std::string toLocalRuleString() const;
        std::string toString() const;
        
        void appendDaysString(std::stringstream& builder) const;
        void appendDaysString(std::stringstream& builder, const std::vector<std::string>& daysNames) const;
        bool appendYearString(std::stringstream& builder, const std::vector<int>& yearMonths, int month) const;

        /**
         * Add a time range (startTime-endTime) to this rule
         *
         * @param startTime startTime to add
         * @param endTime   endTime to add
         */
        void addTimeRange(int startTime, int endTime);
        int timesSize() const;
        void deleteTimeRange(int position);
        
        int calculate(const tm& dateTime) const;
    };
    
    struct UnparseableRule : public OpeningHoursRule
    {
    private:
        std::string _ruleString;
        
    public:
        UnparseableRule(const std::string& ruleString);
        virtual ~UnparseableRule();

        bool isOpenedForTime(const tm& dateTime, bool checkPrevious) const;
        bool containsPreviousDay(const tm& dateTime) const;
        bool hasOverlapTimes() const;
        bool hasOverlapTimes(const tm& dateTime, const std::shared_ptr<OpeningHoursRule>& r) const;
        bool containsDay(const tm& dateTime) const;
        bool containsNextDay(const tm& dateTime) const;
        bool containsMonth(const tm& dateTime) const;
        bool containsYear(const tm& dateTime) const;

        int getSequenceIndex() const;

        bool isOpened24_7() const;
        
        std::string toRuleString() const;
        std::string toLocalRuleString() const;
        std::string toString() const;

        std::string getTime(const tm& dateTime, bool checkAnotherDay, int limit, bool opening) const;
        
        bool isOpenedForTime(const tm& dateTime) const;
        bool contains(const tm& dateTime) const;
    };
    
    /**
     * This class contains the entire OpeningHours schema and
     * offers methods to check directly weather something is open
     *
     * @author sander
     */
    struct OpeningHours
    {
        static const int ALL_SEQUENCES = -1;
        
        struct Info
        {
            bool opened;
            bool opened24_7;
            std::string openingTime;
            std::string nearToOpeningTime;
            std::string closingTime;
            std::string nearToClosingTime;
            std::string openingTomorrow;
            std::string openingDay;
            std::string ruleString;
        
            /**
             * Empty constructor
             */
            Info();
            virtual ~Info();
            
            std::string getInfo();
        };
        
    private:
        /**
         * list of the different rules
         */
        std::vector<std::shared_ptr<OpeningHoursRule>> _rules;
        std::string _original;
        int _sequenceCount;
        
        std::shared_ptr<Info> getInfo(const tm& dateTime, int sequenceIndex) const;
        
    public:
        /**
         * Constructor
         *
         * @param rules List of OpeningHoursRule to be given
         */
        OpeningHours(std::vector<std::shared_ptr<OpeningHoursRule>>& rules);

        /**
         * Empty constructor
         */
        OpeningHours();
        virtual ~OpeningHours();

        std::vector<std::shared_ptr<Info>> getInfo();
        std::vector<std::shared_ptr<Info>> getInfo(const tm& dateTime);
        std::shared_ptr<Info> getCombinedInfo();
        std::shared_ptr<Info> getCombinedInfo(const tm& dateTime);
        
        /**
         * add a rule to the opening hours
         *
         * @param r rule to add
         */
        void addRule(std::shared_ptr<OpeningHoursRule> r);
        
        /**
         * add rules to the opening hours
         *
         * @param rules - rules to add
         */
        void addRules(std::vector<std::shared_ptr<OpeningHoursRule>>& rules);
        
        /**
         * return the list of rules
         *
         * @return the rules
         */
        std::vector<std::shared_ptr<OpeningHoursRule>> getRules() const;
        
        std::vector<std::shared_ptr<OpeningHoursRule>> getRules(int sequenceIndex) const;

        int getSequenceCount() const;
        void setSequenceCount(int sequenceCount);
        
        /**
         * check if the feature is opened at time "cal"
         *
         * @param dateTime the time to check
         * @return true if feature is open
         */
        bool isOpenedForTimeV2(const tm& dateTime, int sequenceIndex) const;
        
        /**
         * check if the feature is opened now
         *
         * @return true if feature is open
         */
        bool isOpened() const;

        /**
         * check if the feature is opened at time "cal"
         *
         * @param dateTime the time to check
         * @return true if feature is open
         */
        bool isOpenedForTime(const tm& dateTime) const;

        
        bool isOpened24_7(int sequenceIndex) const;
        
        std::string getNearToOpeningTime(const tm& dateTime, int sequenceIndex) const;
        std::string getOpeningTime(const tm& dateTime, int sequenceIndex) const;
        std::string getNearToClosingTime(const tm& dateTime, int sequenceIndex) const;
        std::string getClosingTime(const tm& dateTime, int sequenceIndex) const;
        std::string getOpeningTomorrow(const tm& dateTime, int sequenceIndex) const;
        std::string getOpeningDay(const tm& dateTime, int sequenceIndex) const;
        
        std::string getTime(const tm& dateTime, int limit, bool opening, int sequenceIndex) const;
        std::string getTimeDay(const tm& dateTime, int limit, bool opening, int sequenceIndex) const;
        std::string getTimeAnotherDay(const tm& dateTime, int limit, bool opening, int sequenceIndex) const;

        std::string getCurrentRuleTime(const tm& dateTime, int sequenceIndex) const;
        
        std::string toString() const;
        std::string toLocalString() const;
        
        void setOriginal(std::string original);
        std::string getOriginal() const;
    };
    
private:
    std::string openingHours;
    
    static void findInArray(std::shared_ptr<Token>& t, const std::vector<std::string>& list, TokenType tokenType);
    static std::vector<std::vector<std::string>> splitSequences(const std::string& format);

    static bool parseTime(const std::string& time, tm& dateTime);
    static void testOpened(const std::string& time, const std::shared_ptr<OpeningHours>& hours, bool expected);
    static void testInfo(const std::string& time, const std::shared_ptr<OpeningHours>& hours, const std::string& expected);
    static void testInfo(const std::string& time, const std::shared_ptr<OpeningHours>& hours, const std::string& expected, int sequenceIndex);

    static void testParsedAndAssembledCorrectly(const std::string& timeString, const std::shared_ptr<OpeningHours>& hours);
    static std::shared_ptr<OpeningHours> parseOpenedHoursHandleErrors(const std::string& format);

    static void buildRule(std::shared_ptr<BasicOpeningHourRule>& basic, std::vector<std::shared_ptr<Token>>& tokens, std::vector<std::shared_ptr<OpeningHoursRule>>& rules);
    static void fillRuleArray(std::vector<bool>* array, const std::shared_ptr<std::vector<std::shared_ptr<Token>>>& pair);

public:
    
    OpeningHoursParser(const std::string& openingHours);
    ~OpeningHoursParser();

    static void setAdditionalString(const std::string& key, const std::string& value);

    static void parseRuleV2(const std::string& rl, int sequenceIndex, std::vector<std::shared_ptr<OpeningHoursRule>>& rules);
    static void parseRules(const std::string& rl, int sequenceIndex, std::vector<std::shared_ptr<OpeningHoursRule>>& rules);

    static std::shared_ptr<OpeningHours> parseOpenedHours(const std::string& format);
    
    static std::vector<std::shared_ptr<OpeningHours::Info>> getInfo(const std::string& format);
    
    static void runTest();

};

#endif // _OPENINGHOURSPARSER_H
