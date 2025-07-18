/* jdkcat
 * Copyright (c) 2014-2019 heapwolf    <https://github.com/heapwolf>
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


#ifndef jdkcat_HTTPSCLIENT_H
#define jdkcat_HTTPSCLIENT_H


using BIO       = struct bio_st;
using SSL_CTX   = struct ssl_ctx_st;
using SSL       = struct ssl_st;
using X509      = struct x509_st;


#include "base/net/http/HttpClient.h"
#include "base/tools/String.h"


namespace jdkcat {


class HttpsClient : public HttpClient
{
public:
    jdkcat_DISABLE_COPY_MOVE_DEFAULT(HttpsClient)

    HttpsClient(const char *tag, FetchRequest &&req, const std::weak_ptr<IHttpListener> &listener);
    ~HttpsClient() override;

    const char *tlsFingerprint() const override;
    const char *tlsVersion() const override;

protected:
    void handshake() override;
    void read(const char *data, size_t size) override;

private:
    void write(std::string &&data, bool close) override;

    bool verify(X509 *cert);
    bool verifyFingerprint(X509 *cert);
    void flush(bool close);

    BIO *m_read                         = nullptr;
    BIO *m_write                        = nullptr;
    bool m_ready                        = false;
    char m_fingerprint[32 * 2 + 8]{};
    SSL *m_ssl                          = nullptr;
    SSL_CTX *m_ctx                      = nullptr;
};


} // namespace jdkcat


#endif // jdkcat_HTTPSCLIENT_H
