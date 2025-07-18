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

#ifndef jdkcat_JSONCHAIN_H
#define jdkcat_JSONCHAIN_H


#include <vector>


#include "3rdparty/rapidjson/document.h"
#include "base/kernel/interfaces/IJsonReader.h"
#include "base/tools/String.h"


namespace jdkcat {


class JsonChain : public IJsonReader
{
public:
    JsonChain();

    bool add(rapidjson::Document &&doc);
    bool addFile(const char *fileName);
    bool addRaw(const char *json);

    void dump(const char *fileName);

    inline const String &fileName() const { return m_fileName; }
    inline size_t size() const            { return m_chain.size(); }

protected:
    inline bool isEmpty() const override  { return m_chain.empty(); }

    bool getBool(const char *key, bool defaultValue = false) const override;
    const char *getString(const char *key, const char *defaultValue = nullptr) const override;
    const rapidjson::Value &getArray(const char *key) const override;
    const rapidjson::Value &getObject(const char *key) const override;
    const rapidjson::Value &getValue(const char *key) const override;
    const rapidjson::Value &object() const override;
    double getDouble(const char *key, double defaultValue = 0) const override;
    int getInt(const char *key, int defaultValue = 0) const override;
    int64_t getInt64(const char *key, int64_t defaultValue = 0) const override;
    String getString(const char *key, size_t maxSize) const override;
    uint64_t getUint64(const char *key, uint64_t defaultValue = 0) const override;
    unsigned getUint(const char *key, unsigned defaultValue = 0) const override;

private:
    std::vector<rapidjson::Document> m_chain;
    String m_fileName;
};


} /* namespace jdkcat */


#endif /* jdkcat_JSONCHAIN_H */
