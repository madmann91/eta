#include "core/log.h"

#include <string.h>

typedef enum log_msg_type {
    LOG_ERROR,
    LOG_WARNING,
    LOG_NOTE
} LogMsgType;

static void log_msg(
    struct log* log,
    LogMsgType msg_type,
    const FileLoc* loc,
    const char* format_str,
    const FormatArg* args)
{
    static const struct format_style header_styles[] = {
        { STYLE_BOLD, COLOR_RED },
        { STYLE_BOLD, COLOR_YELLOW },
        { STYLE_BOLD, COLOR_BLUE }
    };
    static const char* headers[] = { "error", "warning", "note" };

    if (msg_type == LOG_ERROR) log->error_count++;
    else if (msg_type == LOG_WARNING) log->warning_count++;

    format(&log->state, "{$}{s}{$}: ", (FormatArg[]) {
        { .style = header_styles[msg_type] },
        { .s = headers[msg_type] },
        { .style = reset_style } });
    format(&log->state, format_str, args);
    format(&log->state, "\n", NULL);
    if (loc && loc->file_name) {
        static const struct format_style loc_style = { STYLE_BOLD, COLOR_WHITE };
        format(
            &log->state,
            memcmp(&loc->begin, &loc->end, sizeof(loc->begin))
                ? "  in {$}{s}({u32}, {u32} -- {u32}, {u32}){$}\n"
                : "  in {$}{s}({u32}, {u32}){$}\n",
            (FormatArg[]) {
                { .style = loc_style },
                { .s = loc->file_name ? loc->file_name : "<unknown>" },
                { .u32 = loc->begin.row },
                { .u32 = loc->begin.col },
                { .u32 = loc->end.row },
                { .u32 = loc->end.col },
                { .style = reset_style }
            });
    }
}

void log_error(struct log* log, const FileLoc* loc, const char* format_str, const FormatArg* args) {
    log_msg(log, LOG_ERROR, loc, format_str, args);
}

void log_warning(struct log* log, const FileLoc* loc, const char* format_str, const FormatArg* args) {
    log_msg(log, LOG_WARNING, loc, format_str, args);
}

void log_note(struct log* log, const FileLoc* loc, const char* format_str, const FormatArg* args) {
    log_msg(log, LOG_NOTE, loc, format_str, args);
}
