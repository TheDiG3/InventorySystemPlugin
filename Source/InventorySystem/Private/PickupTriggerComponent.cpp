// Copyright SygiFox 2020 - Simone Di Gravio dig@sygifox.com

#include "PickupTriggerComponent.h"

#include <limits>

bool findPickableFromActor(AActor* actor, FFindPickableResult& out_pickable_result)
{
    check(IsValid(actor));
    if (actor->Implements<UPickable>())
    {
        out_pickable_result = {actor, actor};
        return true; // Direct implementation
    }
    auto pickables = actor->GetComponentsByInterface(UPickable::StaticClass()); // Pickable implemented in components maybe.
    if (pickables.Num() == 0) return false;
    out_pickable_result = {actor, pickables[0]};
    return true;
}

UPickupTriggerComponent::UPickupTriggerComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.2f;
    SetGenerateOverlapEvents(true);
}

void UPickupTriggerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    const TScriptInterface<IPickable> new_closest = getClosestPickable();
    if (new_closest != closest_pickable)
    {
        closest_pickable = new_closest;
        closestPickableUpdated(closest_pickable);
    }
}

void UPickupTriggerComponent::BeginPlay()
{
    Super::BeginPlay();
    OnComponentBeginOverlap.AddDynamic(this, &UPickupTriggerComponent::handleTriggerBeginOverlap);
    OnComponentEndOverlap.AddDynamic(this, &UPickupTriggerComponent::handleTriggerEndOverlap);
}

void UPickupTriggerComponent::setAutoUpdateClosestPickable(bool bEnabled)
{
    bAutoUpdateClosestPickable = bEnabled;
    PrimaryComponentTick.SetTickFunctionEnable(bEnabled);
}

TScriptInterface<IPickable> UPickupTriggerComponent::getClosestPickable()
{
    if (available_pickables.Num() == 0) return nullptr;

    FVector const location = this->GetComponentLocation();
    float min_distance_sq = closest_pickable != nullptr ? FVector::DistSquared(closest_pickable->Execute_getPickableLocation(closest_pickable.GetObject()), location) : std::numeric_limits<float>::max();
    TScriptInterface<IPickable> closest = nullptr;
    for (auto&& pickable : available_pickables)
    {
        float const dist_sq = FVector::DistSquared(location, pickable->Execute_getPickableLocation(pickable.GetObject()));
        if (dist_sq <= min_distance_sq)
        {
            min_distance_sq = dist_sq;
            closest = pickable;
        }
    }
    return closest;
}

void UPickupTriggerComponent::closestPickableUpdated_Implementation(const TScriptInterface<IPickable>& pickable)
{
    OnClosestPickableUpdated.Broadcast(pickable, this);
}

void UPickupTriggerComponent::handleTriggerBeginOverlap(UPrimitiveComponent* overlapped_component, AActor* other_actor, UPrimitiveComponent* other_comp, int32 other_body_index, bool b_from_sweep, const FHitResult& sweep_result)
{
    FFindPickableResult pickable_result;
    if (!findPickableFromActor(other_actor, pickable_result)) return;
    available_pickables.Add(pickable_result.Pickable);
    OnPickableTriggerEnter.Broadcast(pickable_result.Pickable, this);
}

void UPickupTriggerComponent::handleTriggerEndOverlap(UPrimitiveComponent* overlapped_component, AActor* other_actor, UPrimitiveComponent* other_comp, int32 other_body_index)
{
    FFindPickableResult pickable_result;
    if (!findPickableFromActor(other_actor, pickable_result)) return;
    available_pickables.Remove(pickable_result.Pickable);
    OnPickableTriggerExit.Broadcast(pickable_result.Pickable, this);
}
