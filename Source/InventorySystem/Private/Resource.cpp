// Copyright SygiFox 2020 - Simone Di Gravio dig@sygifox.com

#pragma once

#include "Resource.h"

UResourceComponent::UResourceComponent()
{
    ItemData = CreateDefaultSubobject<UResourceData>(TEXT("ResourceData"));
    ItemData->Category = EItemCategory::Resource;
}

EPickupBehavior UResourceComponent::getPickupBehavior_Implementation()
{
    return EPickupBehavior::UseDataAndDestroyActor;
}
