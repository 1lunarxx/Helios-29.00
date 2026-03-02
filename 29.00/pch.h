#pragma once

#include "Memcury.h"
#include "JSON.hpp"
#include "Minhook.h"
#include "Offsets.h"

#include <windows.h>
#include <thread>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <utility>
#include <numeric> 
#include <map>
#include <mutex>
#include "Options.h"

#include "SDK/UnrealContainers.hpp"
#include "SDK/SDK/Basic.hpp"
#include "SDK/SDK/CoreUObject_structs.hpp"
#include "SDK/SDK/CoreUObject_classes.hpp"
#include "SDK/SDK/Engine_structs.hpp"
#include "SDK/SDK/Engine_classes.hpp"
#include "SDK/SDK/FortniteGame_structs.hpp"
#include "SDK/SDK/FortniteGame_classes.hpp"
#include "SDK/SDK/PlayerPawn_Athena_classes.hpp"
#include "SDK/SDK/FortniteUI_classes.hpp"
#include "SDK/SDK/FortniteUI_structs.hpp"
#include "SDK/SDK/GameplayAbilities_structs.hpp"
#include "SDK/SDK/GameplayAbilities_classes.hpp"
#include "SDK/SDK/LagerRuntime_classes.hpp"
#include "SDK/SDK/LagerRuntime_structs.hpp"
#include "SDK/SDK/FortniteAI_classes.hpp"
#include "SDK/SDK/FortniteAI_structs.hpp"
#include "SDK/SDK/AscenderCodeRuntime_classes.hpp"
#include "SDK/SDK/GAB_AthenaDBNO_classes.hpp"

using namespace SDK;
using json = nlohmann::json;

inline uint64_t ImageBase = *(uint64_t*)(__readgsqword(0x60) + 0x10);