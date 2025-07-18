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

#include "base/crypto/Coin.h"
#include "3rdparty/rapidjson/document.h"
#include "base/io/json/Json.h"
#include "base/io/log/Log.h"


#include <cstring>


#ifdef _MSC_VER
#   define strcasecmp _stricmp
#endif


namespace jdkcat {


struct CoinInfo
{
    const Algorithm::Id algorithm;
    const char *code;
    const char *name;
    const uint64_t target;
    const uint64_t units;
    const char *tag;
};


static const CoinInfo coinInfo[] = {
    { Algorithm::INVALID,         nullptr,    nullptr,        0,      0,              nullptr },
    { Algorithm::RX_0,            "XMR",      "Monero",       120,    1000000000000,  YELLOW_BG_BOLD( WHITE_BOLD_S " monero  ") },
    { Algorithm::CN_R,            "SUMO",     "Sumokoin",     240,    1000000000,     BLUE_BG_BOLD(   WHITE_BOLD_S " sumo    ") },
    { Algorithm::RX_ARQ,          "ARQ",      "ArQmA",        120,    1000000000,     BLUE_BG_BOLD(   WHITE_BOLD_S " arqma   ") },
    { Algorithm::RX_GRAFT,        "GRFT",     "Graft",        120,    10000000000,    BLUE_BG_BOLD(   WHITE_BOLD_S " graft   ") },
    { Algorithm::KAWPOW_RVN,      "RVN",      "Ravencoin",    0,      0,              BLUE_BG_BOLD(   WHITE_BOLD_S " raven   ") },
    { Algorithm::RX_WOW,          "WOW",      "Wownero",      300,    100000000000,   MAGENTA_BG_BOLD(WHITE_BOLD_S " wownero ") },
    { Algorithm::RX_0,            "ZEPH",     "Zephyr",       120,    1000000000000,  BLUE_BG_BOLD(   WHITE_BOLD_S " zephyr  ") },
    { Algorithm::RX_0,            "Townforge","Townforge",    30,     100000000,      MAGENTA_BG_BOLD(WHITE_BOLD_S " townforge ") },
    { Algorithm::RX_YADA,         "YDA",      "YadaCoin",     120,    100000000,      BLUE_BG_BOLD(   WHITE_BOLD_S " yada    ") },
};


static_assert(Coin::MAX == sizeof(coinInfo) / sizeof(coinInfo[0]), "size mismatch");


const char *Coin::kDisabled = "DISABLED_COIN";
const char *Coin::kField    = "coin";
const char *Coin::kUnknown  = "UNKNOWN_COIN";


} /* namespace jdkcat */


jdkcat::Coin::Coin(const rapidjson::Value &value)
{
    if (value.IsString()) {
        m_id = parse(value.GetString());
    }
    else if (value.IsObject() && !value.ObjectEmpty()) {
        m_id = parse(Json::getString(value, kField));
    }
}


jdkcat::Algorithm jdkcat::Coin::algorithm(uint8_t) const
{
    return coinInfo[m_id].algorithm;
}


const char *jdkcat::Coin::code() const
{
    return coinInfo[m_id].code;
}


const char *jdkcat::Coin::name() const
{
    return coinInfo[m_id].name;
}


rapidjson::Value jdkcat::Coin::toJSON() const
{
    using namespace rapidjson;

    return isValid() ? Value(StringRef(code())) : Value(kNullType);
}


uint64_t jdkcat::Coin::target(uint8_t) const
{
    return coinInfo[m_id].target;
}


uint64_t jdkcat::Coin::units() const
{
    return coinInfo[m_id].units;
}


jdkcat::Coin::Id jdkcat::Coin::parse(const char *name)
{
    if (name == nullptr || strlen(name) < 3) {
        return INVALID;
    }

    for (uint32_t i = 1; i < MAX; ++i) {
        if (strcasecmp(name, coinInfo[i].code) == 0 || strcasecmp(name, coinInfo[i].name) == 0) {
            return static_cast<Id>(i);
        }
    }

    return INVALID;
}


const char *jdkcat::Coin::tag(Id id)
{
    return coinInfo[id].tag;
}
