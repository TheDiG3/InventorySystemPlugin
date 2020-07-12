// Copyright SygiFox 2020 - Simone Di Gravio dig@sygifox.com

#pragma once

#include "Item.h"
#include "UObject/ObjectMacros.h"

#include "Tool.generated.h"

/**
* Lists all supported kinds of tools.
* This should be customized per project based on the tools you actually have.
**/
UENUM(BlueprintType,Blueprintable)
enum class EToolCategory: uint8
{
    None,
    Axe,
    Pickaxe
};

UCLASS(BlueprintType)
class INVENTORYSYSTEM_API UToolData : public UItemData
{
    GENERATED_BODY()

public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EToolCategory ToolCategory;
    /** Defines the maximum amount of durability for this tool. This could be number of uses, "health" of the tool or other. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 MaxDurability = 10;
};

UCLASS(BlueprintType)
class INVENTORYSYSTEM_API UToolComponent : public UItemComponent
{
    GENERATED_BODY()

public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Tool", SaveGame)
    int32 Durability;

public:
    UToolComponent();
    UFUNCTION(BlueprintCallable, BlueprintPure)
    FORCEINLINE UToolData* ToolData() const { return Cast<UToolData>(ItemData); }

    virtual EPickupBehavior getPickupBehavior_Implementation() override;
    virtual void PostLoad() override;

#if UE_EDITOR
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

private:
    void initToolDurability();
};
