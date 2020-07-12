// Copyright SygiFox 2020 - Simone Di Gravio dig@sygifox.com

#pragma once

#include "InventorySystemCommon.h"
#include "Pickable.h"
#include "Components/ActorComponent.h"
#include "Engine/DataAsset.h"

#include "Item.generated.h"

class UInventoryBagComponent;

UENUM(BlueprintType)
enum class EItemCategory : uint8
{
    None,
    Resource,
    Tool
};

/**
 * Base class for item data.
 * Subclass this to provide your own custom item data.
 */
UCLASS(BlueprintType, Blueprintable)
class INVENTORYSYSTEM_API UItemData : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    EItemCategory Category = EItemCategory::None;
    /** Actor that will be spawned when an item of this type is dropped from inventory. No actor will be spawned if unset.*/
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TSubclassOf<AActor> OnDropSpawnedActor;
};

/**
 * Adding this component to an actor makes it an Item usable by the Inventory System.
 * Subclass from this to provide your own functionality and item data.
 */
UCLASS(BlueprintType, Blueprintable, ClassGroup="Item", meta = (BlueprintSpawnableComponent))
class INVENTORYSYSTEM_API UItemComponent : public UActorComponent, public IPickable
{
    GENERATED_BODY()
public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory")
    UItemData* ItemData;

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere, Category="Inventory")
    UInventoryBagComponent* OwningBag;

public:
    UItemComponent();

    virtual AActor* getPickableActor_Implementation() override;
    virtual UItemData* getItemData_Implementation() override;
    virtual UItemComponent* getItemComponent_Implementation() override;
    virtual FVector getPickableLocation_Implementation() override;
    virtual EPickupBehavior getPickupBehavior_Implementation() override;
    virtual TScriptInterface<IPickable> getPickable_Implementation() override;
    virtual void OnItemPickedUp_Implementation(UInventoryBagComponent* owning_bag) override;
    virtual void OnItemDropped_Implementation(UInventoryBagComponent* owning_bag) override;
};
