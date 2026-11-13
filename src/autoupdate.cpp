// Copyright (c) 2026 The NuBits developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "autoupdate.h"
#include "version.h"
#include "util.h"
#include "net.h"
#include "init.h"
#include "bitcoinrpc.h"
#include "json/json_spirit_reader_template.h"
#include "json/json_spirit_writer_template.h"
#include "json/json_spirit_utils.h"

#include <curl/curl.h>
#include <openssl/sha.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <limits.h>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string.hpp>

#ifdef WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

const std::string CAutoUpdate::OFFICIAL_REPO = "gabrielpgava/nubits";

UpdateInfo CAutoUpdate::updateInfo;
boost::mutex CAutoUpdate::cs_update;
bool CAutoUpdate::fThreadRunning = false;
boost::thread* CAutoUpdate::pUpdateThread = NULL;

namespace
{
    boost::filesystem::path GetExecutablePath()
    {
#ifdef WIN32
        char modulePath[MAX_PATH + 1];
        DWORD length = GetModuleFileNameA(NULL, modulePath, MAX_PATH);
        if (length == 0 || length >= MAX_PATH)
            return boost::filesystem::current_path() / "nud.exe";
        return boost::filesystem::path(std::string(modulePath, length));
#elif defined(__linux__)
        char modulePath[PATH_MAX + 1];
        ssize_t length = readlink("/proc/self/exe", modulePath, PATH_MAX);
        if (length <= 0 || length >= PATH_MAX)
            return boost::filesystem::current_path() / "nud";
        modulePath[length] = '\0';
        return boost::filesystem::path(modulePath);
#else
        return boost::filesystem::current_path() / "Nu";
#endif
    }

    unsigned long GetUpdateProcessId()
    {
#ifdef WIN32
        return static_cast<unsigned long>(GetCurrentProcessId());
#else
        return static_cast<unsigned long>(getpid());
#endif
    }

    bool IsSha256(const std::string& digest)
    {
        if (digest.size() != SHA256_DIGEST_LENGTH * 2)
            return false;
        for (size_t i = 0; i < digest.size(); ++i)
        {
            if (!std::isxdigit(static_cast<unsigned char>(digest[i])))
                return false;
        }
        return true;
    }

    std::string FindSha256InManifest(const std::string& manifest, const std::string& assetName)
    {
        std::istringstream lines(manifest);
        std::string digest;
        std::string fileName;
        while (lines >> digest >> fileName)
        {
            if (!fileName.empty() && fileName[0] == '*')
                fileName.erase(0, 1);
            if (fileName == assetName)
            {
                boost::algorithm::to_lower(digest);
                return IsSha256(digest) ? digest : "";
            }
            std::string remainder;
            std::getline(lines, remainder);
        }
        return "";
    }

    void ConfigureCurlTls(CURL* curl)
    {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        static const std::string caBundle = (GetExecutablePath().parent_path() / "cacert.pem").string();
        if (boost::filesystem::exists(caBundle))
            curl_easy_setopt(curl, CURLOPT_CAINFO, caBundle.c_str());
    }

    std::string ShellQuote(const std::string& value)
    {
        std::string quoted("'");
        for (size_t i = 0; i < value.size(); ++i)
        {
            if (value[i] == '\'')
                quoted += "'\\''";
            else
                quoted += value[i];
        }
        quoted += "'";
        return quoted;
    }

    size_t WriteStringCallback(void* contents, size_t size, size_t nmemb, void* userp)
    {
        size_t total = size * nmemb;
        std::string* str = static_cast<std::string*>(userp);
        str->append(static_cast<char*>(contents), total);
        return total;
    }

    size_t WriteFileCallback(void* contents, size_t size, size_t nmemb, void* userp)
    {
        FILE* file = static_cast<FILE*>(userp);
        return fwrite(contents, size, nmemb, file);
    }

    bool HttpGet(const std::string& strUrl, std::string& strResponse, std::string& strError, long nTimeoutSeconds = 15, long* pHttpCode = NULL)
    {
        strResponse.clear();
        if (pHttpCode)
            *pHttpCode = 0;

        CURL* curl = curl_easy_init();
        if (!curl)
        {
            strError = "Failed to initialize curl";
            return false;
        }

        curl_easy_setopt(curl, CURLOPT_URL, strUrl.c_str());
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "NuBits-AutoUpdate/5.3");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, nTimeoutSeconds);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteStringCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &strResponse);
        ConfigureCurlTls(curl);

        if (fUseProxy)
        {
            curl_easy_setopt(curl, CURLOPT_PROXY, addrProxy.ToStringIP().c_str());
            curl_easy_setopt(curl, CURLOPT_PROXYPORT, addrProxy.GetPort());
            curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS4);
        }

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            strError = curl_easy_strerror(res);
            curl_easy_cleanup(curl);
            return false;
        }

        long httpCode = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
        curl_easy_cleanup(curl);

        if (pHttpCode)
            *pHttpCode = httpCode;

        if (httpCode != 200)
        {
            strError = strprintf("HTTP response code: %ld", httpCode);
            return false;
        }

        return true;
    }
}

std::string CAutoUpdate::GetPlatformAssetPattern()
{
#if defined(WIN32)
#if defined(__aarch64__) || defined(_M_ARM64)
    return "-windows-arm64-runtime.zip";
#elif defined(__x86_64__) || defined(_WIN64) || defined(_M_X64)
    return "-windows-x64-runtime.zip";
#else
    return "-windows-x86-runtime.zip";
#endif
#elif defined(__APPLE__)
    return "";
#elif defined(__linux__)
#if defined(__aarch64__)
    return "-linux-arm64-server.tar.gz";
#elif defined(__x86_64__)
    return "-linux-x64-server.tar.gz";
#else
    return "";
#endif
#else
    return "";
#endif
}

std::string CAutoUpdate::ComputeFileSHA256(const std::string& strFilePath)
{
    std::ifstream file(strFilePath.c_str(), std::ios::binary);
    if (!file.is_open())
        return "";

    SHA256_CTX sha256;
    SHA256_Init(&sha256);

    char buffer[32768];
    while (file.read(buffer, sizeof(buffer)))
    {
        SHA256_Update(&sha256, buffer, file.gcount());
    }
    if (file.gcount() > 0)
    {
        SHA256_Update(&sha256, buffer, file.gcount());
    }

    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_Final(hash, &sha256);

    std::ostringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i)
    {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return ss.str();
}

bool CAutoUpdate::IsNewerVersion(const std::string& strRemoteVer, const std::string& strCurrentVer)
{
    std::string remote = strRemoteVer;
    if (!remote.empty() && (remote[0] == 'v' || remote[0] == 'V'))
        remote = remote.substr(1);

    std::string current = strCurrentVer;
    if (!current.empty() && (current[0] == 'v' || current[0] == 'V'))
        current = current.substr(1);

    std::vector<std::string> vRemote, vCurrent;
    boost::split(vRemote, remote, boost::is_any_of(".-_"));
    boost::split(vCurrent, current, boost::is_any_of(".-_"));

    size_t maxParts = std::max(vRemote.size(), vCurrent.size());
    for (size_t i = 0; i < maxParts; ++i)
    {
        int r = (i < vRemote.size()) ? atoi(vRemote[i].c_str()) : 0;
        int c = (i < vCurrent.size()) ? atoi(vCurrent[i].c_str()) : 0;
        if (r > c)
            return true;
        if (r < c)
            return false;
    }
    return false;
}

bool CAutoUpdate::CheckForUpdates(bool fSilent, std::string* pstrError)
{
    std::string currentVersion = strprintf("%d.%d.%d", CLIENT_VERSION_MAJOR, CLIENT_VERSION_MINOR, CLIENT_VERSION_REVISION);

    std::string url = "https://api.github.com/repos/" + OFFICIAL_REPO + "/releases?per_page=20";
    std::string jsonResponse;
    std::string httpErr;
    long httpCode = 0;

    if (!HttpGet(url, jsonResponse, httpErr, 20L, &httpCode))
    {
        if (httpCode != 0)
        {
            std::string err = strprintf("GitHub API HTTP response code: %ld", httpCode);
            if (pstrError)
                *pstrError = err;
            if (!fSilent)
                printf("AutoUpdate: %s\n", err.c_str());
        }
        else
        {
            if (pstrError)
                *pstrError = "Update check failed: " + httpErr;
            if (!fSilent)
                printf("AutoUpdate: HTTP check error: %s\n", httpErr.c_str());
        }
        return false;
    }

    try
    {
        json_spirit::Value parsed;
        if (!json_spirit::read_string(jsonResponse, parsed))
        {
            if (pstrError)
                *pstrError = "Failed to parse release JSON";
            return false;
        }

        if (parsed.type() != json_spirit::array_type)
        {
            if (pstrError)
                *pstrError = "GitHub releases response is not an array";
            return false;
        }

        const json_spirit::Array& releases = parsed.get_array();
        const json_spirit::Object* latestRelease = NULL;
        std::string tag_name;
        bool latestIsPrerelease = true;
        for (size_t i = 0; i < releases.size(); ++i)
        {
            if (releases[i].type() != json_spirit::obj_type)
                continue;
            const json_spirit::Object& release = releases[i].get_obj();
            json_spirit::Value draftValue = json_spirit::find_value(release, "draft");
            if (draftValue.type() == json_spirit::bool_type && draftValue.get_bool())
                continue;

            json_spirit::Value tagValue = json_spirit::find_value(release, "tag_name");
            if (tagValue.type() != json_spirit::str_type)
                continue;
            std::string candidateTag = tagValue.get_str();
            json_spirit::Value prereleaseValue = json_spirit::find_value(release, "prerelease");
            bool candidateIsPrerelease = prereleaseValue.type() == json_spirit::bool_type && prereleaseValue.get_bool();

            if (!latestRelease || IsNewerVersion(candidateTag, tag_name) ||
                (!IsNewerVersion(tag_name, candidateTag) && latestIsPrerelease && !candidateIsPrerelease))
            {
                latestRelease = &release;
                tag_name = candidateTag;
                latestIsPrerelease = candidateIsPrerelease;
            }
        }

        if (!latestRelease)
        {
            if (pstrError)
                *pstrError = "No published GitHub release found";
            return false;
        }

        const json_spirit::Object& obj = *latestRelease;
        std::string name = json_spirit::find_value(obj, "name").get_str();
        std::string html_url = json_spirit::find_value(obj, "html_url").get_str();
        std::string body;
        json_spirit::Value bodyVal = json_spirit::find_value(obj, "body");
        if (bodyVal.type() == json_spirit::str_type)
            body = bodyVal.get_str();

        std::string platformPattern = boost::algorithm::to_lower_copy(GetPlatformAssetPattern());
        std::string downloadUrl = "";
        std::string assetName = "";
        std::string expectedSha256 = "";
        std::string checksumsUrl = "";
        int64_t assetSize = 0;

        json_spirit::Value assetsVal = json_spirit::find_value(obj, "assets");
        if (assetsVal.type() == json_spirit::array_type)
        {
            const json_spirit::Array& assets = assetsVal.get_array();
            for (size_t i = 0; i < assets.size(); ++i)
            {
                const json_spirit::Object& asset = assets[i].get_obj();
                std::string aName = json_spirit::find_value(asset, "name").get_str();
                std::string aLower = boost::algorithm::to_lower_copy(aName);

                if (aLower == "sha256sums" || aLower == "sha256sums.txt")
                {
                    json_spirit::Value checksumsValue = json_spirit::find_value(asset, "browser_download_url");
                    if (checksumsValue.type() == json_spirit::str_type)
                        checksumsUrl = checksumsValue.get_str();
                    continue;
                }

                if (!platformPattern.empty() && aLower.size() >= platformPattern.size() &&
                    aLower.compare(aLower.size() - platformPattern.size(), platformPattern.size(), platformPattern) == 0 &&
                    aName.find('/') == std::string::npos && aName.find('\\') == std::string::npos)
                {
                    assetName = aName;
                    json_spirit::Value downloadValue = json_spirit::find_value(asset, "browser_download_url");
                    if (downloadValue.type() != json_spirit::str_type)
                    {
                        assetName.clear();
                        continue;
                    }
                    downloadUrl = downloadValue.get_str();
                    json_spirit::Value sz = json_spirit::find_value(asset, "size");
                    if (sz.type() == json_spirit::int_type)
                        assetSize = sz.get_int64();

                    json_spirit::Value digestValue = json_spirit::find_value(asset, "digest");
                    if (digestValue.type() == json_spirit::str_type)
                    {
                        expectedSha256 = digestValue.get_str();
                        if (boost::algorithm::istarts_with(expectedSha256, "sha256:"))
                            expectedSha256 = expectedSha256.substr(7);
                        boost::algorithm::to_lower(expectedSha256);
                    }
                    if (IsSha256(expectedSha256))
                        break;
                }
            }
        }

        if (!assetName.empty() && !IsSha256(expectedSha256) && !checksumsUrl.empty())
        {
            std::string checksumsText;
            std::string checksumsError;
            if (HttpGet(checksumsUrl, checksumsText, checksumsError, 20L))
                expectedSha256 = FindSha256InManifest(checksumsText, assetName);
        }

        bool newer = IsNewerVersion(tag_name, currentVersion);

        {
            boost::mutex::scoped_lock lock(cs_update);
            updateInfo.fUpdateAvailable = newer;
            updateInfo.strCurrentVersion = currentVersion;
            updateInfo.strLatestVersion = tag_name;
            updateInfo.strReleaseName = name;
            updateInfo.strReleaseNotes = body;
            updateInfo.strHtmlUrl = html_url;
            updateInfo.strDownloadUrl = downloadUrl;
            updateInfo.strAssetName = assetName;
            updateInfo.strExpectedSha256 = expectedSha256;
            updateInfo.nSize = assetSize;
            updateInfo.nCheckTime = GetTime();
        }

        if (newer)
        {
            printf("\n");
            printf("*****************************************************************\n");
            printf("* ATUALIZACAO DISPONIVEL PARA O NUBITS: %s\n", tag_name.c_str());
            printf("* Versao instalada: v%s  ->  Nova versao: %s\n", currentVersion.c_str(), tag_name.c_str());
            printf("* URL: %s\n", html_url.c_str());
            printf("* Para atualizar via CLI: execute o comando 'nud applyupdate'\n");
            printf("*****************************************************************\n\n");
        }
        else if (!fSilent)
        {
            printf("AutoUpdate: O sistema esta atualizado (v%s).\n", currentVersion.c_str());
        }

        return true;
    }
    catch (std::exception& e)
    {
        if (pstrError)
            *pstrError = std::string("Error parsing update info: ") + e.what();
        return false;
    }
}

bool CAutoUpdate::FetchRemoteSeeds(bool fTestNet, std::vector<std::string>& vSeedsOut, std::string* pstrError)
{
    vSeedsOut.clear();

    std::vector<std::string> vUrls;
    std::string strCustomUrl = GetArg("-seedsurl", "");
    if (!strCustomUrl.empty())
    {
        vUrls.push_back(strCustomUrl);
    }
    else
    {
        vUrls.push_back("https://raw.githubusercontent.com/" + OFFICIAL_REPO + "/main/seeds.json");
        vUrls.push_back("https://raw.githubusercontent.com/" + OFFICIAL_REPO + "/master/seeds.json");
    }

    const char* pszNetKey = fTestNet ? "testnet" : "mainnet";
    std::string strLastError;

    for (size_t i = 0; i < vUrls.size(); ++i)
    {
        const std::string& strUrl = vUrls[i];
        std::string jsonResponse;
        std::string httpErr;

        if (!HttpGet(strUrl, jsonResponse, httpErr, 10L))
        {
            strLastError = strprintf("Failed to fetch %s: %s", strUrl.c_str(), httpErr.c_str());
            continue;
        }

        try
        {
            json_spirit::Value parsed;
            if (!json_spirit::read_string(jsonResponse, parsed) || parsed.type() != json_spirit::obj_type)
            {
                strLastError = strprintf("Invalid JSON object from %s", strUrl.c_str());
                continue;
            }

            const json_spirit::Object& obj = parsed.get_obj();
            json_spirit::Value seedsVal = json_spirit::find_value(obj, pszNetKey);
            if (seedsVal.type() != json_spirit::array_type)
            {
                strLastError = strprintf("Missing or invalid '%s' array in %s", pszNetKey, strUrl.c_str());
                continue;
            }

            const json_spirit::Array& arr = seedsVal.get_array();
            std::vector<std::string> vParsedSeeds;
            for (size_t j = 0; j < arr.size(); ++j)
            {
                if (arr[j].type() == json_spirit::str_type)
                {
                    std::string strSeed = arr[j].get_str();
                    boost::algorithm::trim(strSeed);
                    if (!strSeed.empty())
                        vParsedSeeds.push_back(strSeed);
                }
            }

            vSeedsOut.swap(vParsedSeeds);
            printf("GitHub dynamic seeds: loaded %d node(s) from %s\n", (int)vSeedsOut.size(), strUrl.c_str());
            return true;
        }
        catch (const std::exception& e)
        {
            strLastError = strprintf("Error parsing seeds JSON from %s: %s", strUrl.c_str(), e.what());
        }
    }

    if (pstrError)
        *pstrError = strLastError;
    return false;
}

bool CAutoUpdate::ApplyUpdate(std::string& strMessage)
{
    UpdateInfo currentInfo = GetUpdateInfo();
    if (!currentInfo.fUpdateAvailable && currentInfo.strDownloadUrl.empty())
    {
        // Try checking first
        std::string err;
        if (!CheckForUpdates(false, &err))
        {
            strMessage = "Check for updates failed: " + err;
            return false;
        }
        currentInfo = GetUpdateInfo();
    }

    if (!currentInfo.fUpdateAvailable)
    {
        strMessage = "No newer version available to apply. Currently running v" + currentInfo.strCurrentVersion;
        return false;
    }

    if (currentInfo.strDownloadUrl.empty())
    {
        strMessage = "No compatible, SHA-256 verified runtime package found for platform (" + GetPlatformAssetPattern() + ") in release " + currentInfo.strLatestVersion;
        return false;
    }

    if (!IsSha256(currentInfo.strExpectedSha256))
    {
        strMessage = "Release asset does not provide a valid SHA-256 digest; refusing to install it";
        return false;
    }

    if (currentInfo.strAssetName.empty() ||
        currentInfo.strAssetName.find('/') != std::string::npos ||
        currentInfo.strAssetName.find('\\') != std::string::npos)
    {
        strMessage = "Release asset has an invalid filename";
        return false;
    }

    boost::filesystem::path updateDir = GetDataDir() / "updates";
    boost::filesystem::create_directories(updateDir);

    boost::filesystem::path targetFile = updateDir / currentInfo.strAssetName;

    printf("AutoUpdate: Downloading %s from %s...\n", currentInfo.strAssetName.c_str(), currentInfo.strDownloadUrl.c_str());

    FILE* fp = fopen(targetFile.string().c_str(), "wb");
    if (!fp)
    {
        strMessage = "Failed to open destination file for writing: " + targetFile.string();
        return false;
    }

    CURL* curl = curl_easy_init();
    if (!curl)
    {
        fclose(fp);
        strMessage = "Failed to initialize curl for downloading asset";
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, currentInfo.strDownloadUrl.c_str());
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "NuBits-AutoUpdate/5.3");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFileCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);
    ConfigureCurlTls(curl);

    if (fUseProxy)
    {
        curl_easy_setopt(curl, CURLOPT_PROXY, addrProxy.ToStringIP().c_str());
        curl_easy_setopt(curl, CURLOPT_PROXYPORT, addrProxy.GetPort());
        curl_easy_setopt(curl, CURLOPT_PROXYTYPE, CURLPROXY_SOCKS4);
    }

    CURLcode res = curl_easy_perform(curl);
    long httpCode = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);
    fclose(fp);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || httpCode != 200)
    {
        boost::filesystem::remove(targetFile);
        strMessage = res != CURLE_OK
            ? strprintf("Download error: %s", curl_easy_strerror(res))
            : strprintf("Download failed with HTTP response code %ld", httpCode);
        return false;
    }

    std::string computedHash = ComputeFileSHA256(targetFile.string());
    printf("AutoUpdate: Download complete (%s). SHA256: %s\n", targetFile.string().c_str(), computedHash.c_str());
    boost::algorithm::to_lower(computedHash);
    if (computedHash != currentInfo.strExpectedSha256)
    {
        boost::filesystem::remove(targetFile);
        strMessage = "Downloaded update failed SHA-256 verification; refusing to install it";
        return false;
    }

    if (currentInfo.nSize > 0 && static_cast<int64_t>(boost::filesystem::file_size(targetFile)) != currentInfo.nSize)
    {
        boost::filesystem::remove(targetFile);
        strMessage = "Downloaded update size does not match the GitHub release metadata";
        return false;
    }

    // Prepare updater execution
    boost::filesystem::path executablePath = GetExecutablePath();
    boost::filesystem::path applicationDirectory = executablePath.parent_path();
#if defined(WIN32)
    boost::filesystem::path scriptPath = applicationDirectory / "update_windows.bat";
    if (!boost::filesystem::exists(scriptPath))
    {
        boost::filesystem::remove(targetFile);
        strMessage = "Windows updater script is missing next to the application";
        return false;
    }

    const std::string exeName = boost::algorithm::to_lower_copy(executablePath.filename().string());
    const char* target = exeName == "nu.exe" ? "gui" : "daemon";
    std::string cmd = strprintf("cmd.exe /d /c start \"\" /min \"%s\" \"%s\" %lu %s",
                                scriptPath.string().c_str(), targetFile.string().c_str(), GetUpdateProcessId(), target);
    printf("AutoUpdate: Launching updater script: %s\n", cmd.c_str());
    if (system(cmd.c_str()) != 0)
    {
        boost::filesystem::remove(targetFile);
        strMessage = "Failed to launch the Windows updater script";
        return false;
    }
#else
    boost::filesystem::path scriptPath = applicationDirectory / "update_posix.sh";
    if (!boost::filesystem::exists(scriptPath))
    {
        boost::filesystem::remove(targetFile);
        strMessage = "POSIX updater script is missing next to the application";
        return false;
    }

    std::ostringstream command;
    command << "/bin/bash " << ShellQuote(scriptPath.string()) << " "
            << ShellQuote(targetFile.string()) << " " << GetUpdateProcessId()
            << " >/dev/null 2>&1 &";
    const std::string cmd = command.str();
    printf("AutoUpdate: Launching updater script: %s\n", cmd.c_str());
    if (system(cmd.c_str()) != 0)
    {
        boost::filesystem::remove(targetFile);
        strMessage = "Failed to launch the POSIX updater script";
        return false;
    }
#endif

    strMessage = "Update downloaded successfully (" + currentInfo.strLatestVersion + "). Initiating restart...";
    printf("%s\n", strMessage.c_str());

    // Cleanly trigger shutdown
    StartShutdown();
    return true;
}

UpdateInfo CAutoUpdate::GetUpdateInfo()
{
    boost::mutex::scoped_lock lock(cs_update);
    return updateInfo;
}

void AutoUpdateThreadProc()
{
    printf("AutoUpdate thread started.\n");

    // Wait 30 seconds after startup before checking
    for (int i = 0; i < 30; ++i)
    {
        Sleep(1000);
        if (fShutdown)
            return;
    }

    while (!fShutdown)
    {
        printf("AutoUpdate: Checking for updates from %s...\n", CAutoUpdate::OFFICIAL_REPO.c_str());
        std::string error;
        if (CAutoUpdate::CheckForUpdates(false, &error))
        {
            UpdateInfo info = CAutoUpdate::GetUpdateInfo();
            if (info.fUpdateAvailable)
            {
                if (GetBoolArg("-autoupdate", false))
                {
                    printf("AutoUpdate: Unattended node (-autoupdate=1) detected. Applying update automatically...\n");
                    std::string msg;
                    CAutoUpdate::ApplyUpdate(msg);
                    return;
                }
            }
        }

        // Sleep for 12 hours between periodic checks
        for (int i = 0; i < 12 * 3600; ++i)
        {
            Sleep(1000);
            if (fShutdown)
                return;
        }
    }
}

void CAutoUpdate::StartUpdateThread()
{
    if (fThreadRunning)
        return;

    fThreadRunning = true;
    pUpdateThread = new boost::thread(AutoUpdateThreadProc);
}

void CAutoUpdate::StopUpdateThread()
{
    if (!fThreadRunning)
        return;

    fThreadRunning = false;
    if (pUpdateThread)
    {
        pUpdateThread->interrupt();
        delete pUpdateThread;
        pUpdateThread = NULL;
    }
}

//
// RPC Commands
//

json_spirit::Value checkupdate(const json_spirit::Array& params, bool fHelp)
{
    if (fHelp || params.size() > 0)
        throw std::runtime_error(
            "checkupdate\n"
            "Checks GitHub repository (gabrielpgava/nubits) for new releases.\n"
            "Returns details on whether an update is available.");

    std::string err;
    bool success = CAutoUpdate::CheckForUpdates(false, &err);
    if (!success)
        throw std::runtime_error("Update check failed: " + err);

    UpdateInfo info = CAutoUpdate::GetUpdateInfo();

    json_spirit::Object obj;
    obj.push_back(json_spirit::Pair("update_available", info.fUpdateAvailable));
    obj.push_back(json_spirit::Pair("current_version", info.strCurrentVersion));
    obj.push_back(json_spirit::Pair("latest_version", info.strLatestVersion));
    obj.push_back(json_spirit::Pair("release_name", info.strReleaseName));
    obj.push_back(json_spirit::Pair("release_url", info.strHtmlUrl));
    obj.push_back(json_spirit::Pair("download_url", info.strDownloadUrl));
    obj.push_back(json_spirit::Pair("asset_name", info.strAssetName));
    obj.push_back(json_spirit::Pair("sha256", info.strExpectedSha256));
    obj.push_back(json_spirit::Pair("asset_size_bytes", info.nSize));

    return obj;
}

json_spirit::Value applyupdate(const json_spirit::Array& params, bool fHelp)
{
    if (fHelp || params.size() > 0)
        throw std::runtime_error(
            "applyupdate\n"
            "Downloads the latest compatible release asset, validates integrity,\n"
            "launches the updater replacement script and restarts NuBits.");

    std::string message;
    bool success = CAutoUpdate::ApplyUpdate(message);

    json_spirit::Object obj;
    obj.push_back(json_spirit::Pair("success", success));
    obj.push_back(json_spirit::Pair("message", message));

    return obj;
}
