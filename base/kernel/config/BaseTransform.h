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

#ifndef jdkcat_BASETRANSFORM_H
#define jdkcat_BASETRANSFORM_H


#include "3rdparty/rapidjson/document.h"
#include "base/crypto/Coin.h"
#include "base/kernel/interfaces/IConfigTransform.h"


struct option;


namespace jdkcat {


class IConfigTransform;
class JsonChain;
class Process;


class BaseTransform : public IConfigTransform
{
public:
    static void load(JsonChain &chain, Process *process, IConfigTransform &transform);

protected:
    void finalize(rapidjson::Document &doc) override;
    void transform(rapidjson::Document &doc, int key, const char *arg) override;


    template<typename T>
    inline void set(rapidjson::Document &doc, const char *key, T value) { set<T>(doc, doc, key, value); }


    template<typename T>
    inline void set(rapidjson::Document &doc, const char *objKey, const char *key, T value)
    {
        if (!doc.HasMember(objKey)) {
            doc.AddMember(rapidjson::StringRef(objKey), rapidjson::kObjectType, doc.GetAllocator());
        }

        set<T>(doc, doc[objKey], key, value);
    }


    template<typename T>
    inline void add(rapidjson::Document &doc, const char *arrayKey, const char *key, T value, bool force = false)
    {
        auto &allocator = doc.GetAllocator();

        if (!doc.HasMember(arrayKey)) {
            doc.AddMember(rapidjson::StringRef(arrayKey), rapidjson::kArrayType, allocator);
        }

        rapidjson::Value &array = doc[arrayKey];
        if (force || array.Size() == 0) {
            array.PushBack(rapidjson::kObjectType, allocator);
        }

        set<T>(doc, array[array.Size() - 1], key, value);
    }


    template<typename T>
    inline void set(rapidjson::Document &doc, rapidjson::Value &obj, const char *key, T value)
    {
        if (obj.HasMember(key)) {
            obj[key] = value;
        }
        else {
            obj.AddMember(rapidjson::StringRef(key), value, doc.GetAllocator());
        }
    }

protected:
    Algorithm m_algorithm;
    Coin m_coin;


private:
    void transformBoolean(rapidjson::Document &doc, int key, bool enable);
    void transformUint64(rapidjson::Document &doc, int key, uint64_t arg);

    bool m_http = false;
};


template<>
inline void BaseTransform::set(rapidjson::Document &doc, rapidjson::Value &obj, const char *key, const char *value)
{
    auto &allocator = doc.GetAllocator();

    if (obj.HasMember(key)) {
        obj[key] = rapidjson::Value(value, allocator);
    }
    else {
        obj.AddMember(rapidjson::StringRef(key), rapidjson::Value(value, allocator), allocator);
    }
}


} // namespace jdkcat


#endif /* jdkcat_BASETRANSFORM_H */
