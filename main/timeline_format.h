#ifndef TIMELINE_FORMAT_H_
#define TIMELINE_FORMAT_H_

#include <cstdint>
#include <string>

#include "recording_archive_service.h"

// Shared formatters for the recording timeline shown on the Notes, Todos, and Follow-up pages (and
// the sticky-note overlay, which reuses the same content shape).
namespace timeline_format {

// Extract the "YYYY-MM-DD" day key from a created_local_date that may be either "YYYY-MM-DD" or
// "YYYY-MM-DD HH:MM:SS", so recordings from the same day group under one date chip. This is the
// stable grouping identity -- it never changes as the calendar rolls over.
std::string DateKey(const std::string& created_local_date);

// Human label for a recording's day: "Heute" while its day matches the current local date,
// otherwise the absolute "Mo, 10. Jul". Because the comparison is against the live current date,
// a recording labelled "Heute" automatically re-labels to its absolute date once midnight passes.
// Empty / unparseable dates fall back to "Heute".
std::string FormatDateLabel(const std::string& created_local_date);

// Clock time for a recording's header: 24-hour "HH:MM" when the timestamp is valid, otherwise
// "--:--".
std::string FormatTimeLabel(bool time_valid, int64_t created_unix_seconds);

// Compact duration: "<Ns>" under a minute, otherwise "<Nm>".
std::string FormatDurationLabel(uint32_t duration_ms);

// Trim leading/trailing whitespace from a transcript (empty when all whitespace).
std::string TrimTranscript(const std::string& text);

// Human tag label: kTask -> "Aufgabe", kIdea -> "Idee", otherwise "Notiz".
std::string TagText(recording_archive_service::RecordingTag tag);

// German weekday/month name lookups for on-device date rendering. ESP-IDF's default "C" locale
// has no German locale data, so strftime can't produce these directly -- callers build the date
// string by hand from std::tm fields using these instead.
const char* WeekdayAbbrevDe(int tm_wday);
const char* WeekdayFullDe(int tm_wday);
const char* MonthAbbrevDe(int tm_mon);

}  // namespace timeline_format

#endif  // TIMELINE_FORMAT_H_
