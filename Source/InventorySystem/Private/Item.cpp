// Copyright SygiFox 2020 - Simone Di Gravio dig@sygifox.com

#include "Item.h"
#include "InventoryBagComponent.h"
#include "GameFramework/Actor.h"

UItemComponent::UItemComponent()
{
    ItemData = CreateDefaultSubobject<UItemData>(TEXT("ItemData"));
    ItemData->Category = EItemCategory::None;
}

AActor* UItemComponent::getPickableActor_Implementation()
{
    return this->GetOwner();
}

UItemData* UItemComponent::getItemData_Implementation()
{
    return ItemData;
}

UItemComponent* UItemComponent::getItemComponent_Implementation()
{
    return this;
}

FVector UItemComponent::getPickableLocation_Implementation()
{
    return GetOwner()->GetActorLocation();
}

EPickupBehavior UItemComponent::getPickupBehavior_Implementation()
{
    return EPickupBehavior::UseDataOnly;
}

TScriptInterface<IPickable> UItemComponent::getPickable_Implementation()
{
    return this;
}

void UItemComponent::OnItemPickedUp_Implementation(UInventoryBagComponent* owning_bag)
{
    check(owning_bag != nullptr);
    UE_LOG(LogInventorySystem, Verbose, TEXT("Item [%s] picked up by %s."), *this->GetOwner()->GetName(), *owning_bag->GetName());
    OwningBag = owning_bag;
}

void UItemComponent::OnItemDropped_Implementation(UInventoryBagComponent* owning_bag)
{
    check(owning_bag != nullptr);
    UE_LOG(LogInventorySystem, Verbose, TEXT("Item [%s] dropped from %s."), *this->GetOwner()->GetName(), *owning_bag->GetName());
    OwningBag = nullptr;
}
