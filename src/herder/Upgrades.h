#pragma once

// Copyright 2017 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "xdr/Stellar-ledger.h"

#include "main/Config.h"
#include "util/Timer.h"
#include <optional>
#include <stdint.h>
#include <vector>

namespace stellar
{
class AbstractLedgerTxn;
class Config;
class Database;
struct LedgerHeader;
struct LedgerUpgrade;

class Upgrades
{
  public:
    // # of hours after the scheduled upgrade time before we remove pending
    // upgrades
    static std::chrono::hours const UPDGRADE_EXPIRATION_HOURS;

    struct UpgradeParameters
    {
        UpgradeParameters()
        {
        }
        UpgradeParameters(Config const& cfg)
        {
            mUpgradeTime = cfg.TESTING_UPGRADE_DATETIME;
            mProtocolVersion = std::make_optional<uint32>(
                cfg.TESTING_UPGRADE_LEDGER_PROTOCOL_VERSION);
            mBaseFee =
                std::make_optional<uint32>(cfg.TESTING_UPGRADE_DESIRED_FEE);
            mBasePercentageFee = std::make_optional<uint32>(
                cfg.TESTING_UPGRADE_DESIRED_PERCENTAGE_FEE);
            mMaxFee =
                std::make_optional<uint64>(cfg.TESTING_UPGRADE_DESIRED_MAX_FEE);
            mMaxTxSetSize =
                std::make_optional<uint32>(cfg.TESTING_UPGRADE_MAX_TX_SET_SIZE);
            mBaseReserve =
                std::make_optional<uint32>(cfg.TESTING_UPGRADE_RESERVE);
            mFlags = std::make_optional<uint32>(cfg.TESTING_UPGRADE_FLAGS);
        }
        VirtualClock::system_time_point mUpgradeTime;
        std::optional<uint32> mProtocolVersion;
        std::optional<uint32> mBaseFee;
        std::optional<uint32> mBasePercentageFee;
        std::optional<uint64> mMaxFee;
        std::optional<uint32> mMaxTxSetSize;
        std::optional<uint32> mBaseReserve;
        std::optional<uint32> mFlags;

        std::string toJson() const;
        void fromJson(std::string const& s);
    };

    Upgrades()
    {
    }
    explicit Upgrades(UpgradeParameters const& params);

    void setParameters(UpgradeParameters const& params, Config const& cfg);

    UpgradeParameters const& getParameters() const;

    // create upgrades for given ledger
    std::vector<LedgerUpgrade>
    createUpgradesFor(LedgerHeader const& header) const;

    // apply upgrade to ledger header
    static void applyTo(LedgerUpgrade const& upgrade, AbstractLedgerTxn& ltx);

    // convert upgrade value to string
    static std::string toString(LedgerUpgrade const& upgrade);

    enum class UpgradeValidity
    {
        VALID,
        XDR_INVALID,
        INVALID
    };

    // VALID if it is safe to apply upgrade
    // XDR_INVALID if the upgrade cannot be deserialized
    // INVALID if it is unsafe to apply the upgrade for any other reason
    //
    // If the upgrade could be deserialized then lupgrade is set
    static UpgradeValidity isValidForApply(UpgradeType const& upgrade,
                                           LedgerUpgrade& lupgrade,
                                           LedgerHeader const& header,
                                           uint32_t maxLedgerVersion);

    // returns true if upgrade is a valid upgrade step
    // in which case it also sets upgradeType
    bool isValid(UpgradeType const& upgrade, LedgerUpgradeType& upgradeType,
                 bool nomination, Config const& cfg,
                 LedgerHeader const& header) const;

    // constructs a human readable string that represents
    // the pending upgrades
    std::string toString() const;

    // sets updated to true if some upgrades were removed
    UpgradeParameters
    removeUpgrades(std::vector<UpgradeType>::const_iterator beginUpdates,
                   std::vector<UpgradeType>::const_iterator endUpdates,
                   uint64_t time, bool& updated);

    static void dropAll(Database& db);

    static void storeUpgradeHistory(Database& db, uint32_t ledgerSeq,
                                    LedgerUpgrade const& upgrade,
                                    LedgerEntryChanges const& changes,
                                    int index);
    static void deleteOldEntries(Database& db, uint32_t ledgerSeq,
                                 uint32_t count);
    static void deleteNewerEntries(Database& db, uint32_t ledgerSeq);

  private:
    UpgradeParameters mParams;

    bool timeForUpgrade(uint64_t time) const;

    // returns true if upgrade is a valid upgrade step
    // in which case it also sets lupgrade
    bool isValidForNomination(LedgerUpgrade const& upgrade,
                              LedgerHeader const& header) const;

    static void applyVersionUpgrade(AbstractLedgerTxn& ltx,
                                    uint32_t newVersion);

    static void applyReserveUpgrade(AbstractLedgerTxn& ltx,
                                    uint32_t newReserve);
};
}
