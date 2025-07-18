/* jdkcat
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

#ifndef jdkcat_URL_H
#define jdkcat_URL_H


#include "base/tools/String.h"


namespace jdkcat {


class Url
{
public:
    enum Scheme {
        UNSPECIFIED,
        STRATUM,
        DAEMON,
        SOCKS5
    };

    Url() = default;
    Url(const char *url);
    Url(const char *host, uint16_t port, bool tls = false, Scheme scheme = UNSPECIFIED);

    inline bool isTLS() const                           { return m_tls; }
    inline bool isValid() const                         { return !m_host.isNull() && m_port > 0; }
    inline const String &host() const                   { return m_host; }
    inline const String &url() const                    { return m_url; }
    inline Scheme scheme() const                        { return m_scheme; }
    inline uint16_t port() const                        { return m_port; }

    inline bool operator!=(const Url &other) const      { return !isEqual(other); }
    inline bool operator==(const Url &other) const      { return isEqual(other); }

    bool isEqual(const Url &other) const;

protected:
    bool parse(const char *url);
    bool parseIPv6(const char *addr);

    bool m_tls      = false;
    Scheme m_scheme = UNSPECIFIED;
    String m_host;
    String m_url;
    uint16_t m_port = 3333;
};


} /* namespace jdkcat */


#endif /* jdkcat_URL_H */
