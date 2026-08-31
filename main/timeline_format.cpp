#include "timeline_format.h"

#include <cstdio>
#include <ctime>

#include "timezone_service.h"

namespace timeline_format {

using recording_archive_service::RecordingTag;

namespace {

constexpr const char* kWeekdayAbbrevDe[7] = {"So", "Mo", "Di", "Mi", "Do", "Fr", "Sa"};
constexpr const char* kWeekdayFullDe[7] = {"Sonntag",    "Montag", "Dienstag", "Mittwoch",
                                            "Donnerstag", "Freitag", "Samstag"};
constexpr const char* kMonthAbbrevDe[12] = {"Jan", "Feb", "Mär", "Apr", "Mai", "Jun",
                                             "Jul", "Aug", "Sep", "Okt", "Nov", "Dez"};

}  // namespace

const char* WeekdayAbbrevDe(int tm_wday)
{
    return (tm_wday >= 0 && tm_wday < 7) ? kWeekdayAbbrevDe[tm_wday] : "";
}

const char* WeekdayFullDe(int tm_wday)
{
    return (tm_wday >= 0 && tm_wday < 7) ? kWeekdayFullDe[tm_wday] : "";
}

const char* MonthAbbrevDe(int tm_mon)
{
    return (tm_mon >= 0 && tm_mon < 12) ? kMonthAbbrevDe[tm_mon] : "";
}

std::string DateKey(const std::string& created_local_date)
{
    const auto space = created_local_date.find(' ');
    return space == std::string::npos ? created_local_date : created_local_date.substr(0, space);
}

std::string FormatDateLabel(const std::string& created_local_date)
{
    const std::string day_key = DateKey(created_local_date);

    // "Heute" only while the recording's day equals the current local date. GetSnapshot().current_date
    // is the live "YYYY-MM-DD", so this stops reading "Heute" as soon as the date rolls over.
    if (!day_key.empty() &&
        day_key == timezone_service::GetSnapshot().runtime.current_date) {
        return "Heute";
    }

    int year = 0;
    int month = 0;
    int day = 0;
    if (std::sscanf(day_key.c_str(), "%d-%d-%d", &year, &month, &day) == 3) {
        std::tm tm = {};
        tm.tm_year = year - 1900;
        tm.tm_mon = month - 1;
        tm.tm_mday = day;
        std::time_t stamp = std::mktime(&tm);
        if (stamp != static_cast<std::time_t>(-1)) {
            std::tm local = {};
            localtime_r(&stamp, &local);
            return std::string(WeekdayAbbrevDe(local.tm_wday)) + ", " +
                   std::to_string(local.tm_mday) + ". " + MonthAbbrevDe(local.tm_mon);
        }
    }
    return created_local_date.empty() ? "Heute" : created_local_date;
}

std::string FormatTimeLabel(bool time_valid, int64_t created_unix_seconds)
{
    if (time_valid && created_unix_seconds > 0) {
        std::time_t stamp = static_cast<std::time_t>(created_unix_seconds);
        std::tm local = {};
        localtime_r(&stamp, &local);
        char buffer[16] = {};
        if (std::strftime(buffer, sizeof(buffer), "%H:%M", &local) > 0) {
            return buffer;
        }
    }
    return "--:--";
}

std::string FormatDurationLabel(uint32_t duration_ms)
{
    const uint32_t seconds = duration_ms / 1000U;
    if (seconds < 60U) {
        return std::to_string(seconds) + "s";
    }
    return std::to_string(seconds / 60U) + "m";
}

std::string TrimTranscript(const std::string& text)
{
    const auto begin = text.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) {
        return {};
    }
    const auto end = text.find_last_not_of(" \t\r\n");
    return text.substr(begin, end - begin + 1);
}

std::string TagText(RecordingTag tag)
{
    switch (tag) {
        case RecordingTag::kTask:
            return "Aufgabe";
        case RecordingTag::kIdea:
            return "Idee";
        case RecordingTag::kNote:
        default:
            return "Notiz";
    }
}

}  // namespace timeline_format
