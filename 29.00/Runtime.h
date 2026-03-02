#pragma once
#include "pch.h"

enum class EHook
{
    Hook,
    Virtual,
    Null,
    RetTrue,
};

namespace Runtime {
    inline void* nullptrForHook = nullptr;

    template<typename C = void>
    inline void Hook(EHook type, uintptr_t offset, void* detour = nullptr, void** og = nullptr)
    {
        switch (type)
        {
        case EHook::Hook:
        {
            if (!offset || !detour)
                return;

            MH_CreateHook((LPVOID)offset, detour, og);
            MH_EnableHook((LPVOID)offset);
            break;
        }
        case EHook::Null:
        {
            if (!offset)
                return;

            DWORD old;
            unsigned char v = 0xC3;

            VirtualProtect((LPVOID)offset, 1, PAGE_EXECUTE_READWRITE, &old);
            *reinterpret_cast<unsigned char*>(offset) = v;
            VirtualProtect((LPVOID)offset, 1, old, &old);

            break;
        }
        case EHook::RetTrue:
        {
            DWORD oldProtect;
            VirtualProtect((LPVOID)offset, sizeof(uint32_t), PAGE_EXECUTE_READWRITE, &oldProtect);
            *(uint32_t*)offset = 0xC0FFC031;
            VirtualProtect((LPVOID)offset, sizeof(uint32_t), oldProtect, &oldProtect);

            VirtualProtect((LPVOID)(offset + 4), sizeof(uint8_t), PAGE_EXECUTE_READWRITE, &oldProtect);
            *(uint8_t*)(offset + 4) = 0xC3;
            VirtualProtect((LPVOID)(offset + 4), sizeof(uint8_t), oldProtect, &oldProtect);

            break;
        }
        case EHook::Virtual:
        {
            if constexpr (!std::is_same_v<C, void>)
            {
                auto defaultObj = C::GetDefaultObj();
                if (!defaultObj)
                    return;

                auto vft = *reinterpret_cast<void***>(defaultObj);
                if (!vft)
                    return;

                uint32_t idx = static_cast<uint32_t>(offset);
                DWORD old;

                VirtualProtect(&vft[idx], sizeof(void*), PAGE_EXECUTE_READWRITE, &old);
                if (og)
                    *og = vft[idx];

                vft[idx] = detour;
                VirtualProtect(&vft[idx], sizeof(void*), old, &old);
            }
            break;
        }
        }
    }

    template <typename _At = AActor>
    static TArray<AActor*> GetAll()
    {
        TArray<AActor*> OutActors;
        UGameplayStatics::GetAllActorsOfClass(UWorld::GetWorld(), _At::StaticClass(), &OutActors);
        return OutActors;
    }

    template <typename _Is>
    __forceinline void Patch(uintptr_t ptr, _Is byte)
    {
        DWORD og;
        VirtualProtect(LPVOID(ptr), sizeof(_Is), PAGE_EXECUTE_READWRITE, &og);
        *(_Is*)ptr = byte;
        VirtualProtect(LPVOID(ptr), sizeof(_Is), og, &og);
    }

    template<typename T>
    T* GetInterface(UObject* Object) {
        return(T*)(((void* (*)(SDK::UObject*, SDK::UClass*))Helios::Offsets::GetInterface)(Object, T::StaticClass()));
    }

    template <typename T>
    T* StaticFindObject(const std::string& ObjectPath, SDK::UClass* Class = nullptr)
    {
        return reinterpret_cast<T*>(((SDK::UObject * (*)(SDK::UClass*, SDK::UObject*, const wchar_t*, bool)) Helios::Offsets::StaticFindObject)(Class, nullptr, std::wstring(ObjectPath.begin(), ObjectPath.end()).c_str(), false));
    }

    static inline void* (*StaticLoadObject_)(UClass*, UObject*,  const TCHAR*, const TCHAR*, uint32_t, UObject*, bool, void*) =reinterpret_cast<decltype(StaticLoadObject_)>(Helios::Offsets::StaticLoadObject);

    template<typename T>
    T* StaticLoadObject(std::string name)
    {
        T* Object = StaticFindObject<T>(name);

        if (!Object)
        {
            auto Name = std::wstring(name.begin(), name.end()).c_str();
            Object = (T*)StaticLoadObject_(T::StaticClass(), nullptr, Name, nullptr, 0, nullptr, false, nullptr);
        }

        return Object;
    }

    template <typename _Ot = void*>
    __forceinline static void Exec(const char* _Name, void* _Detour, _Ot& _Orig = nullptrForHook) {
        auto _Fn = StaticFindObject<UFunction>(_Name);
        if (!_Fn)
            return;

        if (!std::is_same_v<_Ot, void*>)
            _Orig = (_Ot)_Fn->ExecFunction;

        _Fn->ExecFunction = reinterpret_cast<UFunction::FNativeFuncPtr>(_Detour);
    }

    static float EvaluateScalableFloat(FScalableFloat& Float, float Level = 0.f)
    {
        if (!Float.Curve.CurveTable || !Float.Curve.RowName.IsValid())
            return Float.Value;

        float Out = 0.f;
        UDataTableFunctionLibrary::EvaluateCurveTableRow(Float.Curve.CurveTable, Float.Curve.RowName, Level, nullptr, &Out, FString());
        return Float.Value * Out;
    }

    template <class T>
    static T* SpawnActor(FVector Location = {}, FRotator Rotation = {}, UClass* Class = T::StaticClass(), AActor* Owner = nullptr)
    {
        FTransform Transform{};
        Transform.Rotation = Rotation.Quat();
        Transform.Translation = Location;
        Transform.Scale3D = { 1, 1, 1 };

        auto Actor = UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn, Owner, ESpawnActorScaleMethod::MultiplyWithRoot);
        if (Actor)
            UGameplayStatics::FinishSpawningActor(Actor, Transform, ESpawnActorScaleMethod::MultiplyWithRoot);

        return (T*)Actor;
    }

    template <class T>
    static T* SpawnActor(FTransform Transform = {}, UClass* Class = T::StaticClass(), AActor* Owner = nullptr)
    {
        auto Actor = UGameplayStatics::BeginDeferredActorSpawnFromClass(UWorld::GetWorld(), Class, Transform, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding, Owner, ESpawnActorScaleMethod::MultiplyWithRoot);
        if (Actor)
            UGameplayStatics::FinishSpawningActor(Actor, Transform, ESpawnActorScaleMethod::MultiplyWithRoot);

        return (T*)Actor;
    }

    template<class T>
    static inline T* Cast(UObject* Object)
    {
        return Object && (Object->IsA(T::StaticClass())) ? (T*)Object : nullptr;
    }

    static void Restart()
    {
        Sleep(30 * 60 * 1000);
        TerminateProcess(GetCurrentProcess(), 0);
    }

    static void DestroyBuildings()
    {
        while (true)
        {
            Sleep(420000);

            for (auto& Actor : Runtime::GetAll<ABuildingSMActor>())
            {
                auto* Building = static_cast<ABuildingSMActor*>(Actor);
                if (!Building || !Building->bPlayerPlaced)
                    continue;

                Building->K2_DestroyActor();
            }
        }
    }

    static void DestroyPickups()
    {
        while (true)
        {
            Sleep(10000);

            for (auto& Actor : Runtime::GetAll<AFortPickupAthena>())
            {
                auto* Pickup = static_cast<AFortPickupAthena*>(Actor);
                if (Pickup)
                    Pickup->K2_DestroyActor();
            }
        }
    }

    static void PatchNetModes(uintptr_t AttemptDeriveFromURL)
    {
        Memcury::PE::Address add{ nullptr };

        const auto sizeOfImage = Memcury::PE::GetNTHeaders()->OptionalHeader.SizeOfImage;
        const auto scanBytes = reinterpret_cast<std::uint8_t*>(Memcury::PE::GetModuleBase());

        for (auto i = 0ul; i < sizeOfImage - 5; ++i)
        {
            if (scanBytes[i] == 0xE8 || scanBytes[i] == 0xE9)
            {
                if (Memcury::PE::Address(&scanBytes[i]).RelativeOffset(1).GetAs<void*>() == (void*)AttemptDeriveFromURL)
                {
                    add = Memcury::PE::Address(&scanBytes[i]);

                    for (auto j = 0; j > -0x100000; j--)
                    {
                        if ((scanBytes[i + j] & 0xF8) == 0x48 && ((scanBytes[i + j + 1] & 0xFC) == 0x80 || (scanBytes[i + j + 1] & 0xF8) == 0x38) && (scanBytes[i + j + 2] & 0xF0) != 0xC0 && (scanBytes[i + j + 2] & 0xF0) != 0xE0 && scanBytes[i + j + 2] != 0x65 && scanBytes[i + j + 2] != 0xBB && scanBytes[i + j + 3] == 0x38 && ((scanBytes[i + j + 1] & 0xFC) != 0x80 || scanBytes[i + j + 4] == 0x0))
                        {
                            bool found = false;
                            for (auto k = 4; k < 0x104; k++)
                            {
                                if (scanBytes[i + j + k] == 0x75)
                                {
                                    auto Scuffness = __int64(&scanBytes[i + j + k + 5]);

                                    if (*(uint32_t*)Scuffness != 0xF0 && (scanBytes[i + j + k + 4] != 0xC || scanBytes[i + j + k + 5] != 0xB) && scanBytes[i + j + k + 4] != 0x09)
                                        continue;

                                    Runtime::Patch<uint16_t>(__int64(&scanBytes[i + j + k]), 0x9090);
                                    if ((scanBytes[i + j + 1] & 0xF8) == 0x38)
                                        Runtime::Patch<uint32_t>(__int64(&scanBytes[i + j]), 0x90909090);
                                    else if ((scanBytes[i + j + 1] & 0xFC) == 0x80)
                                    {
                                        DWORD og;
                                        VirtualProtect(&scanBytes[i + j], 5, PAGE_EXECUTE_READWRITE, &og);
                                        *(uint32*)(&scanBytes[i + j]) = 0x90909090;
                                        *(uint8*)(&scanBytes[i + j + 4]) = 0x90;
                                        VirtualProtect(&scanBytes[i + j], 5, og, &og);
                                    }
                                    FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j], 5);
                                    FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j + k], 2);
                                    found = true;
                                    break;
                                }
                                else if (scanBytes[i + j + k] == 0x74)
                                {
                                    auto Scuffness = __int64(&scanBytes[i + j + k]);
                                    Scuffness = (Scuffness + 2) + *(int8_t*)(Scuffness + 1);

                                    if (*(uint32_t*)(Scuffness + 3) != 0xF0 && (*(uint8_t*)(Scuffness + 2) != 0xC || *(uint8_t*)(Scuffness + 3) != 0xB) && *(uint8_t*)(Scuffness + 2) != 0x09)
                                        continue;

                                    Runtime::Patch<uint8_t>(__int64(&scanBytes[i + j + k]), 0xeb);
                                    if ((scanBytes[i + j + 1] & 0xF8) == 0x38)
                                        Runtime::Patch<uint32_t>(__int64(&scanBytes[i + j]), 0x90909090);
                                    else if ((scanBytes[i + j + 1] & 0xFC) == 0x80)
                                    {
                                        DWORD og;
                                        VirtualProtect(&scanBytes[i + j], 5, PAGE_EXECUTE_READWRITE, &og);
                                        *(uint32*)(&scanBytes[i + j]) = 0x90909090;
                                        *(uint8*)(&scanBytes[i + j + 4]) = 0x90;
                                        VirtualProtect(&scanBytes[i + j], 5, og, &og);
                                    }
                                    FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j], 5);
                                    FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j + k], 1);
                                    found = true;
                                    break;
                                }
                                else if (scanBytes[i + j + k] == 0x0F && scanBytes[i + j + k + 1] == 0x85)
                                {
                                    auto Scuffness = __int64(&scanBytes[i + j + k + 9]);

                                    if (*(uint32_t*)Scuffness != 0xF0 && (scanBytes[i + j + k + 8] != 0xC || scanBytes[i + j + k + 9] != 0xB) && scanBytes[i + j + k + 8] != 0x09)
                                        continue;

                                    DWORD og;
                                    VirtualProtect(&scanBytes[i + j + k], 6, PAGE_EXECUTE_READWRITE, &og);
                                    *(uint32*)(&scanBytes[i + j + k]) = 0x90909090;
                                    *(uint16*)(&scanBytes[i + j + k + 4]) = 0x9090;
                                    VirtualProtect(&scanBytes[i + j + k], 6, og, &og);
                                    if ((scanBytes[i + j + 1] & 0xF8) == 0x38)
                                        Runtime::Patch<uint32_t>(__int64(&scanBytes[i + j]), 0x90909090);
                                    else if ((scanBytes[i + j + 1] & 0xFC) == 0x80)
                                    {
                                        DWORD og;
                                        VirtualProtect(&scanBytes[i + j], 5, PAGE_EXECUTE_READWRITE, &og);
                                        *(uint32*)(&scanBytes[i + j]) = 0x90909090;
                                        *(uint8*)(&scanBytes[i + j + 4]) = 0x90;
                                        VirtualProtect(&scanBytes[i + j], 5, og, &og);
                                    }
                                    FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j], 5);
                                    FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j + k], 6);
                                    found = true;
                                    break;
                                }
                                else if (scanBytes[i + j + k] == 0x0F && scanBytes[i + j + k + 1] == 0x84)
                                {
                                    auto Scuffness = __int64(&scanBytes[i + j + k]);
                                    Scuffness = (Scuffness + 6) + *(int32_t*)(Scuffness + 2);

                                    if (*(uint32_t*)(Scuffness + 3) != 0xF0 && (*(uint8_t*)(Scuffness + 2) != 0xC || *(uint8_t*)(Scuffness + 3) != 0xB) && *(uint8_t*)(Scuffness + 2) != 0x09)
                                        continue;

                                    Runtime::Patch<uint16_t>(__int64(&scanBytes[i + j + k]), 0xe990);
                                    if ((scanBytes[i + j + 1] & 0xF8) == 0x38)
                                        Runtime::Patch<uint32_t>(__int64(&scanBytes[i + j]), 0x90909090);
                                    else if ((scanBytes[i + j + 1] & 0xFC) == 0x80)
                                    {
                                        DWORD og;
                                        VirtualProtect(&scanBytes[i + j], 5, PAGE_EXECUTE_READWRITE, &og);
                                        *(uint32*)(&scanBytes[i + j]) = 0x90909090;
                                        *(uint8*)(&scanBytes[i + j + 4]) = 0x90;
                                        VirtualProtect(&scanBytes[i + j], 5, og, &og);
                                    }
                                    FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j], 5);
                                    FlushInstructionCache(GetCurrentProcess(), &scanBytes[i + j + k], 2);
                                    found = true;
                                    break;
                                }
                            }
                            if (found)
                                break;
                        }
                    }
                }
            }
        }
    }
}

static UFunction* FindFunctionByFName(UClass* Class, FName FuncName)
{
    for (const UStruct* Clss = Class; Clss; Clss = Clss->Super)
    {
        for (UField* Field = Clss->Children; Field; Field = Field->Next)
        {
            if (Field->HasTypeFlag(EClassCastFlags::Function) && Field->Name == FuncName)
                return static_cast<class UFunction*>(Field);
        }
    }
    return nullptr;
}

template<typename Ret, typename... Args>
void TMulticastInlineDelegate<Ret(Args...)>::ProcessDelegate(Args... args)
{
    using FParamsTuple = std::tuple<std::decay_t<Args>...>;
    FParamsTuple Tuple(std::forward<Args>(args)...);

    for (auto& ScriptDelegate : InvocationList)
    {
        if (!ScriptDelegate.Object->Class || !ScriptDelegate.FunctionName.IsValid())
            return;

        UFunction* Function = FindFunctionByFName(ScriptDelegate.Object->Class, ScriptDelegate.FunctionName);

        if constexpr (sizeof...(Args) == 0)
            ScriptDelegate.Object->ProcessEvent(Function, nullptr);
        else
            ScriptDelegate.Object->ProcessEvent(Function, &Tuple);
    }
}

class FOutputDevice
{
public:
    bool bSuppressEventTag;
    bool bAutoEmitLineTerminator;
    uint8_t _Padding1[0x6];
};

struct FOutParmRec
{
    FProperty* Property;
    uint8* PropAddr;
    FOutParmRec* NextOutParm;
};

class FFrame : public FOutputDevice
{
public:
    void** VTable;
    UFunction* Node;
    UObject* Object;
    uint8* Code;
    uint8* Locals;
    FProperty* MostRecentProperty;
    uint8_t* MostRecentPropertyAddress;
    uint8* MostRecentPropertyContainer;
    TArray<void*> FlowStack;
    FFrame* PreviousFrame;
    FOutParmRec* OutParms;
    uint8_t _Padding1[0x20];
    FField* PropertyChainForCompiledIn;
    UFunction* CurrentNativeFunction;
    FFrame* PreviousTrackingFrame;
    bool bArrayContextFailed;

public:
    inline void StepCompiledIn(void* const Result, bool ForceExplicitProp = false)
    {
        if (Code && !ForceExplicitProp)
        {
            ((void (*)(FFrame*, UObject*, void* const))(Helios::Offsets::StepCompiledInCode))(this, Object, Result);
        }
        else
        {
            FField* _Prop = PropertyChainForCompiledIn;
            PropertyChainForCompiledIn = _Prop->Next;
            ((void (*)(FFrame*, void* const, FField*)) (Helios::Offsets::StepCompiledIn))(this, Result, _Prop);
        }
    }

    __forceinline void* StepCompiledInRefInternal(void* _Tm)
    {
        MostRecentPropertyAddress = nullptr;
        *(void**)(__int64(this) + 0x40) = nullptr;

        if (Code)
        {
            ((void (*)(FFrame*, UObject*, void* const))Helios::Offsets::StepCompiledInCode)(this, Object, _Tm);
        }
        else
        {
            const UField* _Prop = *(const UField**)(__int64(this) + 0x88);
            if (!_Prop)
                return _Tm;

            *(const UField**)(__int64(this) + 0x88) =
                *(const UField**)(__int64(_Prop) + 0x18);

            ((void (*)(FFrame*, void* const, const UField*))Helios::Offsets::StepCompiledIn)(this, _Tm, _Prop);
        }

        return MostRecentPropertyAddress ? MostRecentPropertyAddress : _Tm;
    }

    template <typename T>
    __forceinline T& StepCompiledInRef()
    {
        T TempVal{};
        return *(T*)StepCompiledInRefInternal(&TempVal);
    }


    inline void IncrementCode()
    {
        Code = (uint8_t*)(__int64(Code) + (bool)Code);
    }
};

#define defOG(_Tr, _Pt, _Th, ...) ([&](){ auto _Fn = Runtime::StaticFindObject<UFunction>(_Pt "." #_Th); _Fn->ExecFunction = (UFunction::FNativeFuncPtr)Originals::_Th; _Tr->_Th(__VA_ARGS__); _Fn->ExecFunction = (UFunction::FNativeFuncPtr)_Th; })()