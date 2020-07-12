// Copyright SygiFox 2020 - Simone Di Gravio dig@sygifox.com

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Pickable.generated.h"

class UItemData;
class UItemComponent;
class UInventoryBagComponent;

/**
 * Determines how this pickable wants to be used.
 */
UENUM(BlueprintType, Blueprintable)
enum class EPickupBehavior : uint8
{
    UseDataOnly,
    UseDataAndDestroyActor,
    UseItemComponent
};

// This class does not need to be modified.
UINTERFACE(MinimalAPI, BlueprintType)
class UPickable : public UInterface
{
    GENERATED_BODY()
};

/**
 * An object that can be picked up.
 */
class INVENTORYSYSTEM_API IPickable
{
    GENERATED_BODY()

public:

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    AActor* getPickableActor();
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    UItemData* getItemData();
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    TScriptInterface<IPickable> getPickable();
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    UItemComponent* getItemComponent();
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    bool canPickup(AActor* picker);
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    EPickupBehavior getPickupBehavior();
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    FVector getPickableLocation();
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    void OnItemPickedUp(UInventoryBagComponent* owning_bag);
    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    void OnItemDropped(UInventoryBagComponent* owning_bag);
};
