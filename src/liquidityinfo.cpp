// Copyright (c) 2014-2015 The Nu developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <boost/algorithm/string.hpp>

#include "liquidityinfo.h"
#include "main.h"
#include "ui_interface.h"

#define LIQUIDITY_INFO_MAX_HEIGHT 260000 // Liquidity info from custodians who were elected more than LIQUIDITY_INFO_MAX_HEIGHT blocks ago are ignored

#ifdef TESTING
#define MAX_LIQUIDITY_INFO_PER_CUSTODIAN 5
#else
#define MAX_LIQUIDITY_INFO_PER_CUSTODIAN 100
#endif

using namespace std;

map<const CLiquiditySource, CLiquidityInfo> mapLiquidityInfo;
CCriticalSection cs_mapLiquidityInfo;
int64 nLastLiquidityUpdate = 0;

void RemoveExtraLiquidityInfo(const CCustodianAddress& custodianAddress, unsigned char cUnit)
{
    LOCK(cs_mapLiquidityInfo);

    int nCount = 0;
    map<int64, map<const CLiquiditySource, CLiquidityInfo>::iterator> mapLiquidityInfoByTime;

    map<const CLiquiditySource, CLiquidityInfo>::iterator it = mapLiquidityInfo.begin();
    while (it != mapLiquidityInfo.end())
    {
        const CLiquidityInfo& info = it->second;
        const CCustodianAddress& infoAddress = info.GetCustodianAddress();

        if (infoAddress == custodianAddress && cUnit == info.cUnit)
        {
            nCount++;
            mapLiquidityInfoByTime[info.nTime] = it;
        }
        it++;
    }

    if (nCount && nCount > MAX_LIQUIDITY_INFO_PER_CUSTODIAN)
    {
        const map<const CLiquiditySource, CLiquidityInfo>::iterator& oldestit = mapLiquidityInfoByTime.begin()->second;
        const CLiquidityInfo& oldestInfo = oldestit->second;
        printf("Custodian %s has too many liquidity info. Removing the oldest one: %s\n", custodianAddress.GetAddress().ToString().c_str(), oldestInfo.ToString().c_str());
        mapLiquidityInfo.erase(oldestit);
    }
}

bool CLiquidityInfo::ProcessLiquidityInfo()
{
    CCustodianAddress address(GetCustodianAddress());

    {
        LOCK(cs_main);

        if (pindexBest->nProtocolVersion >= PROTOCOL_V2_0 && nVersion > pindexBest->nProtocolVersion)
        {
            if (fDebug)
                printf("Liquidity info version is above protocol version\n");
            return false;
        }

        map<CCustodianAddress, CBlockIndex*> mapElectedCustodian = pindexBest->GetElectedCustodians();
        map<CCustodianAddress, CBlockIndex*>::iterator it;
        it = mapElectedCustodian.find(address);
        if (it == mapElectedCustodian.end())
        {
            if (fDebug)
                printf("Custodian not found in elected ones\n");
            return false;
        }

        CBlockIndex *pindex = it->second;
        if (!pindexBest || pindexBest->nHeight - pindex->nHeight > LIQUIDITY_INFO_MAX_HEIGHT)
        {
            if (fDebug)
                printf("Custodian is too old\n");
            return false;
        }
    }

    if (!CheckSignature())
    {
        if (fDebug)
            printf("Invalid liquidity info signature\n");
        return false;
    }

    if (!IsValid())
    {
        if (fDebug)
            printf("Liquidity info is invalid\n");
        return false;
    }

    CLiquiditySource source(address, cUnit, sIdentifier);

    {
        LOCK(cs_mapLiquidityInfo);

        map<const CLiquiditySource, CLiquidityInfo>::iterator it;
        it = mapLiquidityInfo.find(source);
        if (it != mapLiquidityInfo.end())
        {
            if (it->second.nTime >= nTime)
            {
                if (fDebug)
                    printf("Previous liquidity info has the same time or is more recent\n");
                return false;
            }
        }

        mapLiquidityInfo[source] = *this;
        nLastLiquidityUpdate = GetAdjustedTime();
    }

    printf("accepted liquidity info from %s\n", address.GetAddress().ToString().c_str());

    RemoveExtraLiquidityInfo(address, cUnit);

    MainFrameRepaint();
    return true;
}


bool CUnsignedLiquidityInfo::IsValid() const
{
    if (sIdentifier.length() > 255)
        return false;

    for (std::string::size_type i=0; i<sIdentifier.length(); ++i)
    {
        const unsigned char& chr = sIdentifier[i];
        if (chr < 0x20 || chr > 0x7e) // only printable ASCII is allowed
            return false;
    }

    return true;
}

string CUnsignedLiquidityInfo::GetTier() const
{
    vector<string> items;
    boost::split(items, sIdentifier, boost::is_any_of(":"));

    if (items.size() > 0)
    {
        const string& tierString = items[0];
        if (!tierString.empty() && tierString.find_first_not_of("0123456789") == std::string::npos)
            return tierString;
    }
    return "";
}

void RemoveExpiredLiquidityInfo(int nCurrentHeight)
{
    bool fAnyRemoved = false;
    {
        LOCK2(cs_main, cs_mapLiquidityInfo);

        map<const CLiquiditySource, CLiquidityInfo>::iterator it;
        map<CCustodianAddress, CBlockIndex*> mapElectedCustodian = pindexBest->GetElectedCustodians();
        it = mapLiquidityInfo.begin();
        while (it != mapLiquidityInfo.end())
        {
            const CCustodianAddress& address = it->first.custodianAddress;
            CBlockIndex *pindex = NULL;

            map<CCustodianAddress, CBlockIndex*>::const_iterator electedIt = mapElectedCustodian.find(address);
            if (electedIt != mapElectedCustodian.end())
                pindex = electedIt->second;

            if (!pindex || nCurrentHeight - pindex->nHeight > LIQUIDITY_INFO_MAX_HEIGHT)
            {
                mapLiquidityInfo.erase(it++);
                fAnyRemoved = true;
                nLastLiquidityUpdate = GetAdjustedTime();
            }
            else
                it++;
        }
    }

    if (fAnyRemoved)
        MainFrameRepaint();
}
