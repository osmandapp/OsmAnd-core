#ifndef _OPENINGHOURSPARSER_H
#define _OPENINGHOURSPARSER_H

//  OsmAnd-java/src/net/osmand/util/OpeningHoursParser.java
//  git revision 

#include "CommonCollections.h"
#include "commonOsmAndCore.h"

struct StringsHolder
{
    std::vector<std::string> daysStr;
    std::vector<std::string> localDaysStr;
    std::vector<std::string> monthsStr;
    std::vector<std::string> localMothsStr;
    
    StringsHolder();
    ~StringsHolder();
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
        TOKEN_MONTH,
        TOKEN_DAY_MONTH,
        TOKEN_HOLIDAY,
        TOKEN_DAY_WEEK,
        TOKEN_HOUR_MINUTES,
        TOKEN_OFF_ON
    };
    
    inline static int getTokenTypeOrd(TokenType tokenType)
    {
        switch (tokenType)
        {
            case TokenType::TOKEN_UNKNOWN:      return 0;
            case TokenType::TOKEN_COLON:        return 1;
            case TokenType::TOKEN_COMMA:        return 2;
            case TokenType::TOKEN_DASH:         return 3;
            case TokenType::TOKEN_MONTH:        return 4;
            case TokenType::TOKEN_DAY_MONTH:    return 5;
            case TokenType::TOKEN_HOLIDAY:      return 6;
            case TokenType::TOKEN_DAY_WEEK:     return 6;
            case TokenType::TOKEN_HOUR_MINUTES: return 7;
            case TokenType::TOKEN_OFF_ON:       return 8;
            
            default: return 0;
        }
    }
    
    struct Token
    {
        Token(TokenType tokenType, const std::string& string);
        virtual ~Token();
        
        int mainNumber;
        TokenType type;
        std::string text;
        
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
        virtual bool isOpenedForTime(const std::tm& date, bool checkPrevious) const = 0;
        
        /**
         * Check if, for this rule, the feature is opened for time "cal"
         * @param date
         * @return true if the feature is open
         */
        virtual bool isOpenedForTime(const std::tm& date) const = 0;
        
        /**
         * Check if the previous day before "date" is part of this rule
         *
         * @param date; the time to check
         * @return true if the previous day is part of the rule
         */
        virtual bool containsPreviousDay(const std::tm& date) const = 0;
        
        /**
         * Check if the day of "date" is part of this rule
         *
         * @param date the time to check
         * @return true if the day is part of the rule
         */
        virtual bool containsDay(const std::tm& date) const = 0;
        
        /**
         * Check if the next day after "cal" is part of this rule
         *
         * @param cal the time to check
         * @return true if the next day is part of the rule
         */
        virtual bool containsNextDay(const std::tm& date) const = 0;
        
        /**
         * Check if the month of "date" is part of this rule
         *
         * @param date the time to check
         * @return true if the month is part of the rule
         */
        virtual bool containsMonth(const std::tm& date) const = 0;
        
        /**
         * @return true if the rule overlap to the next day
         */
        virtual bool hasOverlapTimes() const = 0;
        
        /**
         * @param date
         * @return true if rule applies for current time
         */
        virtual bool contains(const std::tm& date) const = 0;
        
        virtual bool isOpened24_7() const = 0;

        virtual std::string getTime(const std::tm& date, bool checkAnotherDay, int limit, bool opening) const = 0;

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
        int getCurrentDay(const std::tm& date) const;
        int getPreviousDay(int currentDay) const;
        int getNextDay(int currentDay) const;
        int getCurrentTimeInMinutes(const std::tm& date) const;
        std::string toRuleString(const std::vector<std::string>& dayNames, const std::vector<std::string>& monthNames) const;
        void addArray(const std::vector<bool>& array, const std::vector<std::string>& arrayNames, std::stringstream& b) const;

    private:
        /**
         * represents the list on which days it is open.
         * Day number 0 is MONDAY
         */
        std::vector<bool> _days;
        
        /**
         * represents the list on which month it is open.
         * Day number 0 is JANUARY.
         */
        std::vector<bool> _months;
        
        /**
         * represents the list on which day it is open.
         */
        std::vector<bool> _dayMonths;
        
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
        
    public:
        BasicOpeningHourRule();
        virtual ~BasicOpeningHourRule();
        
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
        
        /**
         * represents the list on which day it is open.
         */
        std::vector<bool>& getDayMonths();
        
        bool appliesToPublicHolidays() const;
        bool appliesEaster() const;
        bool appliesToSchoolHolidays() const;
        bool appliesOff() const;
        
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
        bool containsDay(const std::tm& date) const;
        
        bool hasOverlapTimes() const;

        /**
         * Check if the next weekday of time "date" is part of this rule
         *
         * @param date the time to check
         * @return true if the next day is part of the rule
         */
        bool containsNextDay(const std::tm& date) const;
        
        /**
         * Check if the previous weekday of time "date" is part of this rule
         *
         * @param date the time to check
         * @return true if the previous day is part of the rule
         */
        bool containsPreviousDay(const std::tm& date) const;
        
        /**
         * Check if the month of "date" is part of this rule
         *
         * @param date the time to check
         * @return true if the month is part of the rule
         */
        bool containsMonth(const std::tm& date) const;
        
        /**
         * Check if this rule says the feature is open at time "date"
         *
         * @param date the time to check
         * @return false in all other cases, also if only day is wrong
         */
        bool isOpenedForTime(const std::tm& date, bool checkPrevious) const;
        bool isOpenedForTime(const std::tm& date) const;

        bool contains(const std::tm& date) const;
        
        bool isOpened24_7() const;

        std::string getTime(const std::tm& date, bool checkAnotherDay, int limit, bool opening) const;

        std::string toRuleString() const;
        std::string toLocalRuleString() const;
        std::string toString() const;
        
        void appendDaysString(std::stringstream& builder) const;
        void appendDaysString(std::stringstream& builder, const std::vector<std::string>& daysNames) const;
        
        /**
         * Add a time range (startTime-endTime) to this rule
         *
         * @param startTime startTime to add
         * @param endTime   endTime to add
         */
        void addTimeRange(int startTime, int endTime);
        int timesSize() const;
        void deleteTimeRange(int position);
        
        int calculate(const std::tm& date) const;
    };
    
    struct UnparseableRule : public OpeningHoursRule
    {
    private:
        std::string _ruleString;
        
    public:
        UnparseableRule(const std::string& ruleString);
        virtual ~UnparseableRule();

        bool isOpenedForTime(const std::tm& date, bool checkPrevious) const;
        bool containsPreviousDay(const std::tm& date) const;
        bool hasOverlapTimes() const;
        bool containsDay(const std::tm& date) const;
        bool containsNextDay(const std::tm& date) const;
        bool containsMonth(const std::tm& date) const;

        bool isOpened24_7() const;
        
        std::string toRuleString() const;
        std::string toLocalRuleString() const;
        std::string toString() const;

        std::string getTime(const std::tm& date, bool checkAnotherDay, int limit, bool opening) const;
        
        bool isOpenedForTime(const std::tm& date) const;
        bool contains(const std::tm& date) const;
    };
    
    /**
     * This class contains the entire OpeningHours schema and
     * offers methods to check directly weather something is open
     *
     * @author sander
     */
    struct OpeningHours
    {
    private:
        /**
         * list of the different rules
         */
        std::vector<std::shared_ptr<OpeningHoursRule>> _rules;
        std::string _original;
        
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

        /**
         * add a rule to the opening hours
         *
         * @param r rule to add
         */
        void addRule(std::shared_ptr<OpeningHoursRule>& r);
        
        /**
         * return the list of rules
         *
         * @return the rules
         */
        std::vector<std::shared_ptr<OpeningHoursRule>>& getRules();
        
        /**
         * check if the feature is opened at time "cal"
         *
         * @param cal the time to check
         * @return true if feature is open
         */
        bool isOpenedForTimeV2(const std::tm& date) const;
        
        /**
         * check if the feature is opened at time "cal"
         *
         * @param cal the time to check
         * @return true if feature is open
         */
        bool isOpenedForTime(const std::tm& date) const;

        
        bool isOpened24_7() const;
        
        std::string getNearToOpeningTime(const std::tm& date) const;
        std::string getOpeningTime(const std::tm& date) const;
        std::string getNearToClosingTime(const std::tm& date) const;
        std::string getClosingTime(const std::tm& date) const;
        
        std::string getOpeningDay(const std::tm& date) const;
        std::string getTime(const std::tm& date, int limit, bool opening) const;
        std::string getTimeDay(const std::tm& date, int limit, bool opening) const;
        std::string getTimeAnotherDay(const std::tm& date, int limit, bool opening) const;

        std::string getCurrentRuleTime(const std::tm& date) const;
        
        std::string getCurrentRuleTimeV1(const std::tm& date) const;
        
        std::string toString() const;
        std::string toLocalString() const;
        
        void setOriginal(std::string original);
        std::string getOriginal() const;
    };
    
private:
    std::string openingHours;
    
    static void findInArray(std::shared_ptr<Token>& t, const std::vector<std::string>& list, TokenType tokenType);
    
    static void testOpened(const std::string& time, const std::shared_ptr<OpeningHours>& hours, bool expected);
    static void testParsedAndAssembledCorrectly(const std::string& timeString, const std::shared_ptr<OpeningHours>& hours);
    static std::shared_ptr<OpeningHours> parseOpenedHoursHandleErrors(const std::string& format);

public:
    
    OpeningHoursParser(const std::string& openingHours);
    ~OpeningHoursParser();

    static std::shared_ptr<OpeningHoursRule> parseRuleV2(const std::string& rl);
    static std::shared_ptr<OpeningHoursRule> parseRule(const std::string& rl);

    static std::shared_ptr<OpeningHours> parseOpenedHours(const std::string& format);

    //bool isOpenedForTime(const std::tm& time) const;
    static void runTest();

};

#endif // _OPENINGHOURSPARSER_H
