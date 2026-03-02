#include "pch.h"
#include "Runtime.h"

UObject* SDK::InternalGet(FSoftObjectPtr* Ptr, UClass* Class)
{
    if (!Ptr)
        return nullptr;

    auto Ret = Ptr->WeakPtr.ObjectIndex && Ptr->WeakPtr.ObjectSerialNumber ? Ptr->Get() : nullptr;
    if ((!Ret || !Ret->IsA(Class)) && Ptr->ObjectID.AssetPath.AssetName.ComparisonIndex > 0)
    {
        auto Path = Ptr->ObjectID.AssetPath.PackageName.GetRawString();
        if (!Ptr->ObjectID.AssetPath.AssetName.GetRawString().empty())
            Path += "." + Ptr->ObjectID.AssetPath.AssetName.GetRawString();

        Ptr->WeakPtr = Ret = Runtime::StaticFindObject<UObject>(Path, Class);
    }

    return Ret;
}