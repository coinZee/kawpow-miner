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

#ifndef jdkcat_HTTPLISTENER_H
#define jdkcat_HTTPLISTENER_H


#include "base/kernel/interfaces/IHttpListener.h"


namespace jdkcat {


class HttpListener : public IHttpListener
{
public:
    inline HttpListener(IHttpListener *listener, const char *tag = nullptr) :
#       ifdef APP_DEBUG
        m_tag(tag),
#       endif
        m_listener(listener)
    {}

protected:
    void onHttpData(const HttpData &data) override;

private:
#   ifdef APP_DEBUG
    const char *m_tag;
#   endif
    IHttpListener *m_listener;
};


} /* namespace jdkcat */


#endif // jdkcat_HTTPLISTENER_H
