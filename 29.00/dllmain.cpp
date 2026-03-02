#include "pch.h"
#include "Misc.h"
#include "GameMode.h"
#include "Player.h"
#include "Abilities.h"
#include "Options.h"
#include "Building.h"
#include "Inventory.h"

void Main()
{
    AllocConsole();
    FILE* F;
    freopen_s(&F, "CONOUT$", "w", stdout);
    SetConsoleTitleA("Helios | Loading");
    MH_Initialize();
    Sleep(5000);

    Misc::Patch();
    GameMode::Patch();
    Player::Patch();
    Inventory::Patch();
    Abilities::Patch();
    Building::Patch();

    *(bool*)Helios::Offsets::GIsClient = false;
    *(bool*)Helios::Offsets::GIsServer = true;

    UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"log LogFortUIDirector None", nullptr);
    UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"log LogIrisBridge None", nullptr);
    UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"log LogIris None", nullptr);
    UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"log LogPhysics None", nullptr);
    UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"log LogCustomControls None", nullptr);
    UKismetSystemLibrary::ExecuteConsoleCommand(UWorld::GetWorld(), L"log LogIrisRpc None", nullptr);

    UWorld::GetWorld()->OwningGameInstance->LocalPlayers.Remove(0);
    UGameplayStatics::OpenLevel(UWorld::GetWorld(), Runtime::StaticFindObject<UFortPlaylistAthena>(UHeliosConfiguration::PlaylistID)->PreloadPersistentLevel.ObjectID.AssetPath.AssetName.IsValid() ? Runtime::StaticFindObject<UFortPlaylistAthena>(UHeliosConfiguration::PlaylistID)->PreloadPersistentLevel.ObjectID.AssetPath.AssetName : UKismetStringLibrary::Conv_StringToName(L"/Game/Athena/Helios/Maps/Helios_Terrain.Helios_Terrain"), false, FString());
}

BOOL APIENTRY DllMain(HMODULE Module, DWORD Reason, LPVOID Reserved)
{
    switch (Reason)
    {
    case 1:
        std::thread(Main).detach();
        if (UHeliosConfiguration::bIsProd)
            std::thread(Runtime::Restart).detach();
        break;
    case 0:
        break;
    }
    return 1;
}