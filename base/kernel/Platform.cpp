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

#include "base/kernel/Platform.h"


#include <cstring>
#include <uv.h>


#ifdef jdkcat_FEATURE_TLS
#   include <openssl/ssl.h>
#   include <openssl/err.h>
#endif


namespace jdkcat {

String Platform::m_userAgent;

} // namespace jdkcat


void jdkcat::Platform::init(const char *userAgent)
{
#   ifdef jdkcat_FEATURE_TLS
    SSL_library_init();
    SSL_load_error_strings();

#   if OPENSSL_VERSION_NUMBER < 0x30000000L || defined(LIBRESSL_VERSION_NUMBER)
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
#   endif

    OpenSSL_add_all_digests();
#   endif

    if (userAgent) {
        m_userAgent = userAgent;
    }
    else {
        m_userAgent = createUserAgent();
    }
}
