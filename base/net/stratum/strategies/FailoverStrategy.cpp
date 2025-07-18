/* jdkcat
 * Copyright (c) 2018-2020 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2020 jdkcat       <https://github.com/jdkcat>, <support@jdkcat.com>
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


#include "base/net/stratum/strategies/FailoverStrategy.h"
#include "3rdparty/rapidjson/document.h"
#include "base/kernel/interfaces/IClient.h"
#include "base/kernel/interfaces/IStrategyListener.h"
#include "base/kernel/Platform.h"


jdkcat::FailoverStrategy::FailoverStrategy(const std::vector<Pool> &pools, int retryPause, int retries, IStrategyListener *listener, bool quiet) :
    m_quiet(quiet),
    m_retries(retries),
    m_retryPause(retryPause),
    m_listener(listener)
{
    for (const Pool &pool : pools) {
        add(pool);
    }
}


jdkcat::FailoverStrategy::FailoverStrategy(int retryPause, int retries, IStrategyListener *listener, bool quiet) :
    m_quiet(quiet),
    m_retries(retries),
    m_retryPause(retryPause),
    m_listener(listener)
{
}


jdkcat::FailoverStrategy::~FailoverStrategy()
{
    for (IClient *client : m_pools) {
        client->deleteLater();
    }
}


void jdkcat::FailoverStrategy::add(const Pool &pool)
{
    IClient *client = pool.createClient(static_cast<int>(m_pools.size()), this);

    client->setRetries(m_retries);
    client->setRetryPause(m_retryPause * 1000);
    client->setQuiet(m_quiet);

    m_pools.push_back(client);
}


int64_t jdkcat::FailoverStrategy::submit(const JobResult &result)
{
    if (!isActive()) {
        return -1;
    }

    return active()->submit(result);
}


void jdkcat::FailoverStrategy::connect()
{
    m_pools[m_index]->connect();
}


void jdkcat::FailoverStrategy::resume()
{
    if (!isActive()) {
        return;
    }

    m_listener->onJob(this, active(), active()->job(), rapidjson::Value(rapidjson::kNullType));
}


void jdkcat::FailoverStrategy::setAlgo(const Algorithm &algo)
{
    for (IClient *client : m_pools) {
        client->setAlgo(algo);
    }
}


void jdkcat::FailoverStrategy::setProxy(const ProxyUrl &proxy)
{
    for (IClient *client : m_pools) {
        client->setProxy(proxy);
    }
}


void jdkcat::FailoverStrategy::stop()
{
    for (auto &pool : m_pools) {
        pool->disconnect();
    }

    m_index  = 0;
    m_active = -1;

    m_listener->onPause(this);
}


void jdkcat::FailoverStrategy::tick(uint64_t now)
{
    for (IClient *client : m_pools) {
        client->tick(now);
    }
}


void jdkcat::FailoverStrategy::onClose(IClient *client, int failures)
{
    if (failures == -1) {
        return;
    }

    if (m_active == client->id()) {
        m_active = -1;
        m_listener->onPause(this);
    }

    if (m_index == 0 && failures < m_retries) {
        return;
    }

    if (m_index == static_cast<size_t>(client->id()) && (m_pools.size() - m_index) > 1) {
        m_pools[++m_index]->connect();
    }
}


void jdkcat::FailoverStrategy::onLogin(IClient *client, rapidjson::Document &doc, rapidjson::Value &params)
{
    m_listener->onLogin(this, client, doc, params);
}


void jdkcat::FailoverStrategy::onJobReceived(IClient *client, const Job &job, const rapidjson::Value &params)
{
    if (m_active == client->id()) {
        m_listener->onJob(this, client, job, params);
    }
}


void jdkcat::FailoverStrategy::onLoginSuccess(IClient *client)
{
    int active = m_active;

    if (client->id() == 0 || !isActive()) {
        active = client->id();
    }

    for (size_t i = 1; i < m_pools.size(); ++i) {
        if (active != static_cast<int>(i)) {
            m_pools[i]->disconnect();
        }
    }

    if (active >= 0 && active != m_active) {
        m_index = m_active = active;
        m_listener->onActive(this, client);
    }
}


void jdkcat::FailoverStrategy::onResultAccepted(IClient *client, const SubmitResult &result, const char *error)
{
    m_listener->onResultAccepted(this, client, result, error);
}


void jdkcat::FailoverStrategy::onVerifyAlgorithm(const IClient *client, const Algorithm &algorithm, bool *ok)
{
    m_listener->onVerifyAlgorithm(this, client, algorithm, ok);
}
