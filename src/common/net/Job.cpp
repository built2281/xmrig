/* XMRig
 * Copyright 2010      Jeff Garzik <jgarzik@pobox.com>
 * Copyright 2012-2014 pooler      <pooler@litecoinpool.org>
 * Copyright 2014      Lucas Jones <https://github.com/lucasjones>
 * Copyright 2014-2016 Wolf9466    <https://github.com/OhGodAPet>
 * Copyright 2016      Jay D Dee   <jayddee246@gmail.com>
 * Copyright 2017-2018 XMR-Stak    <https://github.com/fireice-uk>, <https://github.com/psychocrypt>
 * Copyright 2018      Lee Clagett <https://github.com/vtnerd>
 * Copyright 2016-2018 XMRig       <https://github.com/xmrig>, <support@xmrig.com>
 * Copyright 2018      Team-Hycon  <https://github.com/Team-Hycon>
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


#include <assert.h>
#include <string.h>


#include "common/net/Job.h"


static inline unsigned char hf_hex2bin(char c, bool &err)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    else if (c >= 'a' && c <= 'f') {
        return c - 'a' + 0xA;
    }
    else if (c >= 'A' && c <= 'F') {
        return c - 'A' + 0xA;
    }

    err = true;
    return 0;
}


static inline char hf_bin2hex(unsigned char c)
{
    if (c <= 0x9) {
        return '0' + c;
    }

    return 'a' - 0xA + c;
}


Job::Job() :
    m_nicehash(false),
    m_poolId(-2),
    m_threadId(-1),
    m_size(0),
    m_diff(0),
    m_target(0),
    m_blob()
{
}


Job::Job(int poolId, bool nicehash, xmrig::Algorithm algorithm, const xmrig::Id &clientId) :
    m_nicehash(nicehash),
    m_poolId(poolId),
    m_threadId(-1),
    m_size(0),
    m_diff(0),
    m_target(0),
    m_blob(),
    m_algorithm(algorithm),
    m_clientId(clientId)
{
}


Job::~Job()
{
}


bool Job::setBlob(const char *blob)
{
    if (!blob) {
        return false;
    }

    m_size = strlen(blob);
    if (m_size % 2 != 0) {
        return false;
    }

    m_size /= 2;
    if (m_size < LEN::PREHASH || m_size >= sizeof(m_blob)) {
        return false;
    }

    if (!fromHex(blob, (int) m_size*2, m_blob)) {
        return false;
    }

    if (*nonce() != 0 && !m_nicehash) {
        m_nicehash = true;
    }

#   ifdef XMRIG_PROXY_PROJECT
    memset(m_rawBlob, 0, sizeof(m_rawBlob));
    memcpy(m_rawBlob, blob, m_size * 2);
#   endif

    m_size = LEN::BLOB;
    return true;
}


bool Job::setTarget(const char *target, const int zeroCnt)
{
    if (!target) {
        return false;
    }

    const size_t len = strlen(target);

    if( len <= LEN::DIFF_HEX) {
        char strAllTarget[LEN::RESULT_HEX + 1] = {0};
        memset(strAllTarget, 'f', LEN::RESULT_HEX);
        memset(strAllTarget, '0', zeroCnt);
        memcpy(strAllTarget + zeroCnt, target, LEN::DIFF_HEX);

        for(int i=0; i<LEN::DIFF_ARR_CNT; ++i) 
        {   
            uint64_t tmp = 0;
            char str[LEN::DIFF_ONE_HEX];
            memset(str, '0', LEN::DIFF_ONE_HEX);
            memcpy(str, &strAllTarget[i*LEN::DIFF_ONE_HEX], LEN::DIFF_ONE_HEX);

            if (!fromHex(str, LEN::DIFF_ONE_HEX, reinterpret_cast<unsigned char*>(&tmp)) ) {
                return false;
            }
            
            m_targetAll[LEN::DIFF_ARR_CNT - i -1] = tmp;
        }
        m_target = m_targetAll[3];
    } 
    else if (len <= 8) {
        uint32_t tmp = 0;
        char str[8];
        memcpy(str, target, len);

        if (!fromHex(str, 8, reinterpret_cast<unsigned char*>(&tmp)) || tmp == 0) {
            return false;
        }

        m_target = 0xFFFFFFFFFFFFFFFFULL / (0xFFFFFFFFULL / static_cast<uint64_t>(tmp));
    }
    else if (len <= 16) {
        m_target = 0;
        char str[16];
        memcpy(str, target, len);

        if (!fromHex(str, 16, reinterpret_cast<unsigned char*>(&m_target)) || m_target == 0) {
            return false;
        }
    }
    else {
        return false;
    }

#   ifdef XMRIG_PROXY_PROJECT
    memset(m_rawTarget, 0, sizeof(m_rawTarget));
    memcpy(m_rawTarget, target, len);
#   endif
    
    m_diff = m_target;
    return true;
}


bool Job::setJobId(const char *id)
{
    if(!id) {
        return false; 
    }
    const size_t jobIdLen = strlen(id);
    if(jobIdLen != LEN::NONCE_HEX){
        return false;
    }
    char jobId[LEN::NONCE_HEX+1];
    memset(jobId, 0, LEN::NONCE_HEX+1);
    memcpy(jobId, id, jobIdLen);

    if(!fromHex(jobId, jobIdLen, reinterpret_cast<unsigned char*>(&m_jobId))){
        return false;
    }
    return true;
}

bool Job::setJobUnit(const char *unit){
    if(!unit) {
        return false; 
    }

    const size_t unitLen = strlen(unit);

    char jobUnit[unitLen+1];
    memset(jobUnit, 0, unitLen+1);
    memcpy(jobUnit, unit, unitLen);

    if(!fromHex(jobUnit, unitLen, reinterpret_cast<unsigned char*>(&m_jobUnit)) || m_jobUnit == 0){
        return false;
    }
    return true;
}

xmrig::Variant Job::variant() const
{
    if (m_algorithm.algo() == xmrig::CRYPTONIGHT_HEAVY) {
        return xmrig::VARIANT_0;
    }

    if (m_algorithm.variant() == xmrig::VARIANT_XTL && m_blob[0] < 4) {
        return xmrig::VARIANT_1;
    }

    if (m_algorithm.variant() == xmrig::VARIANT_AUTO) {
        if (m_algorithm.algo() == xmrig::CRYPTONIGHT) {
            return xmrig::VARIANT_1;
        }

        return (m_blob[0] > 6 ? xmrig::VARIANT_1 : xmrig::VARIANT_0);
    }

    return m_algorithm.variant();
}


bool Job::fromHex(const char* in, unsigned int len, unsigned char* out)
{
    bool error = false;
    for (unsigned int i = 0; i < len; i += 2) {
        out[(len -i -1) / 2] = (hf_hex2bin(in[i], error) << 4) | (hf_hex2bin(in[i + 1], error) );
        if (error) {
            return false;
        }
    }
    return true;
}

void Job::toHexLittle(const unsigned char* in, unsigned int len, char* out)
{
    for (unsigned int i = 0; i < len; i++) {
        out[(len - i)*2 - 2] = hf_bin2hex((in[i] & 0xF0) >> 4);
        out[(len - i)*2 - 1] = hf_bin2hex(in[i] & 0x0F);
    }
}

void Job::toHex(const unsigned char* in, unsigned int len, char* out)
{
    for (unsigned int i = 0; i < len; i++) {
        out[i * 2] = hf_bin2hex((in[i] & 0xF0) >> 4);
        out[i * 2 + 1] = hf_bin2hex(in[i] & 0x0F);
    }
}


#ifdef APP_DEBUG
char *Job::toHex(const unsigned char* in, unsigned int len)
{
    char *out = new char[len * 2 + 1]();
    toHex(in, len, out);

    return out;
}
#endif


bool Job::operator==(const Job &other) const
{
    return m_id == other.m_id && memcmp(m_blob, other.m_blob, sizeof(m_blob)) == 0;
}


bool Job::operator!=(const Job &other) const
{
    return m_id != other.m_id || memcmp(m_blob, other.m_blob, sizeof(m_blob)) != 0;
}
