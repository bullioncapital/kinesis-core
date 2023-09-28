#pragma once

// Copyright 2017 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "invariant/Invariant.h"
#include "main/Config.h"
#include <memory>

namespace stellar
{

class Application;
struct LedgerTxnDelta;

// This Invariant is used to validate that the total number of lumens only
// changes during inflation. The Invariant also checks that, after inflation,
// the totalCoins and feePool of the LedgerHeader matches the total balance
// in the database.
class ConservationOfLumens : public Invariant
{
  public:
#ifdef ENABLE_NEXT_PROTOCOL_VERSION_UNSAFE_FOR_PRODUCTION
    ConservationOfLumens(Hash const& lumenContractID,
                         SCVal const& balanceSymbol, SCVal const& amountSymbol);
#else
    ConservationOfLumens();
#endif

    static std::shared_ptr<Invariant> registerInvariant(Application& app);

    virtual std::string getName() const override;

    virtual std::string
    checkOnOperationApply(Operation const& operation,
                          OperationResult const& result,
                          LedgerTxnDelta const& ltxDelta) override;

  private:
#ifdef ENABLE_NEXT_PROTOCOL_VERSION_UNSAFE_FOR_PRODUCTION
    Hash const mLumenContractID;
    SCVal const mBalanceSymbol;
    SCVal const mAmountSymbol;
#endif
};
}
