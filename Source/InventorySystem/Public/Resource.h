// Copyright SygiFox 2020 - Simone Di Gravio dig@sygifox.com

#pragma once

#include "Item.h"
#include "Resource.generated.h"

/**
* Lists all supported kinds of resource.
* This should be customized per project based on the resources you actually have.
**/
UENUM(BlueprintType, Blueprintable)
enum class EResourceCategory : uint8
{
    None,
    Generic,
    Crafting,
    Wood,
    Rock,
    Metal,
    Fiber,
    Food,
    Health,
};

UCLASS(BlueprintType)
class INVENTORYSYSTEM_API UResourceData : public UItemData
{
    GENERATED_BODY()

public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EResourceCategory ResourceCategory = EResourceCategory::None;
};

UCLASS(BlueprintType)
class INVENTORYSYSTEM_API UResourceComponent : public UItemComponent
{
    GENERATED_BODY()

public:
    UResourceComponent();
    virtual EPickupBehavior getPickupBehavior_Implementation() override;
};
