// Copyright SygiFox 2020 - Simone Di Gravio dig@sygifox.com

#pragma once

#include "Item.h"
#include "Tool.h"
#include "Resource.h"
#include "Components/ActorComponent.h"
#include "UObject/ObjectMacros.h"
#include "InventoryBagComponent.generated.h"

struct FStreamableHandle;
class UInventoryBagComponent;

/**
 * Defines limits (max quantity etc.) for a single item type in the bag.
 */
UCLASS(Blueprintable, BlueprintType)
class UItemBagLimit : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:

    /** Maximum size of a stack of the same type of item for a single slot. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory")
    int32 MaxStackSize = 20;
    /** Total maximum number of items of the same type that can be held in the bag, counted among all stacks. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory")
    int32 MaxQuantity = 20;
};

/**
 * Defines properties of an inventory bag such as
 * maximum number of slots and limits for each item type.
 */
UCLASS(BlueprintType, Blueprintable)
class UBagProperties : public UPrimaryDataAsset
{
    GENERATED_BODY()

public:

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory")
    TMap<TSoftObjectPtr<UItemData>, TSoftObjectPtr<UItemBagLimit>> Limits;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Inventory")
    int32 MaxResourceSlots = 20;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory")
    int32 MaxToolsSlots = 10;
    UPROPERTY(BlueprintReadOnly, EditAnywhere, Category="Inventory")
    int32 MaxItemId = 1000;
};

/**
 * Holds information about a tool in the bag.
 */
USTRUCT(BlueprintType)
struct FBagToolInfo
{
    GENERATED_BODY()

    friend bool operator==(const FBagToolInfo& lhs, const FBagToolInfo& rhs)
    {
        return lhs.ToolId == rhs.ToolId;
    }

    friend bool operator!=(const FBagToolInfo& lhs, const FBagToolInfo& rhs)
    {
        return !(lhs == rhs);
    }

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 ToolId;
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 Durability;
};

/**
 * Holds IDs of all the resources held in this slot.
 */
USTRUCT(BlueprintType)
struct FBagResourceSlot
{
    GENERATED_BODY()

    friend bool operator==(const FBagResourceSlot& lhs, const FBagResourceSlot& rhs)
    {
        return lhs.ResourceIds == rhs.ResourceIds;
    }

    friend bool operator!=(const FBagResourceSlot& lhs, const FBagResourceSlot& rhs)
    {
        return !(lhs == rhs);
    }

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory")
    int32 Id;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory")
    TArray<int32> ResourceIds;
};

/**
 * Holds tool info (such as ID and current durability) for all tools held in this slot.
 */
USTRUCT(BlueprintType)
struct FBagToolSlot
{
    GENERATED_BODY()

    friend bool operator==(const FBagToolSlot& lhs, const FBagToolSlot& rhs)
    {
        return lhs.ToolsInfo == rhs.ToolsInfo;
    }

    friend bool operator!=(const FBagToolSlot& lhs, const FBagToolSlot& rhs)
    {
        return !(lhs == rhs);
    }

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory")
    int32 Id;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory")
    TArray<FBagToolInfo> ToolsInfo;
};

/**
 * Holds actual slots data used by a single resource type (UResourceData).
 * All resources of a specific data type will be placed inside slots.
 * Each slot holds a maximum of the used item data type's max stack and there's a cap for the
 * total number of items held between all slots.
 */
USTRUCT(BlueprintType)
struct FBagResourcesData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory")
    TArray<FBagResourceSlot> Slots;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory")
    int32 ResourceQuantity = 0;
};

/**
* Holds actual slots data used by a single tool type (UToolData).
* All tools of a specific data type will be placed inside slots.
* Each slot holds a maximum of the used item data type's max stack and there's a cap for the
* total number of items held between all slots.
*/
USTRUCT(BlueprintType)
struct FBagToolsData
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory")
    int32 ToolQuantity = 0;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory")
    TArray<FBagToolSlot> Slots;
};

/**
 * Holds all the data for resource items.
 */
USTRUCT(BlueprintType)
struct FBagResources
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory")
    TMap<UResourceData*, FBagResourcesData> Data;
    /** Total number of slots currently in use for all resources. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory")
    int32 UsedSlots;
};

/**
 * Holds all the data for tool items.
 */
USTRUCT(BlueprintType)
struct FBagTools
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory")
    TMap<UToolData*, FBagToolsData> Data;
    /** Total number of slots currently in use for all tools. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory")
    int32 UsedSlots;
};

USTRUCT(BlueprintType)
struct FInventoryBagAddItemResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    bool bAdded;
    UPROPERTY(BlueprintReadWrite)
    int32 AssignedId;
};

USTRUCT(BlueprintType)
struct FInventoryBagRemoveItemResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    bool bRemoved;
    UPROPERTY(BlueprintReadWrite)
    int32 RemovedId = -1;
    UPROPERTY(BlueprintReadWrite)
    AActor* SpawnedActor = nullptr;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInventoryBagUpdatedDelegate, UInventoryBagComponent*, bag);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FInventoryBagResourceSlotUpdatedDelegate, UInventoryBagComponent*, bag, UResourceData*, slot_type, int32, slot_id, FBagResourceSlot, slot);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FInventoryBagToolSlotUpdatedDelegate, UInventoryBagComponent*, bag, UToolData*, slot_type, int32, slot_id, FBagToolSlot, slot);

/**
 * Provides inventory functionality for storing resources and tools.
 */
UCLASS(BlueprintType, Blueprintable)
class UInventoryBagComponent : public UActorComponent
{
    GENERATED_BODY()

public:

    // EVENTS
    UPROPERTY(BlueprintCallable, BlueprintAssignable, Category="Inventory")
    FInventoryBagUpdatedDelegate OnInventoryBagUpdated;
    UPROPERTY(BlueprintCallable, BlueprintAssignable, Category="Inventory")
    FInventoryBagResourceSlotUpdatedDelegate OnResourceSlotAdded;
    UPROPERTY(BlueprintCallable, BlueprintAssignable, Category="Inventory")
    FInventoryBagResourceSlotUpdatedDelegate OnResourceSlotRemoved;
    UPROPERTY(BlueprintCallable, BlueprintAssignable, Category="Inventory")
    FInventoryBagResourceSlotUpdatedDelegate OnResourceSlotUpdated;
    UPROPERTY(BlueprintCallable, BlueprintAssignable, Category="Inventory")
    FInventoryBagToolSlotUpdatedDelegate OnToolSlotAdded;
    UPROPERTY(BlueprintCallable, BlueprintAssignable, Category="Inventory")
    FInventoryBagToolSlotUpdatedDelegate OnToolSlotRemoved;
    UPROPERTY(BlueprintCallable, BlueprintAssignable, Category="Inventory")
    FInventoryBagToolSlotUpdatedDelegate OnToolSlotUpdated;

    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory")
    UBagProperties* BagProperties;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory")
    FBagResources Resources;
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="Inventory")
    FBagTools Tools;

private:

    TArray<int32> item_ids_pool;
    TArray<int32> slot_ids_pool;
    /** Items registered via component can later retrieve their ID through their reference. */
    UPROPERTY()
    TMap<UItemComponent*, int32> item_comp_to_id;
    TSharedPtr<FStreamableHandle> limits_stream_handle;

public:

    UInventoryBagComponent();

    // UItemComponent versions
    UFUNCTION(BlueprintCallable, Category="Inventory")
    FInventoryBagAddItemResult addItemComponent(UItemComponent* item);
    UFUNCTION(BlueprintCallable, Category="Inventory")
    FInventoryBagRemoveItemResult removeItemComponent(UItemComponent* item, bool bAllowActorSpawn = true);
    UFUNCTION(BlueprintCallable, Category="Inventory")
    UItemComponent* getItemComponentFromId(int32 id);

    // UItemData versions
    UFUNCTION(BlueprintCallable, Category="Inventory")
    FInventoryBagAddItemResult addItem(UItemData* item_data);
    UFUNCTION(BlueprintCallable, Category="Inventory")
    FInventoryBagRemoveItemResult removeItem(UItemData* item_data, bool bAllowActorSpawn = true);
    UFUNCTION(BlueprintPure, Category="Inventory")
    int32 getItemQuantity(UItemData* item_data);

    UFUNCTION(BlueprintCallable, Category="Inventory")
    bool updateToolDurability(UToolComponent* tool, int32 const durability);

    void BeginPlay() override;

private:

    bool tryAddItem(UItemData* item_data, int32 const id, UItemComponent* item_component = nullptr);
    /**
     * Tries to remove one item of item_data type.
     * @param item_data Type of item to remove.
     * @param remove_id When >= 0 it will try and remove the item with the specified ID (fails if no such ID can be found).
     *                  Passing -1 will remove one item from the last slot of that item's type and set remove_id to the removed item's ID.
     * @return Whether the item could be removed.
     */
    bool tryRemoveItem(UItemData* item_data, int32& remove_id);
    bool tryAddResource(UResourceData* resource_data, int32 const id);
    /**
    * Tries to remove one resource of resource_data type.
    * @param resource_data Type of resource to remove.
    * @param remove_id When >= 0 it will try and remove the resource with the specified ID (fails if no such ID can be found).
    *                  Passing -1 will remove one resource from the last slot of that resource's type and set remove_id to the removed resource's ID.
    * @return Whether the item could be removed.
    */
    bool tryRemoveResource(UResourceData* resource_data, int32& remove_id);
    bool tryAddTool(UToolData* tool_data, int32 const id, int32 const durability);
    /**
    * Tries to remove one tool of tool_data type.
    * @param tool_data Type of tool to remove.
    * @param remove_id When >= 0 it will try and remove the tool with the specified ID (fails if no such ID can be found).
    *                  Passing -1 will remove one tool from the last slot of that tool's type and set remove_id to the removed tool's ID.
    * @return Whether the tool could be removed.
    */
    bool tryRemoveTool(UToolData* tool_data, int32& remove_id);
    bool isValidItemData(UItemData* item_data) const;
    bool hasValidItemLimits(UItemData* item_data) const;
    bool hasAvailableIds() const;

    /**
     * Starts the process of streaming in all item data and bag limits that will be used with this bag.
     */
    void streamInLimits();
    void handleLimitsStreamedInCompleted() const;
};
