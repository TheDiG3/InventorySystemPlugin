// Copyright SygiFox 2020 - Simone Di Gravio dig@sygifox.com

#include "Crafting/CraftingUtils.h"

TMap<UItemData*, FRecipesSet> UCraftingUtils::generateRecipesForItemMappings(UCraftablesCollection* craftables_collection)
{
    if (!IsValid(craftables_collection))
    {
        UE_LOG(LogInventorySystem, Error, TEXT("Invalid craftables collection."))
        return {};
    }
    TMap<UItemData*, FRecipesSet> mappings;
    for (auto&& craftable : craftables_collection->Craftables)
    {
        for (auto&& recipe_requirement : craftable->Requirements)
        {
            if (!mappings.Contains(recipe_requirement.Item)) mappings.Add(recipe_requirement.Item); // Add item keys we don't have yet.
            mappings[recipe_requirement.Item].Recipes.Add(craftable);
        }
    }
    return mappings;
}

TArray<UCraftingRecipe*> UCraftingUtils::getCraftableRecipesForAvailableItems(TMap<UItemData*, int32> available_items, UCraftablesCollection* craftables_collection)
{
    if (!IsValid(craftables_collection))
    {
        UE_LOG(LogInventorySystem, Error, TEXT("Invalid craftables collection."))
        return {};
    }

    TArray<UCraftingRecipe*> available_recipes;
    // For each recipe we want to check whether we have all necessary item requirements and quantity.
    for (auto&& recipe : craftables_collection->Craftables)
    {
        bool bHasAllRequirements = true;
        for (auto&& requirement : recipe->Requirements)
        {
            // Early out as soon as a requirement is not satisfied.
            if (!available_items.Contains(requirement.Item) || requirement.Quantity > available_items[requirement.Item])
            {
                bHasAllRequirements = false;
                break;
            }
        }
        if (bHasAllRequirements) available_recipes.Add(recipe); // Add the recipe if we have everything needed for it
    }

    return available_recipes;
}

TMap<UItemData*, int32> UCraftingUtils::generateAvailableItemsFromResources(const FBagResources& in_resources)
{
    TMap<UItemData*, int32> available_items;
    for (auto&& resource : in_resources.Data)
    {
        available_items.Add(resource.Key, resource.Value.ResourceQuantity);
    }
    return available_items;
}
