// Copyright SygiFox 2020 - Simone Di Gravio dig@sygifox.com

#pragma once

#include "Tool.h"

UToolComponent::UToolComponent()
{
    ItemData = CreateDefaultSubobject<UToolData>(TEXT("ToolData"));
    ItemData->Category = EItemCategory::Tool;
    initToolDurability();
}

EPickupBehavior UToolComponent::getPickupBehavior_Implementation()
{
    return EPickupBehavior::UseItemComponent;
}

void UToolComponent::initToolDurability()
{
    UToolData* tool_data = Cast<UToolData>(ItemData);
    if (!IsValid(tool_data)) return;
    Durability = tool_data->MaxDurability;
}

void UToolComponent::PostLoad()
{
    Super::PostLoad();
    initToolDurability();
}

#if UE_EDITOR
void UToolComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);
    initToolDurability();
}
#endif
