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

#ifndef jdkcat_HTTP_H
#define jdkcat_HTTP_H


#include "base/tools/String.h"


namespace jdkcat {


class Http
{
public:
    static const char *kEnabled;
    static const char *kHost;
    static const char *kLocalhost;
    static const char *kPort;
    static const char *kRestricted;
    static const char *kToken;

    Http();

    inline bool isAuthRequired() const         { return !m_restricted || !m_token.isNull(); }
    inline bool isEnabled() const              { return m_enabled; }
    inline bool isRestricted() const           { return m_restricted; }
    inline const String &host() const          { return m_host; }
    inline const String &token() const         { return m_token; }
    inline uint16_t port() const               { return m_port; }
    inline void setEnabled(bool enabled)       { m_enabled = enabled; }
    inline void setHost(const char *host)      { m_host = host; }
    inline void setRestricted(bool restricted) { m_restricted = restricted; }
    inline void setToken(const char *token)    { m_token = token; }

    inline bool operator!=(const Http &other) const    { return !isEqual(other); }
    inline bool operator==(const Http &other) const    { return isEqual(other); }

    bool isEqual(const Http &other) const;
    rapidjson::Value toJSON(rapidjson::Document &doc) const;
    void load(const rapidjson::Value &http);
    void setPort(int port);

private:
    bool m_enabled      = false;
    bool m_restricted   = true;
    String m_host;
    String m_token;
    uint16_t m_port     = 0;
};


} // namespace jdkcat


#endif // jdkcat_HTTP_H

