/* jdkcat
 * Copyright (c) 2018-2023 SChernykh   <https://github.com/SChernykh>
 * Copyright (c) 2016-2023 jdkcat       <https://github.com/jdkcat>, <support@jdkcat.com>
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

#ifndef jdkcat_HTTPD_H
#define jdkcat_HTTPD_H


#include "base/kernel/interfaces/IBaseListener.h"
#include "base/net/http/HttpListener.h"


namespace jdkcat {


class Base;
class HttpServer;
class HttpsServer;
class TcpServer;


class Httpd : public IBaseListener, public IHttpListener
{
public:
    jdkcat_DISABLE_COPY_MOVE_DEFAULT(Httpd)

    explicit Httpd(Base *base);
    ~Httpd() override;

    inline bool isBound() const { return m_server != nullptr; }

    bool start();
    void stop();

protected:
    void onConfigChanged(Config *config, Config *previousConfig) override;
    void onHttpData(const HttpData &data) override;

private:
    int auth(const HttpData &req) const;

    const Base *m_base;
    std::shared_ptr<IHttpListener> m_httpListener;
    TcpServer *m_server     = nullptr;
    uint16_t m_port         = 0;

#   ifdef jdkcat_FEATURE_TLS
    HttpsServer *m_http     = nullptr;
#   else
    HttpServer *m_http      = nullptr;
#   endif
};


} // namespace jdkcat


#endif // jdkcat_HTTPD_H
