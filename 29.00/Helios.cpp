#include "pch.h"
#include "Helios.h"

void Helios::EndBattleRoyaleGameV2(AFortPlayerControllerAthena* Controller, int TotalXP, int KillScore, int Placement)
{
    if (!Controller || !Controller->PlayerState || !UHeliosConfiguration::bIsProd)
        return;

    AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
    std::string URL = "https://api.flairfn.xyz/fortnite/api/game/v2/profile/" + Controller->PlayerState->GetPlayerName().ToString() + "/dedicated_server/EndBattleRoyaleGameV2?profileId=athena";

    json Payload;

    Payload["PlaylistId"] = GameMode->CurrentPlaylistName.ToString();
    Payload["TotalXPAccum"] = TotalXP;
    Payload["RestedXPAccum"] = 0;
    Payload["bIsTournament"] = UHeliosConfiguration::bIsTournament;
    Payload["FriendshipXpBoost"] = 0;
    Payload["MatchStats"]["KillScore"] = KillScore;
    Payload["MatchStats"]["Placement"] = Placement;

    std::thread([URL, Payload]() {
        CURL* curl = curl_easy_init();
        if (!curl)
            return;

        std::string PayloadBody = Payload.dump();

        curl_slist* Headers = nullptr;
        Headers = curl_slist_append(Headers, "Content-Type: application/json");

        curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, Headers);
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, PayloadBody.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, PayloadBody.size());

        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L); 
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

        curl_easy_setopt(
            curl,
            CURLOPT_WRITEFUNCTION,
            +[](char*, size_t s, size_t n, void*) { return s * n; }
        );

        CURLcode res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            printf(
                "[Helios] EndBattleRoyaleGameV2 failed: %s\n",
                curl_easy_strerror(res)
            );
        }

        curl_slist_free_all(Headers);
        curl_easy_cleanup(curl);
     }).detach();
}

void Helios::Sessions::CreateSeleniumSession()
{
    AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
    if (!GameMode)
        return;

    CURL* curl = curl_easy_init();
    if (!curl)
        return;

    Curl::Init();

    const std::string URL = "http://127.0.0.1:81/selenium/api/v1/servers/create";
    json Payload;

    Payload["PlaylistId"] = GameMode->CurrentPlaylistName.ToString();
    Payload["Region"] = UHeliosConfiguration::Region;
    Payload["ServerPort"] = UHeliosConfiguration::Port;
    Payload["SessionData"] = {
        { "MaxPlayers",  100 },
        { "bIsTournament", false }
    };

    const std::string PayloadStr = Payload.dump();

    struct curl_slist* Headers = nullptr;
    Headers = curl_slist_append(Headers, "Content-Type: application/json");

    std::ostringstream Response;

    curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, PayloadStr.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, PayloadStr.size());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, Headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Curl::WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &Response);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 2000L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 4000L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    CURLcode Result = curl_easy_perform(curl);

    if (Result == CURLE_OK)
    {
        long Status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &Status);
    }

    curl_slist_free_all(Headers);
    curl_easy_cleanup(curl);
}

void Helios::Sessions::DeleteSeleniumSession()
{
    AFortGameModeAthena* GameMode = (AFortGameModeAthena*)UWorld::GetWorld()->AuthorityGameMode;
    if (!GameMode)
        return;

    CURL* curl = curl_easy_init();
    if (!curl)
        return;

    Curl::Init();

    const std::string URL = "http://127.0.0.1:81/selenium/api/v1/servers/delete";
    json Payload;

    Payload["PlaylistId"] = GameMode->CurrentPlaylistName.ToString();
    Payload["Region"] = UHeliosConfiguration::Region;
    Payload["ServerPort"] = UHeliosConfiguration::Port;
    Payload["SessionData"] = {
        { "MaxPlayers", 100 },
        { "bIsTournament", false }
    };

    const std::string PayloadStr = Payload.dump();

    struct curl_slist* Headers = nullptr;
    Headers = curl_slist_append(Headers, "Content-Type: application/json");

    std::ostringstream Response;

    curl_easy_setopt(curl, CURLOPT_URL, URL.c_str());
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, PayloadStr.c_str());
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, PayloadStr.size());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, Headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, Curl::WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &Response);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 2000L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 4000L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    CURLcode Result = curl_easy_perform(curl);

    if (Result == CURLE_OK)
    {
        long Status = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &Status);
    }

    curl_slist_free_all(Headers);
    curl_easy_cleanup(curl);
}

std::string Helios::Curl::GetResponse(const std::string& url)
{
    CURL* curl = curl_easy_init();
    if (!curl)
        return {};

    std::string response;

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    return response;
}