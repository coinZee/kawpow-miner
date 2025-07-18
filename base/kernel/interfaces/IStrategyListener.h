/* jdkcat
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018-2020 SChernykh   <https://github.com/SChernykh>
 * Copyright 2016-2020 jdkcat       <https://github.com/jdkcat>, <support@jdkcat.com>
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

#ifndef jdkcat_ISTRATEGYLISTENER_H
#define jdkcat_ISTRATEGYLISTENER_H


#include "3rdparty/rapidjson/fwd.h"
#include "base/tools/Object.h"


namespace jdkcat {


class Algorithm;
class IClient;
class IStrategy;
class Job;
class SubmitResult;


class IStrategyListener
{
public:
    jdkcat_DISABLE_COPY_MOVE(IStrategyListener);

    IStrategyListener()             = default;
    virtual ~IStrategyListener()    = default;

    virtual void onActive(IStrategy *strategy, IClient *client)                                                        = 0;
    virtual void onJob(IStrategy *strategy, IClient *client, const Job &job, const rapidjson::Value &params)           = 0;
    virtual void onLogin(IStrategy *strategy, IClient *client, rapidjson::Document &doc, rapidjson::Value &params)     = 0;
    virtual void onPause(IStrategy *strategy)                                                                          = 0;
    virtual void onResultAccepted(IStrategy *strategy, IClient *client, const SubmitResult &result, const char *error) = 0;
    virtual void onVerifyAlgorithm(IStrategy *strategy, const IClient *client, const Algorithm &algorithm, bool *ok)   = 0;
};


} /* namespace jdkcat */


#endif // jdkcat_ISTRATEGYLISTENER_H
