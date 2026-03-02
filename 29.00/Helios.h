#pragma once
#pragma once
#define CURL_STATICLIB
#include "curl/curl.h"
#include "pch.h"

namespace Helios
{
    namespace Curl
    {
        inline void Init() {
            static std::once_flag flag;
            std::call_once(flag, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });
        }

        inline size_t WriteCallback(void* ptr, size_t size, size_t nmemb, void* userdata) {
            auto* ss = static_cast<std::ostringstream*>(userdata);
            ss->write(static_cast<char*>(ptr), size * nmemb);
            return size * nmemb;
        }

        std::string GetResponse(const std::string& url);
    }

    namespace Sessions
    {
         void CreateSeleniumSession();
         void DeleteSeleniumSession();
    }

    void EndBattleRoyaleGameV2(AFortPlayerControllerAthena*, int, int, int);
}