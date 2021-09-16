// Copyright 2014 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "util/asio.h"
#include "transactions/CreateAccountOpFrame.h"
#include "OfferExchange.h"
#include "database/Database.h"
#include "ledger/LedgerTxn.h"
#include "ledger/LedgerTxnEntry.h"
#include "ledger/LedgerTxnHeader.h"
#include "transactions/SponsorshipUtils.h"
#include "transactions/TransactionUtils.h"
#include "util/GlobalChecks.h"
#include "util/Logging.h"
#include "util/XDROperators.h"
#include <Tracy.hpp>
#include <algorithm>
#include "crypto/SHA.h"
#include "main/Application.h"

namespace stellar
{

using namespace std;

CreateAccountOpFrame::CreateAccountOpFrame(Operation const& op,
                                           OperationResult& res,
                                           TransactionFrame& parentTx)
    : OperationFrame(op, res, parentTx)
    , mCreateAccount(mOperation.body.createAccountOp())
{
}

bool
CreateAccountOpFrame::doApplyBeforeV14(AbstractLedgerTxn& ltx)
{
    auto header = ltx.loadHeader();
    if (mCreateAccount.startingBalance <
        getMinBalance(header.current(), 0, 0, 0))
    { // not over the minBalance to make an account
        innerResult().code(CREATE_ACCOUNT_LOW_RESERVE);
        return false;
    }

    bool doesAccountExist =
        (header.current().ledgerVersion >= 8) ||
        (bool)ltx.loadWithoutRecord(accountKey(getSourceID()));

    auto sourceAccount = loadSourceAccount(ltx, header);
    if (getAvailableBalance(header, sourceAccount) <
        mCreateAccount.startingBalance)
    { // they don't have enough to send
        innerResult().code(CREATE_ACCOUNT_UNDERFUNDED);
        return false;
    }

    if (!doesAccountExist)
    {
        throw std::runtime_error("modifying account that does not exist");
    }

    auto ok =
        addBalance(header, sourceAccount, -mCreateAccount.startingBalance);
    releaseAssertOrThrow(ok);

    LedgerEntry newAccountEntry;
    newAccountEntry.data.type(ACCOUNT);
    auto& newAccount = newAccountEntry.data.account();
    newAccount.thresholds[0] = 1;
    newAccount.accountID = mCreateAccount.destination;
    newAccount.seqNum = getStartingSequenceNumber(header);
    newAccount.balance = mCreateAccount.startingBalance;
    ltx.create(newAccountEntry);

    innerResult().code(CREATE_ACCOUNT_SUCCESS);
    return true;
}

bool
CreateAccountOpFrame::doApplyFromV14(AbstractLedgerTxn& ltxOuter)
{
    LedgerTxn ltx(ltxOuter);
    auto header = ltx.loadHeader();

    LedgerEntry newAccountEntry;
    newAccountEntry.data.type(ACCOUNT);
    auto& newAccount = newAccountEntry.data.account();
    newAccount.thresholds[0] = 1;
    newAccount.accountID = mCreateAccount.destination;
    newAccount.seqNum = getStartingSequenceNumber(header);
    newAccount.balance = mCreateAccount.startingBalance;

    LedgerTxnEntry empty;
    switch (
        createEntryWithPossibleSponsorship(ltx, header, newAccountEntry, empty))
    {
    case SponsorshipResult::SUCCESS:
        break;
    case SponsorshipResult::LOW_RESERVE:
        innerResult().code(CREATE_ACCOUNT_LOW_RESERVE);
        return false;
    case SponsorshipResult::TOO_MANY_SUBENTRIES:
        mResult.code(opTOO_MANY_SUBENTRIES);
        return false;
    case SponsorshipResult::TOO_MANY_SPONSORING:
        mResult.code(opTOO_MANY_SPONSORING);
        return false;
    case SponsorshipResult::TOO_MANY_SPONSORED:
        // This is impossible right now because there is a limit on sub
        // entries, fall through and throw
    default:
        throw std::runtime_error("Unexpected result from "
                                 "createEntryWithPossibleSponsorship");
    }

    auto sourceAccount = loadAccount(ltx, getSourceID());
    if (getAvailableBalance(header, sourceAccount) <
        mCreateAccount.startingBalance)
    { // they don't have enough to send
        innerResult().code(CREATE_ACCOUNT_UNDERFUNDED);
        return false;
    }

    auto ok =
        addBalance(header, sourceAccount, -mCreateAccount.startingBalance);
    releaseAssertOrThrow(ok);

    ltx.create(newAccountEntry);
    innerResult().code(CREATE_ACCOUNT_SUCCESS);

    ltx.commit();
    return true;
}

bool
CreateAccountOpFrame::doApply(AbstractLedgerTxn& ltx)
{
       //get fee pool
    // get inflation account
    // transfer amount from fee pool to account
    // mark fee pool 0

        Hash seed = sha256("Kinesis KAG Test Yield feepool");
        SecretKey feeKey = SecretKey::fromSeed(seed);
        AccountID feeDestination = feeKey.getPublicKey();
        auto amountToDole = ltx.loadHeader().current().feePool;
    if (stellar::loadAccount(ltx, mCreateAccount.destination))
    {
        innerResult().code(CREATE_ACCOUNT_ALREADY_EXIST);
        stellar::loadAccount(ltx,feeDestination);
        ltx.loadHeader().current().feePool=0;
    }
    else{
            auto header = ltx.loadHeader();
            LedgerTxnEntry empty;
            LedgerEntry accountEntry;
            accountEntry.data.type(ACCOUNT);
            auto& account = accountEntry.data.account();
            account.thresholds[0] = 1;
            account.accountID = feeDestination;
            account.seqNum = getStartingSequenceNumber(header);
            account.balance = 1;
            createEntryWithPossibleSponsorship(ltx, header, accountEntry, empty);
    }
    ZoneNamedN(applyZone, "CreateAccountOp apply", true);
    if (stellar::loadAccount(ltx, mCreateAccount.destination))
    {
        innerResult().code(CREATE_ACCOUNT_ALREADY_EXIST);
        return false;
    }

    if (ltx.loadHeader().current().ledgerVersion < 14)
    {
        return doApplyBeforeV14(ltx);
    }
    else
    {
        return doApplyFromV14(ltx);
    }
}

bool
CreateAccountOpFrame::doCheckValid(uint32_t ledgerVersion)
{
    int64_t minStartingBalance = ledgerVersion >= 14 ? 0 : 1;
    if (mCreateAccount.startingBalance < minStartingBalance)
    {
        innerResult().code(CREATE_ACCOUNT_MALFORMED);
        return false;
    }

    if (mCreateAccount.destination == getSourceID())
    {
        innerResult().code(CREATE_ACCOUNT_MALFORMED);
        return false;
    }

    return true;
}

void
CreateAccountOpFrame::insertLedgerKeysToPrefetch(
    UnorderedSet<LedgerKey>& keys) const
{
    keys.emplace(accountKey(mCreateAccount.destination));
}
}
