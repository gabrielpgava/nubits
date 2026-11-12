// Copyright (c) 2026 The NuBits developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef NUBITS_AUTOUPDATE_H
#define NUBITS_AUTOUPDATE_H

#include <string>
#include <vector>
#include <boost/thread.hpp>
#include "json/json_spirit_value.h"

struct UpdateInfo
{
    bool fUpdateAvailable;
    std::string strCurrentVersion;
    std::string strLatestVersion;
    std::string strReleaseName;
    std::string strReleaseNotes;
    std::string strHtmlUrl;
    std::string strDownloadUrl;
    std::string strAssetName;
    std::string strExpectedSha256;
    int64_t nSize;
    int64_t nCheckTime;

    UpdateInfo()
        : fUpdateAvailable(false),
          nSize(0),
          nCheckTime(0)
    {
    }
};

class CAutoUpdate
{
private:
    static UpdateInfo updateInfo;
    static boost::mutex cs_update;
    static bool fThreadRunning;
    static boost::thread* pUpdateThread;

public:
    static const std::string OFFICIAL_REPO;

    // Checks GitHub releases for new versions
    static bool CheckForUpdates(bool fSilent = false, std::string* pstrError = NULL);

    // Fetches dynamic seed node list (IP:port or host:port) from GitHub repository seeds.json
    static bool FetchRemoteSeeds(bool fTestNet, std::vector<std::string>& vSeedsOut, std::string* pstrError = NULL);

    // Downloads release asset, verifies SHA256, and executes the updater script
    static bool ApplyUpdate(std::string& strMessage);

    // Thread management
    static void StartUpdateThread();
    static void StopUpdateThread();

    // Query cached status
    static UpdateInfo GetUpdateInfo();

    // Utility: calculate SHA256 of local file
    static std::string ComputeFileSHA256(const std::string& strFilePath);

    // Detect asset name based on OS and architecture
    static std::string GetPlatformAssetPattern();

    // Version comparison: returns true if strRemoteVer > strCurrentVer
    static bool IsNewerVersion(const std::string& strRemoteVer, const std::string& strCurrentVer);
};

// RPC commands registered in bitcoinrpc.cpp
extern json_spirit::Value checkupdate(const json_spirit::Array& params, bool fHelp);
extern json_spirit::Value applyupdate(const json_spirit::Array& params, bool fHelp);

#endif // NUBITS_AUTOUPDATE_H
