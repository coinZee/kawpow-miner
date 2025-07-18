/* jdkcat
 * Copyright (c) 2018      Lee Clagett <https://github.com/vtnerd>
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

#ifndef jdkcat_CLIENT_TLS_H
#define jdkcat_CLIENT_TLS_H


using BIO       = struct bio_st;
using SSL       = struct ssl_st;
using SSL_CTX   = struct ssl_ctx_st;
using X509      = struct x509_st;


#include "base/net/stratum/Client.h"
#include "base/tools/Object.h"


namespace jdkcat {


class Client::Tls
{
public:
    jdkcat_DISABLE_COPY_MOVE_DEFAULT(Tls)

    Tls(Client *client);
    ~Tls();

    bool handshake(const char* servername);
    bool send(const char *data, size_t size);
    const char *fingerprint() const;
    const char *version() const;
    void read(const char *data, size_t size);

private:
    bool send();
    bool verify(X509 *cert);
    bool verifyFingerprint(X509 *cert);

    BIO *m_read     = nullptr;
    BIO *m_write    = nullptr;
    bool m_ready    = false;
    char m_fingerprint[32 * 2 + 8]{};
    Client *m_client;
    SSL *m_ssl      = nullptr;
    SSL_CTX *m_ctx;
};


} /* namespace jdkcat */


#endif /* jdkcat_CLIENT_TLS_H */
