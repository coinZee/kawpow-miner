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

#include <uv.h>


#include "base/net/https/HttpsServer.h"
#include "base/net/https/HttpsContext.h"
#include "base/net/tls/TlsContext.h"
#include "base/net/tools/NetBuffer.h"


jdkcat::HttpsServer::HttpsServer(const std::shared_ptr<IHttpListener> &listener) :
    m_listener(listener)
{
}


jdkcat::HttpsServer::~HttpsServer()
{
    HttpContext::closeAll();

    delete m_tls;
}


bool jdkcat::HttpsServer::setTls(const TlsConfig &config)
{
    m_tls = TlsContext::create(config);

    return m_tls != nullptr;
}


void jdkcat::HttpsServer::onConnection(uv_stream_t *stream, uint16_t)
{
    auto ctx = new HttpsContext(m_tls, m_listener);
    uv_accept(stream, ctx->stream());

    uv_read_start(ctx->stream(), NetBuffer::onAlloc, onRead); // NOLINT(clang-analyzer-cplusplus.NewDeleteLeaks)
}


void jdkcat::HttpsServer::onRead(uv_stream_t *stream, ssize_t nread, const uv_buf_t *buf)
{
    auto ctx = static_cast<HttpsContext*>(stream->data);
    if (nread >= 0) {
        ctx->append(buf->base, static_cast<size_t>(nread));
    }
    else {
        ctx->close();
    }

    NetBuffer::release(buf);
}
