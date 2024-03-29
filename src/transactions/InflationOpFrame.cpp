// Copyright 2014 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "transactions/InflationOpFrame.h"
#include "crypto/SHA.h"
#include "ledger/LedgerManager.h"
#include "ledger/LedgerTxn.h"
#include "ledger/LedgerTxnEntry.h"
#include "ledger/LedgerTxnHeader.h"
#include "main/Application.h"
#include "overlay/StellarXDR.h"
#include "transactions/TransactionUtils.h"
#include "util/ProtocolVersion.h"

const uint32_t INFLATION_FREQUENCY = (60 * 60 * 24 * 7); // every 7 days
// inflation is .000190721 per 7 days, or 1% a year
const int64_t INFLATION_RATE_TRILLIONTHS = 190721000LL;
const int64_t TRILLION = 1000000000000LL;
const int64_t INFLATION_WIN_MIN_PERCENT = 500000000LL; // .05%
const int INFLATION_NUM_WINNERS = 2000;
const time_t INFLATION_START_TIME = (1404172800LL); // 1-jul-2014 (unix epoch)

namespace stellar
{
InflationOpFrame::InflationOpFrame(Operation const& op, OperationResult& res,
                                   TransactionFrame& parentTx)
    : OperationFrame(op, res, parentTx)
{
}

#ifdef _KINESIS

bool
InflationOpFrame::doApply(Application& app, AbstractLedgerTxn& ltx)
{
    auto header = ltx.loadHeader();
    auto& lh = header.current();
    time_t closeTime = lh.scpValue.closeTime;
    uint64_t seq = lh.inflationSeq;

    time_t inflationTime = (INFLATION_START_TIME + seq * INFLATION_FREQUENCY);
    if (closeTime < inflationTime)
    {
        innerResult().code(INFLATION_NOT_TIME);
        return false;
    }

    auto amountToDole = lh.feePool;
    lh.feePool = 0;
    lh.inflationSeq++;

    // now credit each account
    innerResult().code(INFLATION_SUCCESS);
    auto& payouts = innerResult().payouts();

    Hash seed = sha256(app.getConfig().NETWORK_PASSPHRASE + "feepool");
    SecretKey feeKey = SecretKey::fromSeed(seed);
    AccountID feeDestination = feeKey.getPublicKey();

    int64 toDoleThisWinner = amountToDole;
    int64 leftAfterDole = amountToDole;
    auto winner = stellar::loadAccount(ltx, feeDestination);
    if (winner) {
        leftAfterDole -= toDoleThisWinner;
        addBalance(header, winner, toDoleThisWinner);
        payouts.emplace_back(feeDestination, toDoleThisWinner);
    }

    // put back in fee pool as unclaimed funds
    lh.feePool += leftAfterDole;

    return true;
}

#endif

bool
InflationOpFrame::doApply(AbstractLedgerTxn& ltx)
{
    auto header = ltx.loadHeader();
    auto& lh = header.current();
    time_t closeTime = lh.scpValue.closeTime;
    uint64_t seq = lh.inflationSeq;

    time_t inflationTime = (INFLATION_START_TIME + seq * INFLATION_FREQUENCY);
    if (closeTime < inflationTime)
    {
        innerResult().code(INFLATION_NOT_TIME);
        return false;
    }

    /*
    Inflation is calculated using the following

    1. calculate tally of votes based on "inflationDest" set on each account
    2. take the top accounts (by vote) that get at least .05% of the vote
    3. If no accounts are over this threshold then the extra goes back to the
       inflation pool
    */

    int64_t totalVotes = lh.totalCoins;
    int64_t minBalance = bigDivideOrThrow(totalVotes, INFLATION_WIN_MIN_PERCENT,
                                          TRILLION, ROUND_DOWN);

    auto winners = ltx.queryInflationWinners(INFLATION_NUM_WINNERS, minBalance);

    auto inflationAmount = bigDivideOrThrow(
        lh.totalCoins, INFLATION_RATE_TRILLIONTHS, TRILLION, ROUND_DOWN);
    auto amountToDole = inflationAmount + lh.feePool;

    lh.feePool = 0;
    lh.inflationSeq++;

    // now credit each account
    innerResult().code(INFLATION_SUCCESS);
    auto& payouts = innerResult().payouts();

    int64 leftAfterDole = amountToDole;

    for (auto const& w : winners)
    {
        int64_t toDoleThisWinner =
            bigDivideOrThrow(amountToDole, w.votes, totalVotes, ROUND_DOWN);
        if (toDoleThisWinner == 0)
            continue;

        if (protocolVersionStartsFrom(lh.ledgerVersion, ProtocolVersion::V_10))
        {
            auto winner = stellar::loadAccountWithoutRecord(ltx, w.accountID);
            if (winner)
            {
                toDoleThisWinner = std::min(getMaxAmountReceive(header, winner),
                                            toDoleThisWinner);
                if (toDoleThisWinner == 0)
                    continue;
            }
        }

        auto winner = stellar::loadAccount(ltx, w.accountID);
        if (winner)
        {
            leftAfterDole -= toDoleThisWinner;
            if (protocolVersionIsBefore(lh.ledgerVersion, ProtocolVersion::V_8))
            {
                lh.totalCoins += toDoleThisWinner;
            }
            if (!addBalance(header, winner, toDoleThisWinner))
            {
                throw std::runtime_error(
                    "inflation overflowed destination balance");
            }
            payouts.emplace_back(w.accountID, toDoleThisWinner);
        }
    }

    // put back in fee pool as unclaimed funds
    lh.feePool += leftAfterDole;
    if (protocolVersionStartsFrom(lh.ledgerVersion, ProtocolVersion::V_8))
    {
        lh.totalCoins += inflationAmount;
    }

    return true;
}



bool
InflationOpFrame::doCheckValid(uint32_t ledgerVersion)
{
    return true;
}

#ifdef _KINESIS

// kinesis implementation
bool
InflationOpFrame::isOpSupported(LedgerHeader const& header) const
{
    return true;
}

#else

// original function implementation
bool
InflationOpFrame::isOpSupported(LedgerHeader const& header) const
{
    return protocolVersionIsBefore(header.ledgerVersion, ProtocolVersion::V_12);
}

#endif

ThresholdLevel
InflationOpFrame::getThresholdLevel() const
{
    return ThresholdLevel::LOW;
}
}
