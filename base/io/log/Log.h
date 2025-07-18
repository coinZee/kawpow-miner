/* jdkcat
 * Copyright (c) 2019      Spudz76     <https://github.com/Spudz76>
 * Copyright (c) 2018-2021 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2021 jdkcat       <https://github.com/jdkcat>, <support@jdkcat.com>
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef jdkcat_LOG_H
#define jdkcat_LOG_H


#include <cstddef>
#include <cstdint>


namespace jdkcat {


class ILogBackend;
class LogPrivate;


class Log
{
public:
    enum Level : int {
        NONE = -1,
        EMERG,   // system is unusable
        ALERT,   // action must be taken immediately
        CRIT,    // critical conditions
        ERR,     // error conditions
        WARNING, // warning conditions
        NOTICE,  // normal but significant condition
        INFO,    // informational
        DEBUG,   // debug-level messages
    };

    constexpr static size_t kMaxBufferSize = 16384;

    static void add(ILogBackend *backend);
    static void destroy();
    static void init();
    static void print(const char *fmt, ...);
    static void print(Level level, const char *fmt, ...);

    static inline bool isBackground()                   { return m_background; }
    static inline bool isColors()                       { return m_colors; }
    static inline bool isVerbose()                      { return m_verbose > 0; }
    static inline uint32_t verbose()                    { return m_verbose; }
    static inline void setBackground(bool background)   { m_background = background; }
    static inline void setColors(bool colors)           { m_colors = colors; }
    static inline void setVerbose(uint32_t verbose)     { m_verbose = verbose; }

private:
    static bool m_background;
    static bool m_colors;
    static LogPrivate *d;
    static uint32_t m_verbose;
};


#define CSI                 "\x1B["     // Control Sequence Introducer (ANSI spec name)
#define CLEAR               CSI "0m"    // all attributes off
#define BRIGHT_BLACK_S      CSI "0;90m" // somewhat MD.GRAY
#define BLACK_S             CSI "0;30m"

#ifdef jdkcat_OS_APPLE
#   define BLACK_BOLD_S     CSI "0;37m"
#else
#   define BLACK_BOLD_S     CSI "1;30m" // another name for GRAY
#endif

#define RED_S               CSI "0;31m"
#define RED_BOLD_S          CSI "1;31m"
#define GREEN_S             CSI "0;32m"
#define GREEN_BOLD_S        CSI "1;32m"
#define YELLOW_S            CSI "0;33m"
#define YELLOW_BOLD_S       CSI "1;33m"
#define BLUE_S              CSI "0;34m"
#define BLUE_BOLD_S         CSI "1;34m"
#define MAGENTA_S           CSI "0;35m"
#define MAGENTA_BOLD_S      CSI "1;35m"
#define CYAN_S              CSI "0;36m"
#define CYAN_BOLD_S         CSI "1;36m"
#define WHITE_S             CSI "0;37m" // another name for LT.GRAY
#define WHITE_BOLD_S        CSI "1;37m" // actually white

#define RED_BG_BOLD_S       CSI "41;1m"
#define GREEN_BG_BOLD_S     CSI "42;1m"
#define YELLOW_BG_BOLD_S    CSI "43;1m"
#define BLUE_BG_S           CSI "44m"
#define BLUE_BG_BOLD_S      CSI "44;1m"
#define MAGENTA_BG_S        CSI "45m"
#define MAGENTA_BG_BOLD_S   CSI "45;1m"
#define CYAN_BG_S           CSI "46m"
#define CYAN_BG_BOLD_S      CSI "46;1m"

//color wrappings
#define BLACK(x)            BLACK_S x CLEAR
#define BLACK_BOLD(x)       BLACK_BOLD_S x CLEAR
#define RED(x)              RED_S x CLEAR
#define RED_BOLD(x)         RED_BOLD_S x CLEAR
#define GREEN(x)            GREEN_S x CLEAR
#define GREEN_BOLD(x)       GREEN_BOLD_S x CLEAR
#define YELLOW(x)           YELLOW_S x CLEAR
#define YELLOW_BOLD(x)      YELLOW_BOLD_S x CLEAR
#define BLUE(x)             BLUE_S x CLEAR
#define BLUE_BOLD(x)        BLUE_BOLD_S x CLEAR
#define MAGENTA(x)          MAGENTA_S x CLEAR
#define MAGENTA_BOLD(x)     MAGENTA_BOLD_S x CLEAR
#define CYAN(x)             CYAN_S x CLEAR
#define CYAN_BOLD(x)        CYAN_BOLD_S x CLEAR
#define WHITE(x)            WHITE_S x CLEAR
#define WHITE_BOLD(x)       WHITE_BOLD_S x CLEAR

#define RED_BG_BOLD(x)      RED_BG_BOLD_S x CLEAR
#define GREEN_BG_BOLD(x)    GREEN_BG_BOLD_S x CLEAR
#define YELLOW_BG_BOLD(x)   YELLOW_BG_BOLD_S x CLEAR
#define BLUE_BG(x)          BLUE_BG_S x CLEAR
#define BLUE_BG_BOLD(x)     BLUE_BG_BOLD_S x CLEAR
#define MAGENTA_BG(x)       MAGENTA_BG_S x CLEAR
#define MAGENTA_BG_BOLD(x)  MAGENTA_BG_BOLD_S x CLEAR
#define CYAN_BG(x)          CYAN_BG_S x CLEAR
#define CYAN_BG_BOLD(x)     CYAN_BG_BOLD_S x CLEAR


#define LOG_EMERG(x, ...)   jdkcat::Log::print(jdkcat::Log::EMERG,   x, ##__VA_ARGS__)
#define LOG_ALERT(x, ...)   jdkcat::Log::print(jdkcat::Log::ALERT,   x, ##__VA_ARGS__)
#define LOG_CRIT(x, ...)    jdkcat::Log::print(jdkcat::Log::CRIT,    x, ##__VA_ARGS__)
#define LOG_ERR(x, ...)     jdkcat::Log::print(jdkcat::Log::ERR,     x, ##__VA_ARGS__)
#define LOG_WARN(x, ...)    jdkcat::Log::print(jdkcat::Log::WARNING, x, ##__VA_ARGS__)
#define LOG_NOTICE(x, ...)  jdkcat::Log::print(jdkcat::Log::NOTICE,  x, ##__VA_ARGS__)
#define LOG_INFO(x, ...)    jdkcat::Log::print(jdkcat::Log::INFO,    x, ##__VA_ARGS__)
#define LOG_VERBOSE(x, ...) if (jdkcat::Log::verbose() > 0) { jdkcat::Log::print(jdkcat::Log::INFO, x, ##__VA_ARGS__); }
#define LOG_V1(x, ...)      if (jdkcat::Log::verbose() > 0) { jdkcat::Log::print(jdkcat::Log::INFO, x, ##__VA_ARGS__); }
#define LOG_V2(x, ...)      if (jdkcat::Log::verbose() > 1) { jdkcat::Log::print(jdkcat::Log::INFO, x, ##__VA_ARGS__); }
#define LOG_V3(x, ...)      if (jdkcat::Log::verbose() > 2) { jdkcat::Log::print(jdkcat::Log::INFO, x, ##__VA_ARGS__); }
#define LOG_V4(x, ...)      if (jdkcat::Log::verbose() > 3) { jdkcat::Log::print(jdkcat::Log::INFO, x, ##__VA_ARGS__); }
#define LOG_V5(x, ...)      if (jdkcat::Log::verbose() > 4) { jdkcat::Log::print(jdkcat::Log::INFO, x, ##__VA_ARGS__); }

#ifdef APP_DEBUG
#   define LOG_DEBUG(x, ...) jdkcat::Log::print(jdkcat::Log::DEBUG, x, ##__VA_ARGS__)
#else
#   define LOG_DEBUG(x, ...)
#endif

#if defined(APP_DEBUG) || defined(APP_DEVEL)
#   define LOG_DEBUG_ERR(x, ...)  jdkcat::Log::print(jdkcat::Log::ERR,     x, ##__VA_ARGS__)
#   define LOG_DEBUG_WARN(x, ...) jdkcat::Log::print(jdkcat::Log::WARNING, x, ##__VA_ARGS__)
#else
#   define LOG_DEBUG_ERR(x, ...)
#   define LOG_DEBUG_WARN(x, ...)
#endif


} /* namespace jdkcat */


#endif /* jdkcat_LOG_H */
