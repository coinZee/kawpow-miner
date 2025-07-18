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

#ifndef jdkcat_PROCESS_H
#define jdkcat_PROCESS_H


#include "base/tools/Arguments.h"


#ifdef WIN32
#   define jdkcat_DIR_SEPARATOR "\\"
#else
#   define jdkcat_DIR_SEPARATOR "/"
#endif


namespace jdkcat {


class Process
{
public:
    enum Location {
        ExeLocation,
        CwdLocation,
        DataLocation,
        HomeLocation,
        TempLocation
    };

    Process(int argc, char **argv);

    static int pid();
    static int ppid();
    static String exepath();
    static String location(Location location, const char *fileName = nullptr);

    inline const Arguments &arguments() const { return m_arguments; }

private:
    Arguments m_arguments;
};


} /* namespace jdkcat */


#endif /* jdkcat_PROCESS_H */
