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

#include "base/net/http/Http.h"
#include "3rdparty/rapidjson/document.h"
#include "base/io/json/Json.h"


namespace jdkcat {


const char *Http::kEnabled    = "enabled";
const char *Http::kHost       = "host";
const char *Http::kLocalhost  = "127.0.0.1";
const char *Http::kPort       = "port";
const char *Http::kRestricted = "restricted";
const char *Http::kToken      = "access-token";


} // namespace jdkcat


jdkcat::Http::Http() :
    m_host(kLocalhost)
{
}


bool jdkcat::Http::isEqual(const Http &other) const
{
    return other.m_enabled    == m_enabled &&
           other.m_restricted == m_restricted &&
           other.m_host       == m_host &&
           other.m_token      == m_token &&
           other.m_port       == m_port;
}


rapidjson::Value jdkcat::Http::toJSON(rapidjson::Document &doc) const
{
    using namespace rapidjson;
    auto &allocator = doc.GetAllocator();

    Value obj(kObjectType);

    obj.AddMember(StringRef(kEnabled),    m_enabled, allocator);
    obj.AddMember(StringRef(kHost),       m_host.toJSON(), allocator);
    obj.AddMember(StringRef(kPort),       m_port, allocator);
    obj.AddMember(StringRef(kToken),      m_token.toJSON(), allocator);
    obj.AddMember(StringRef(kRestricted), m_restricted, allocator);

    return obj;
}


void jdkcat::Http::load(const rapidjson::Value &http)
{
    if (!http.IsObject()) {
        return;
    }

    m_enabled    = Json::getBool(http, kEnabled);
    m_restricted = Json::getBool(http, kRestricted, true);
    m_host       = Json::getString(http, kHost, kLocalhost);
    m_token      = Json::getString(http, kToken);

    setPort(Json::getInt(http, kPort));
}


void jdkcat::Http::setPort(int port)
{
    if (port >= 0 && port <= 65536) {
        m_port = static_cast<uint16_t>(port);
    }
}
