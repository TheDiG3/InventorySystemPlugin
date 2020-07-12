#include "InventoryBagComponent.h"
#include "Item.h"
#include "Engine/AssetManager.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

/**
 * Provides a safe way to grab an item id from a pool,
 * automatically returning it to the pool if the transaction is not committed before leaving the scope.
 */
struct FScopedItemPoolIdTransaction
{
    FScopedItemPoolIdTransaction(TArray<int32>& id_pool) : pool(id_pool)
    {
        bCommitted = false;
        id = pool.Pop();
    }

    int32 Id() const { return id; }
    void commit() { bCommitted = true; }
    ~FScopedItemPoolIdTransaction() { if (!bCommitted) pool.Push(id); }

private:

    int32 id;
    bool bCommitted;
    TArray<int32>& pool;
};

UInventoryBagComponent::UInventoryBagComponent()
{
}

FInventoryBagAddItemResult UInventoryBagComponent::addItemComponent(UItemComponent* item)
{
    // Safety, item already present and valid limits checks
    if (!IsValid(item) || !isValidItemData(item->ItemData) || !hasAvailableIds() || !hasValidItemLimits(item->ItemData))
    {
        UE_LOG(LogInventorySystem, Display, TEXT("Can't add item [%s] to bag [%s]"), IsValid(item) ? *item->GetPathName() : TEXT("InvalidItem"), *GetPathName());
        return {false, -1};
    }

    // We have a valid item and ID is available. We can try to add the item.
    // If anything fails we should push back the ID we were going to use.
    FScopedItemPoolIdTransaction id_transaction{item_ids_pool};
    UItemData* item_data = item->ItemData;
    if (!tryAddItem(item_data, id_transaction.Id(), item)) return {false, -1};

    id_transaction.commit();
    item_comp_to_id.Add(item, id_transaction.Id());
    UE_LOG(LogInventorySystem, Display, TEXT("Item [%s] added to bag [%s]"), *item->GetPathName(), *GetPathName());
    item->Execute_OnItemPickedUp(item, this);
    OnInventoryBagUpdated.Broadcast(this);
    return {true, id_transaction.Id()};
}

FInventoryBagRemoveItemResult UInventoryBagComponent::removeItemComponent(UItemComponent* item, bool bAllowActorSpawn)
{
    // Safety checks
    if (!IsValid(item) || !isValidItemData(item->ItemData))
    {
        UE_LOG(LogInventorySystem, Display, TEXT("Can't remove item [%s] from bag [%s]"), IsValid(item) ? *item->GetPathName() : TEXT("InvalidItem"), *GetPathName());
        return {false, -1};
    }

    int32* remove_id_ptr = item_comp_to_id.Find(item); // Find item id from component
    if (remove_id_ptr == nullptr)
    {
        UE_LOG(LogInventorySystem, Display, TEXT("Can't remove item [%s] from bag [%s]. Item not found."), *item->GetPathName(), *GetPathName());
        return {false, -1};
    }
    int32 remove_id = *remove_id_ptr;
    UItemData* item_data = item->ItemData;
    if (!tryRemoveItem(item_data, remove_id)) return {false};

    // Recover the used id for later use, spawn wanted actor and trigger dropped event.
    item_ids_pool.Push(remove_id);
    item->Execute_OnItemDropped(item, this);
    AActor* spawn_actor = nullptr;
    if (bAllowActorSpawn && IsValid(item_data->OnDropSpawnedActor))
    {
        spawn_actor = GetWorld()->SpawnActor(item_data->OnDropSpawnedActor);
        UItemComponent* actor_item_comp = Cast<UItemComponent>(spawn_actor->GetComponentByClass(UItemComponent::StaticClass()));
        if (actor_item_comp != nullptr) actor_item_comp->OnItemDropped(this);
    }
    UE_LOG(LogInventorySystem, Display, TEXT("Item [%s] removed from bag [%s]"), *item->GetPathName(), *GetPathName());
    OnInventoryBagUpdated.Broadcast(this);
    return {true, remove_id, spawn_actor};
}

UItemComponent* UInventoryBagComponent::getItemComponentFromId(int32 id)
{
    UItemComponent* const* item_comp = item_comp_to_id.FindKey(id);
    if (item_comp != nullptr)
    {
        // Item component might have been destroyed.
        // Maybe the user added the item via item component and then destroyed the owning actor.
        if (IsValid(*item_comp)) return *item_comp;

        // Clear out NULL (deleted) components.
        UE_LOG(LogInventorySystem, Warning,
               TEXT(
                   "You have removed an item that was registered as an item component but that does not exist anymore. That most likely means its owning actor was deleted. If you immediately delete an item actor on pickup consider just adding the item data to the bag."
               ));
        item_comp_to_id.Remove(*item_comp);
    }
    UE_LOG(LogInventorySystem, Display, TEXT("Can't find item with id [%d] in bag [%s]."), id, *GetPathName());
    return nullptr;
}

FInventoryBagAddItemResult UInventoryBagComponent::addItem(UItemData* item_data)
{
    if (!isValidItemData(item_data) || !hasAvailableIds() || !hasValidItemLimits(item_data))
    {
        UE_LOG(LogInventorySystem, Display, TEXT("Can't add item [%s] to bag [%s]"), IsValid(item_data) ? *item_data->GetPathName() : TEXT("InvalidItem"), *GetPathName());
        return {false, -1};
    }

    // We have a valid item and ID is available. We can try to add the item.
    // If anything fails we should push back the ID we were going to use.
    FScopedItemPoolIdTransaction id_transaction{item_ids_pool};
    if (!tryAddItem(item_data, id_transaction.Id())) return {false, -1};

    id_transaction.commit();
    UE_LOG(LogInventorySystem, Display, TEXT("Item [%s] added to bag [%s]"), *item_data->GetPathName(), *GetPathName());
    OnInventoryBagUpdated.Broadcast(this);
    return {true, id_transaction.Id()};
}

FInventoryBagRemoveItemResult UInventoryBagComponent::removeItem(UItemData* item_data, bool bAllowActorSpawn)
{
    if (!isValidItemData(item_data))
    {
        UE_LOG(LogInventorySystem, Display, TEXT("Can't remove item [%s] from bag [%s]"), IsValid(item_data) ? *item_data->GetPathName() : TEXT("InvalidItem"), *GetPathName());
        return {false};
    }

    int32 remove_id = -1; // Remove from last slot.
    if (!tryRemoveItem(item_data, remove_id)) return {false};

    // Recover the used id for later use, spawn wanted actor and trigger dropped event.
    item_ids_pool.Push(remove_id);
    // Find whether one of the registered item components has the id we want to remove.
    UItemComponent* const* item_comp = item_comp_to_id.FindKey(remove_id);
    if (item_comp != nullptr)
    {
        // Item component might have been destroyed.
        // Maybe the user added the item via item component and then destroyed the owning actor.
        if (IsValid(*item_comp))
        {
            (*item_comp)->Execute_OnItemDropped(*item_comp, this);
        }
        else
        {
            UE_LOG(LogInventorySystem, Warning,
                   TEXT(
                       "You have removed an item that was registered as an item component but that does not exist anymore. That most likely means its owning actor was deleted. If you immediately delete an item actor on pickup consider just adding the item data to the bag."
                   ));
        }
        item_comp_to_id.Remove(*item_comp); // This will also clear out NULL (deleted) components.
    }
    AActor* spawn_actor = nullptr;
    if (bAllowActorSpawn && IsValid(item_data->OnDropSpawnedActor))
    {
        spawn_actor = GetWorld()->SpawnActor(item_data->OnDropSpawnedActor);
        UItemComponent* actor_item_comp = Cast<UItemComponent>(spawn_actor->GetComponentByClass(UItemComponent::StaticClass()));
        if (actor_item_comp != nullptr) actor_item_comp->Execute_OnItemDropped(actor_item_comp, this);
    }
    UE_LOG(LogInventorySystem, Display, TEXT("Item [%s] removed from bag [%s]"), *item_data->GetPathName(), *GetPathName());
    OnInventoryBagUpdated.Broadcast(this);
    return {true, remove_id, spawn_actor};
}

int32 UInventoryBagComponent::getItemQuantity(UItemData* item_data)
{
    if (!isValidItemData(item_data))
    {
        UE_LOG(LogInventorySystem, Display, TEXT("Invalid item data."), IsValid(item_data) ? *item_data->GetPathName() : TEXT("InvalidItem"), *GetPathName());
        return 0;
    }

    switch (item_data->Category)
    {
    case EItemCategory::Resource:
        {
            UResourceData* resource_data = Cast<UResourceData>(item_data);
            if (resource_data == nullptr)
            {
                UE_LOG(LogInventorySystem, Error, TEXT("Invalid resource data type [%s]"), *GetPathName());
                return false;
            }
            return Resources.Data.Contains(resource_data) ? Resources.Data[resource_data].ResourceQuantity : 0;
        }
    case EItemCategory::Tool:
        {
            UToolData* tool_data = Cast<UToolData>(item_data);
            if (tool_data == nullptr)
            {
                UE_LOG(LogInventorySystem, Error, TEXT("Invalid tool data type [%s]"), *GetPathName());
                return false;
            }
            return Tools.Data.Contains(tool_data) ? Tools.Data[tool_data].ToolQuantity : 0;
        }
    case EItemCategory::None: ;
    default:
        {
            UE_LOG(LogInventorySystem, Warning, TEXT("Invalid item category [%s]"), *GetPathName());
            return false;
        }
    }
}

bool UInventoryBagComponent::updateToolDurability(UToolComponent* tool, int32 const durability)
{
    if (!IsValid(tool) || !isValidItemData(tool->ItemData))
    {
        UE_LOG(LogInventorySystem, Display, TEXT("Invalid item [%s]"), IsValid(tool) ? *tool->GetPathName() : TEXT("InvalidTool"));
        return false;
    }

    int32* tool_id_ptr = item_comp_to_id.Find(tool);
    if (tool_id_ptr == nullptr)
    {
        UE_LOG(LogInventorySystem, Display, TEXT("Tool [%s] not found in bag [%s]."), *tool->GetPathName(), *GetPathName());
        return false;
    }
    int32 const tool_id = *tool_id_ptr;
    UToolData* tool_data = Cast<UToolData>(tool->ItemData);
    if (tool_data == nullptr)
    {
        UE_LOG(LogInventorySystem, Error, TEXT("Invalid tool data type [%s]"), *tool->GetPathName());
    }

    check(Tools.Data.Contains(tool_data)); // Tools with valid ID should always be mapped.
    // Search for the tool with matching ID.
    // IMPROV: Maybe keep updated info to avoid searching by tool ID.
    for (auto&& slot : Tools.Data[tool_data].Slots)
    {
        for (auto&& tool_info : slot.ToolsInfo)
        {
            if (tool_info.ToolId == tool_id)
            {
                tool_info.Durability = durability;
                OnToolSlotUpdated.Broadcast(this, tool_data, slot.Id, slot);
                return true;
            }
        }
    }
    checkNoEntry(); // Shouldn't be able to get here since if we have a tool_comp->id mapping the tool should be found in one of the slots
    return false;
}

void UInventoryBagComponent::BeginPlay()
{
    Super::BeginPlay();
    streamInLimits();

    // Init tool id pool
    item_ids_pool.AddUninitialized(BagProperties->MaxItemId);
    for (int i = 0; i < item_ids_pool.Num(); ++i)
    {
        item_ids_pool[i] = i;
    }
    // Init slot id pool
    slot_ids_pool.AddUninitialized(BagProperties->MaxToolsSlots + BagProperties->MaxResourceSlots);
    for (int i = 0; i < slot_ids_pool.Num(); ++i)
    {
        slot_ids_pool[i] = i;
    }
}

bool UInventoryBagComponent::tryAddItem(UItemData* item_data, int32 const id, UItemComponent* item_component /** = nullptr */)
{
    check(IsValid(item_data));

    switch (item_data->Category)
    {
    case EItemCategory::Resource:
        {
            UResourceData* resource_data = Cast<UResourceData>(item_data);
            if (resource_data == nullptr)
            {
                UE_LOG(LogInventorySystem, Error, TEXT("Trying to add resource with invalid resource data type to the bag [%s]"), *GetPathName());
                return false;
            }
            return tryAddResource(resource_data, id);
        }
    case EItemCategory::Tool:
        {
            UToolData* tool_data = Cast<UToolData>(item_data);
            int32 durability = tool_data->MaxDurability;
            if (item_component != nullptr)
            {
                UToolComponent* tool_component = Cast<UToolComponent>(item_component);
                if (tool_component == nullptr)
                {
                    UE_LOG(LogInventorySystem, Error, TEXT("Trying to add invalid tool to the bag [%s]"), *GetPathName());
                    return false;
                }
                durability = tool_component->Durability;
            }
            if (tool_data == nullptr)
            {
                UE_LOG(LogInventorySystem, Error, TEXT("Trying to add tool with invalid tool data type to the bag [%s]"), *GetPathName());
                return false;
            }
            return tryAddTool(tool_data, id, durability);
        }
    case EItemCategory::None: ;
    default:
        {
            UE_LOG(LogInventorySystem, Warning, TEXT("Trying to add item with invalid category to the bag [%s]"), *GetPathName());
            return false;
        }
    }
}

bool UInventoryBagComponent::tryRemoveItem(UItemData* item_data, int32& remove_id)
{
    check(IsValid(item_data));

    // Remove tool or resource?
    switch (item_data->Category)
    {
    case EItemCategory::Resource:
        {
            UResourceData* resource_data = Cast<UResourceData>(item_data);
            if (resource_data == nullptr)
            {
                UE_LOG(LogInventorySystem, Error, TEXT("Trying to remove resource with invalid resource data type from the bag [%s]"), *GetPathName());
                return false;
            }
            return tryRemoveResource(resource_data, remove_id);
        }
    case EItemCategory::Tool:
        {
            UToolData* tool_data = Cast<UToolData>(item_data);
            if (tool_data == nullptr)
            {
                UE_LOG(LogInventorySystem, Error, TEXT("Trying to remove tool with invalid tool data type from the bag [%s]"), *GetPathName());
                return false;
            }
            return tryRemoveTool(tool_data, remove_id);
        }
    case EItemCategory::None: ;
    default:
        {
            UE_LOG(LogInventorySystem, Warning, TEXT("Trying to remove item with invalid category from the bag [%s]"), *GetPathName());
            return false;
        }
    }
}

bool UInventoryBagComponent::tryAddResource(UResourceData* resource_data, int32 const id)
{
    checkf(BagProperties->Limits[resource_data].Get() != nullptr,
           TEXT("%s should be loaded by now but it's not. Check why that happens. It should get loaded since hasValidLimits -called before tryAdd- loads the objects of the map."),
           *resource_data->GetPathName());
    UItemBagLimit const* const bag_limit = BagProperties->Limits.FindRef(resource_data).Get();
    // 0 max quantity limit check should already be performed at this point.
    check(bag_limit->MaxQuantity > 0 && bag_limit->MaxStackSize > 0);

    // This will point to either newly added resources data or already present one.
    // Just a cache to avoid a double find on the map.
    FBagResourcesData* resources_data;

    // Check if we already have data for this resource type.
    // See if we can add another slot in case we need it.
    if (!Resources.Data.Contains(resource_data))
    {
        if (Resources.UsedSlots >= BagProperties->MaxResourceSlots)
        {
            UE_LOG(LogInventorySystem, Verbose, TEXT("Can't add another resource slot. Max slots capacity reached for bag [%s]."), *GetPathName());
            return false;
        }
        resources_data = &Resources.Data.Emplace(resource_data, FBagResourcesData());
        UE_LOG(LogInventorySystem, Verbose, TEXT("Added new resource type [%s] to bag [%s]."), *resource_data->GetPathName(), *GetPathName());
    }
    else resources_data = Resources.Data.Find(resource_data);

    // We can now treat both paths (newly added resource type and already present resource type) the same way.
    if (resources_data->ResourceQuantity >= bag_limit->MaxQuantity)
    {
        UE_LOG(LogInventorySystem, Verbose, TEXT("Can't add resource. Max quantity capacity reached for bag [%s]."), *GetPathName());
        return false;
    }

    // Find first slot with free space.
    for (auto&& slot : resources_data->Slots)
    {
        if (slot.ResourceIds.Num() < bag_limit->MaxStackSize)
        {
            slot.ResourceIds.Add(id);
            ++resources_data->ResourceQuantity;
            OnResourceSlotUpdated.Broadcast(this, resource_data, slot.Id, slot);
            return true;
        }
    }

    // No free slot, try to create one if possible.
    if (Resources.UsedSlots >= BagProperties->MaxResourceSlots)
    {
        UE_LOG(LogInventorySystem, Verbose, TEXT("Can't add another resource slot. Max slots capacity reached for bag [%s]."), *GetPathName());
        return false;
    }
    // Add new slot and add item to it
    FBagResourceSlot& new_slot = resources_data->Slots[resources_data->Slots.Add({slot_ids_pool.Pop()})];
    new_slot.ResourceIds.Add(id);
    ++Resources.UsedSlots;
    UE_LOG(LogInventorySystem, Verbose, TEXT("Added new resource slot to bag [%s]."), *GetPathName());
    ++resources_data->ResourceQuantity;
    OnResourceSlotAdded.Broadcast(this, resource_data, new_slot.Id, new_slot);
    return true;
}

bool UInventoryBagComponent::tryRemoveResource(UResourceData* resource_data, int32& remove_id)
{
    check(IsValid(resource_data) && remove_id >= -1);

    FBagResourcesData* bag_resources_data_ptr = Resources.Data.Find(resource_data);
    // We actually don't have this kind of resource type.
    if (bag_resources_data_ptr == nullptr)
    {
        UE_LOG(LogInventorySystem, Display, TEXT("Can't remove item [%s] from bag [%s]. No resource of this type found."), *resource_data->GetPathName(), *GetPathName());
        return false;
    }

    FBagResourcesData& bag_resources_data = *bag_resources_data_ptr;
    check(bag_resources_data.ResourceQuantity > 0); // Since we remove any resource data mapping when the quantity reaches 0, this should be an error if hit.

    FBagResourceSlot* selected_resource_slot = nullptr;
    // Just remove the last item from the last slot
    if (remove_id == -1)
    {
        selected_resource_slot = &bag_resources_data.Slots.Last();
        check(selected_resource_slot->ResourceIds.Num() > 0); // We remove empty slots. Count as error if this happens.
        remove_id = selected_resource_slot->ResourceIds.Pop();
    }
    else // Or search for the specified item ID in all the slots.
    {
        for (auto&& slot : bag_resources_data.Slots)
        {
            check(selected_resource_slot->ResourceIds.Num() > 0); // We remove empty slots. Count as error if this happens.
            int const removed_count = slot.ResourceIds.Remove(remove_id);
            check(removed_count <= 1); // Should only ever have unique ids
            // Early out when we find the item.
            if (removed_count > 0)
            {
                selected_resource_slot = &slot;
                break;
            }
        }
    }

    // When searching for a specific ID we might not actually
    // have it (ideally shouldn't happen), hence the slot could not be found.
    if (selected_resource_slot == nullptr)
    {
        UE_LOG(LogInventorySystem, Display, TEXT("Can't remove item [%s] from bag [%s]. Resource with ID [%d] not found."), *resource_data->GetPathName(), *GetPathName(), remove_id);
        return false;
    }

    // Check for empty slot and remove it
    bool bSlotRemoved = false;
    const int32 removed_slot_id = selected_resource_slot->Id; // Save the id in case we remove the slot data.
    if (selected_resource_slot->ResourceIds.Num() == 0)
    {
        bag_resources_data.Slots.RemoveSingleSwap(*selected_resource_slot);
        UE_LOG(LogInventorySystem, Verbose, TEXT("Removed one resource slot [type: %s] from bag [%s]."), *resource_data->GetPathName(), *GetPathName(), remove_id);
        --Resources.UsedSlots;
        slot_ids_pool.Push(removed_slot_id);
        bSlotRemoved = true;
    }
    --bag_resources_data.ResourceQuantity;
    // Remove mappings when we don't have any more resources of this type
    if (bag_resources_data.ResourceQuantity == 0)
    {
        UE_LOG(LogInventorySystem, Verbose, TEXT("Removed resource mapping [type: %s] from bag [%s]."), *resource_data->GetPathName(), *GetPathName(), remove_id);
        Resources.Data.Remove(resource_data);
    }

    if (bSlotRemoved) OnResourceSlotRemoved.Broadcast(this, resource_data, removed_slot_id, {removed_slot_id});
    else OnResourceSlotUpdated.Broadcast(this, resource_data, selected_resource_slot->Id, *selected_resource_slot);
    return true;
}

bool UInventoryBagComponent::tryAddTool(UToolData* tool_data, int32 const id, int32 const durability)
{
    checkf(BagProperties->Limits[tool_data].Get() != nullptr,
           TEXT("%s should be loaded by now but it's not. Check why that happens. It should get loaded since hasValidLimits -called before tryAdd- loads the objects of the map."),
           *tool_data->GetPathName());
    UItemBagLimit const* const bag_limit = BagProperties->Limits.FindRef(tool_data).Get();
    // 0 max quantity limit check should already be performed at this point.
    check(bag_limit->MaxQuantity > 0 && bag_limit->MaxStackSize > 0);

    // This will point to either newly added tools data or already present one.
    // Just a cache to avoid a double find on the map.
    FBagToolsData* tools_data;
    // Check if we already have data for this tool type.
    // See if we can add another slot in case we need it.
    if (!Tools.Data.Contains(tool_data))
    {
        if (Tools.UsedSlots >= BagProperties->MaxResourceSlots)
        {
            UE_LOG(LogInventorySystem, Verbose, TEXT("Can't add another tool slot. Max slots capacity reached for bag [%s]."), *GetPathName());
            return false;
        }
        tools_data = &Tools.Data.Emplace(tool_data, FBagToolsData());
        UE_LOG(LogInventorySystem, Verbose, TEXT("Added new tool type [%s] to bag [%s]."), *tool_data->GetPathName(), *GetPathName());
    }
    else tools_data = Tools.Data.Find(tool_data);

    // We can now treat both paths (newly added tool type and already present tool type) the same way.
    if (tools_data->ToolQuantity >= bag_limit->MaxQuantity)
    {
        UE_LOG(LogInventorySystem, Verbose, TEXT("Can't add tool. Max quantity capacity reached for bag [%s]."), *GetPathName());
        return false;
    }

    // Find first slot with free space.
    for (auto&& slot : tools_data->Slots)
    {
        if (slot.ToolsInfo.Num() < bag_limit->MaxStackSize)
        {
            slot.ToolsInfo.Add({id, durability});
            ++tools_data->ToolQuantity;
            OnToolSlotUpdated.Broadcast(this, tool_data, slot.Id, slot);
            return true;
        }
    }

    // No free slot, try to create one if possible.
    if (Tools.UsedSlots >= BagProperties->MaxResourceSlots)
    {
        UE_LOG(LogInventorySystem, Verbose, TEXT("Can't add another tool slot. Max slots capacity reached for bag [%s]."), *GetPathName());
        return false;
    }

    // Add new slot and add item to it
    FBagToolSlot& new_slot = tools_data->Slots[tools_data->Slots.Add({slot_ids_pool.Pop()})];
    new_slot.ToolsInfo.Add({id, durability});
    ++Tools.UsedSlots;
    UE_LOG(LogInventorySystem, Verbose, TEXT("Added new tool slot to bag [%s]."), *GetPathName());
    ++tools_data->ToolQuantity;
    OnToolSlotAdded.Broadcast(this, tool_data, new_slot.Id, new_slot);
    return true;
}

bool UInventoryBagComponent::tryRemoveTool(UToolData* tool_data, int32& remove_id)
{
    check(IsValid(tool_data) && remove_id >= -1);

    FBagToolsData* bag_tools_data_ptr = Tools.Data.Find(tool_data);
    // We actually don't have this kind of tool type.
    if (bag_tools_data_ptr == nullptr)
    {
        UE_LOG(LogInventorySystem, Display, TEXT("Can't remove item [%s] from bag [%s]. No tool of this type found."), *tool_data->GetPathName(), *GetPathName());
        return false;
    }

    FBagToolsData& bag_tools_data = *bag_tools_data_ptr;
    check(bag_tools_data.ToolQuantity > 0); // Since we remove any tool data mapping when the quantity reaches 0, this should be an error if hit.

    FBagToolSlot* selected_tool_slot = nullptr;
    // Just remove the last item from the last slot
    if (remove_id == -1)
    {
        selected_tool_slot = &bag_tools_data.Slots.Last();
        check(selected_tool_slot->ToolsInfo.Num() > 0); // We remove empty slots. Count as error if this happens.
        remove_id = selected_tool_slot->ToolsInfo.Pop().ToolId;
    }
    else // Or search for the specified item ID in all the slots.
    {
        for (auto&& slot : bag_tools_data.Slots)
        {
            check(slot.ToolsInfo.Num() > 0); // We remove empty slots. Count as error if this happens.
            int const removed_count = slot.ToolsInfo.RemoveAll([remove_id](FBagToolInfo& tool_info) { return tool_info.ToolId == remove_id; });
            check(removed_count <= 1); // Should only ever have unique ids
            // Early out when we find the item.
            if (removed_count > 0)
            {
                selected_tool_slot = &slot;
                break;
            }
        }
    }

    // When searching for a specific ID we might not actually
    // have it (ideally shouldn't happen), hence the slot could not be found.
    if (selected_tool_slot == nullptr)
    {
        UE_LOG(LogInventorySystem, Display, TEXT("Can't remove item [%s] from bag [%s]. Tool with ID [%d] not found."), *tool_data->GetPathName(), *GetPathName(), remove_id);
        return false;
    }

    // Check for empty slot and remove it
    bool bSlotRemoved = false;
    const int32 removed_slot_id = selected_tool_slot->Id; // Save the id in case we remove the slot data.
    if (selected_tool_slot->ToolsInfo.Num() == 0)
    {
        bag_tools_data.Slots.RemoveSingleSwap(*selected_tool_slot);
        UE_LOG(LogInventorySystem, Verbose, TEXT("Removed one tool slot [type: %s] from bag [%s]."), *tool_data->GetPathName(), *GetPathName(), remove_id);
        --Tools.UsedSlots;
        slot_ids_pool.Push(removed_slot_id);
        bSlotRemoved = true;
    }
    --bag_tools_data.ToolQuantity;
    // Remove mappings when we don't have any more resources of this type
    if (bag_tools_data.ToolQuantity == 0)
    {
        UE_LOG(LogInventorySystem, Verbose, TEXT("Removed tool mapping [type: %s] from bag [%s]."), *tool_data->GetPathName(), *GetPathName(), remove_id);
        Tools.Data.Remove(tool_data);
    }

    if (bSlotRemoved) OnToolSlotRemoved.Broadcast(this, tool_data, removed_slot_id, {removed_slot_id});
    else OnToolSlotUpdated.Broadcast(this, tool_data, selected_tool_slot->Id, *selected_tool_slot);
    return true;
}

bool UInventoryBagComponent::isValidItemData(UItemData* item_data) const
{
    if (!IsValid(item_data))
    {
        UE_LOG(LogInventorySystem, Error, TEXT("Invalid item data for bag [%s]"), *GetPathName());
        return false;
    }
    return true;
}

bool UInventoryBagComponent::hasValidItemLimits(UItemData* item_data) const
{
    if (!IsValid(item_data))
    {
        UE_LOG(LogInventorySystem, Error, TEXT("Invalid bag limits [Bag: %s] for null/invalid item_data."), *GetPathName());
        return false;
    }
    if (!BagProperties->Limits.Contains(item_data))
    {
        UE_LOG(LogInventorySystem, Error, TEXT("Invalid bag limits [Bag: %s] for item: %s"), *GetPathName(), *item_data->GetPathName());
        return false;
    }

    // Check if we've completed loading.
    checkf(limits_stream_handle.IsValid() && !limits_stream_handle->WasCanceled(), TEXT("Streaming handle should not be invalid since we start streaming in on beginplay."));
    if (limits_stream_handle->IsLoadingInProgress()) limits_stream_handle->WaitUntilComplete();
    checkf(IsValid(BagProperties->Limits[item_data].Get()), TEXT("Asset should be loaded by now."));

    // You usually wouldn't have items with 0 max quantity.
    if (BagProperties->Limits[item_data]->MaxQuantity <= 0)
    {
        UE_LOG(LogInventorySystem, Display, TEXT("Can't add item [%s] to bag [%s]. Max ResourceQuantity = 0"), *item_data->GetPathName(), *GetPathName());
        return false;
    }
    return true;
}

bool UInventoryBagComponent::hasAvailableIds() const
{
    if (item_ids_pool.Num() == 0)
    {
        UE_LOG(LogInventorySystem, Display, TEXT("Trying to add item with no more available ids for the bag [%s]"), *GetPathName());
        return false;
    }
    return true;
}

void UInventoryBagComponent::streamInLimits()
{
    if (!IsValid(BagProperties))
    {
        UE_LOG(LogInventorySystem, Error, TEXT("Invalid bag properties. [Bag: %s]"), *this->GetPathName());
        return;
    }
    if (!UAssetManager::IsValid())
    {
        UE_LOG(LogInventorySystem, Error, TEXT("Asset manager unavailable, can't stream in required assets"));
        return;
    }

    TArray<FSoftObjectPath> stream_in_assets;
    for (auto&& limit : BagProperties->Limits)
    {
        stream_in_assets.Add(limit.Key.ToSoftObjectPath());
        stream_in_assets.Add(limit.Value.ToSoftObjectPath());
    }
    if (stream_in_assets.Num() == 0)
    {
        UE_LOG(LogInventorySystem, Warning, TEXT("No assets found in bag properties limits to stream. [Bag: %s]"), *this->GetPathName());
        return;
    }

    limits_stream_handle = UAssetManager::GetStreamableManager().RequestAsyncLoad(stream_in_assets,
                                                                                  FStreamableDelegate::CreateUObject(this, &UInventoryBagComponent::handleLimitsStreamedInCompleted),
                                                                                  FStreamableManager::AsyncLoadHighPriority);
}

void UInventoryBagComponent::handleLimitsStreamedInCompleted() const
{
    UE_LOG(LogInventorySystem, Log, TEXT("Completed loading of bag props limits objects. [Bag: %s]"), *this->GetPathName());
}
