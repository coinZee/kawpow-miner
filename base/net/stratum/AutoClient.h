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

#ifndef jdkcat_AUTOCLIENT_H
#define jdkcat_AUTOCLIENT_H


#include "base/net/stratum/EthStratumClient.h"


#include <utility>


namespace jdkcat {


class AutoClient : public EthStratumClient
{
public:
    jdkcat_DISABLE_COPY_MOVE_DEFAULT(AutoClient)

    AutoClient(int id, const char *agent, IClientListener *listener);
    ~AutoClient() override = default;

protected:
    inline void login() override    { Client::login(); }

    bool handleResponse(int64_t id, const rapidjson::Value &result, const rapidjson::Value &error) override;
    bool parseLogin(const rapidjson::Value &result, int *code) override;
    int64_t submit(const JobResult &result) override;
    void parseNotification(const char *method, const rapidjson::Value &params, const rapidjson::Value &error) override;

private:
    enum Mode {
        DEFAULT_MODE,
        ETH_MODE
    };

    Mode m_mode = DEFAULT_MODE;
};


} /* namespace jdkcat */


#endif /* jdkcat_AUTOCLIENT_H */
