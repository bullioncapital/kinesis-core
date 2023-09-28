#pragma once

// Copyright 2015 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include <type_traits>

#include "overlay/StellarXDR.h"
#include "util/XDROperators.h"

namespace stellar
{

template <typename T>
bool
lexCompare(T&& lhs1, T&& rhs1)
{
    return lhs1 < rhs1;
}

template <typename T, typename... U>
bool
lexCompare(T&& lhs1, T&& rhs1, U&&... args)
{
    if (lhs1 < rhs1)
    {
        return true;
    }
    else if (rhs1 < lhs1)
    {
        return false;
    }
    return lexCompare(std::forward<U>(args)...);
}

/**
 * Compare two LedgerEntries or LedgerKeys for 'identity', not content.
 *
 * LedgerEntries are identified iff they have:
 *
 *   - The same type
 *     - If accounts, then with same accountID
 *     - If trustlines, then with same (accountID, asset) pair
 *     - If offers, then with same (sellerID, sequence) pair
 *
 * Equivalently: Two LedgerEntries have the same 'identity' iff their
 * corresponding LedgerKeys are exactly equal. This operator _could_ be
 * implemented in terms of extracting 2 LedgerKeys from 2 LedgerEntries and
 * doing operator< on them, but that would be comparatively inefficient.
 */
struct LedgerEntryIdCmp
{
    template <typename T, typename U>
    auto
    operator()(T const& a, U const& b) const
        -> decltype(a.type(), b.type(), bool())
    {
        LedgerEntryType aty = a.type();
        LedgerEntryType bty = b.type();

        if (aty < bty)
            return true;

        if (aty > bty)
            return false;

        switch (aty)
        {
        case ACCOUNT:
            return a.account().accountID < b.account().accountID;
        case TRUSTLINE:
            return lexCompare(a.trustLine().accountID, b.trustLine().accountID,
                              a.trustLine().asset, b.trustLine().asset);
        case OFFER:
            return lexCompare(a.offer().sellerID, b.offer().sellerID,
                              a.offer().offerID, b.offer().offerID);
        case DATA:
            return lexCompare(a.data().accountID, b.data().accountID,
                              a.data().dataName, b.data().dataName);
        case CLAIMABLE_BALANCE:
            return a.claimableBalance().balanceID <
                   b.claimableBalance().balanceID;
        case LIQUIDITY_POOL:
            return a.liquidityPool().liquidityPoolID <
                   b.liquidityPool().liquidityPoolID;
#ifdef ENABLE_NEXT_PROTOCOL_VERSION_UNSAFE_FOR_PRODUCTION
        case CONTRACT_DATA:
            return lexCompare(a.contractData().contractID,
                              b.contractData().contractID, a.contractData().key,
                              b.contractData().key);
        case CONTRACT_CODE:
            return a.contractCode().hash < b.contractCode().hash;
        case CONFIG_SETTING:
        {
            auto getConfigSettingId = [](auto const& v) -> ConfigSettingID {
                using ConfigT = decltype(v);
                if constexpr (std::is_same_v<ConfigT, LedgerKey const&>)
                {
                    return v.configSetting().configSettingID;
                }
                else if constexpr (std::is_same_v<ConfigT,
                                                  LedgerEntry::_data_t const&>)
                {
                    return v.configSetting().configSettingID();
                }
                else
                {
                    throw std::runtime_error("Unexpected entry type");
                }
            };
            return getConfigSettingId(a) < getConfigSettingId(b);
        }
#endif
        }
        return false;
    }
};

/**
 * Compare two BucketEntries for identity by comparing their respective
 * LedgerEntries (ignoring their hashes, as the LedgerEntryIdCmp ignores their
 * bodies).
 */
struct BucketEntryIdCmp
{
    bool
    operator()(BucketEntry const& a, BucketEntry const& b) const
    {
        BucketEntryType aty = a.type();
        BucketEntryType bty = b.type();

        // METAENTRY sorts below all other entries, comes first in buckets.
        if (aty == METAENTRY || bty == METAENTRY)
        {
            return aty < bty;
        }

        if (aty == LIVEENTRY || aty == INITENTRY)
        {
            if (bty == LIVEENTRY || bty == INITENTRY)
            {
                return LedgerEntryIdCmp{}(a.liveEntry().data,
                                          b.liveEntry().data);
            }
            else
            {
                if (bty != DEADENTRY)
                {
                    throw std::runtime_error("Malformed bucket: unexpected "
                                             "non-INIT/LIVE/DEAD entry.");
                }
                return LedgerEntryIdCmp{}(a.liveEntry().data, b.deadEntry());
            }
        }
        else
        {
            if (aty != DEADENTRY)
            {
                throw std::runtime_error(
                    "Malformed bucket: unexpected non-INIT/LIVE/DEAD entry.");
            }
            if (bty == LIVEENTRY || bty == INITENTRY)
            {
                return LedgerEntryIdCmp{}(a.deadEntry(), b.liveEntry().data);
            }
            else
            {
                if (bty != DEADENTRY)
                {
                    throw std::runtime_error("Malformed bucket: unexpected "
                                             "non-INIT/LIVE/DEAD entry.");
                }
                return LedgerEntryIdCmp{}(a.deadEntry(), b.deadEntry());
            }
        }
    }
};
}
