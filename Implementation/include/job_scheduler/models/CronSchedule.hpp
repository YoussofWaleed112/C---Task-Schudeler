#pragma once
#include "ISchedule.hpp"
#include <string>

class CronSchedule : public ISchedule {
public:
    explicit CronSchedule(const std::string& expression);

    std::optional<TimePoint> nextRunAfter(TimePoint from) const override;
    std::string type()      const override;
    std::string serialize() const override;

    static bool isValid(const std::string& expression);

private:
    std::string _expression;

    struct Fields { int minute, hour, dom, month, dow; };
    Fields _fields;

    static Fields parse(const std::string& expression);
    static bool fieldMatches(int value, int field);
};
