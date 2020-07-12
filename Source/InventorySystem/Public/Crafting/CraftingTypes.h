// Copyright SygiFox 2020 - Simone Di Gravio dig@sygifox.com

#pragma once

#include "Item.h"
#include "Engine/DataAsset.h"

#include "CraftingTypes.generated.h"

/**
 * Required item type and quantity.
 */
USTRUCT(BlueprintType)
struct INVENTORYSYSTEM_API FRecipeRequirement
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crafting")
    UItemData* Item;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crafting")
    int32 Quantity = 1;
};

/**
 * Recipe for a single craftable item.
 */
UCLASS(BlueprintType, Blueprintable)
class INVENTORYSYSTEM_API UCraftingRecipe : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crafting")
    UItemData* CraftedItem;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crafting")
    int32 QuantityPerCraft = 1;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crafting")
    TArray<FRecipeRequirement> Requirements;
};

/**
 * Contains a list of all available craftable recipes.
 */
UCLASS(BlueprintType, Blueprintable)
class INVENTORYSYSTEM_API UCraftablesCollection : public UPrimaryDataAsset
{
    GENERATED_BODY()
public:

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Crafting")
    TArray<UCraftingRecipe*> Craftables;
};
