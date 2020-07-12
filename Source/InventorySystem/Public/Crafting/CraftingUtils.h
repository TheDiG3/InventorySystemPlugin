// Copyright SygiFox 2020 - Simone Di Gravio dig@sygifox.com

#pragma once

#include "CoreMinimal.h"
#include "CraftingTypes.h"
#include "InventoryBagComponent.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CraftingUtils.generated.h"

/**
 * Holds a set of recipes.
 * Needed due to BP functions not directly accepting nested containers.
 */
USTRUCT(BlueprintType)
struct FRecipesSet
{
    GENERATED_BODY()

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Crafting")
    TSet<UCraftingRecipe*> Recipes;
};

/**
 * Utility functions to work with the crafting system.
 */
UCLASS()
class INVENTORYSYSTEM_API UCraftingUtils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

    /**
     * Generates a map that lets you find out what recipes are available for each item type based on the given craftables collection.
     * You'd usually generate this once or any time you update the craftables collection.
     * @param craftables_collection List of all available recipes for which to generate the mappings.
     * @return Mappings for each item type to recipes that require it.
     */
    UFUNCTION(BlueprintCallable, Category="Crafting")
    static TMap<UItemData*, FRecipesSet> generateRecipesForItemMappings(UCraftablesCollection* craftables_collection);

    /**
     * Examines what craftables you can currently create based on the items you have available.
     * @param available_items Mapping in the form of item type - available quantity.
     * @param craftables_collection Collection of all possible recipes.
     * @return Array of currently craftable recipes.
     */
    UFUNCTION(BlueprintCallable, Category="Crafting")
    static TArray<UCraftingRecipe*> getCraftableRecipesForAvailableItems(TMap<UItemData*, int32> available_items, UCraftablesCollection* craftables_collection);

    /**
     * Helper function to generate the item-quantity map needed for getCraftableRecipesForAvailableItems.
     * @param in_resources Resources you want to generate the map for.
     * @return Map of item type -> quantity.
     */
    UFUNCTION(BlueprintCallable, Category="Crafting")
    static TMap<UItemData*, int32> generateAvailableItemsFromResources(const FBagResources& in_resources);
};
