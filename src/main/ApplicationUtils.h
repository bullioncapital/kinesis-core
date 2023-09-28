#pragma once

// Copyright 2018 Stellar Development Foundation and contributors. Licensed
// under the Apache License, Version 2.0. See the COPYING file at the root
// of this distribution or at http://www.apache.org/licenses/LICENSE-2.0

#include "history/HistoryArchive.h"
#include "ledger/LedgerRange.h"
#include "main/Application.h"
#include <optional>

namespace stellar
{

class CatchupConfiguration;

// Create application and validate its configuration
Application::pointer setupApp(Config& cfg, VirtualClock& clock,
                              uint32_t startAtLedger,
                              std::string const& startAtHash);
int runApp(Application::pointer app);
void setForceSCPFlag();
void initializeDatabase(Config cfg);
void httpCommand(std::string const& command, unsigned short port);
int selfCheck(Config cfg);
int mergeBucketList(Config cfg, std::string const& outputDir);
int dumpLedger(Config cfg, std::string const& outputFile,
               std::optional<std::string> filterQuery,
               std::optional<uint32_t> lastModifiedLedgerCount,
               std::optional<uint64_t> limit,
               std::optional<std::string> groupBy,
               std::optional<std::string> aggregate);
void showOfflineInfo(Config cfg, bool verbose);
void closeLedgersOffline(Config cfg, bool verbose, size_t nLedgers);
int reportLastHistoryCheckpoint(Config cfg, std::string const& outputFile);
#ifdef BUILD_TESTS
void loadXdr(Config cfg, std::string const& bucketFile);
int rebuildLedgerFromBuckets(Config cfg);
#endif
void genSeed();
int initializeHistories(Config cfg,
                        std::vector<std::string> const& newHistories);
void writeCatchupInfo(Json::Value const& catchupInfo,
                      std::string const& outputFile);
int catchup(Application::pointer app, CatchupConfiguration cc,
            Json::Value& catchupInfo, std::shared_ptr<HistoryArchive> archive);
// Reduild ledger state based on the buckets. Ensure ledger state is properly
// reset before calling this function.
bool applyBucketsForLCL(Application& app);
bool applyBucketsForLCL(Application& app,
                        std::function<bool(LedgerEntryType)> onlyApply);
int publish(Application::pointer app);
std::string minimalDBForInMemoryMode(Config const& cfg);
bool canRebuildInMemoryLedgerFromBuckets(uint32_t startAtLedger, uint32_t lcl);
void setAuthenticatedLedgerHashPair(Application::pointer app,
                                    LedgerNumHashPair& authPair,
                                    uint32_t startLedger,
                                    std::string startHash);
std::optional<uint32_t>
getStellarCoreMajorReleaseVersion(std::string const& vstr);
}
