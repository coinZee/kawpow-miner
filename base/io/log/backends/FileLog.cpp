/* jdkcat
 * Copyright (c) 2019      Spudz76     <https://github.com/Spudz76>
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


#include "base/io/log/backends/FileLog.h"


#include <cassert>
#include <cstring>


jdkcat::FileLog::FileLog(const char *fileName) :
    m_writer(fileName)
{
}


void jdkcat::FileLog::print(uint64_t, int, const char *line, size_t, size_t size, bool colors)
{
    if (!m_writer.isOpen() || colors) {
        return;
    }

    assert(strlen(line) == size);

    m_writer.write(line, size);
}
