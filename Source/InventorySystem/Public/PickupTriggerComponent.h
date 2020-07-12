// Copyright SygiFox 2020 - Simone Di Gravio dig@sygifox.com

#pragma once

#include "CoreMinimal.h"
#include "Pickable.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "PickupTriggerComponent.generated.h"

class UPickupTriggerComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPickableEventDelegate, TScriptInterface<IPickable>, pickable, UPickupTriggerComponent*, trigger);

USTRUCT(BlueprintType)
struct FFindPickableResult
{
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
    AActor* Actor;
    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
    TScriptInterface<IPickable> Pickable;
};

/**
 * Keeps track of any pickable items entering the area of this component.
 * Provides events for when new pickables enter/exit the trigger and utility functions to find the closest one.
 */
UCLASS(Blueprintable, BlueprintType, ClassGroup=(Inventory,Pickup), meta = (BlueprintSpawnableComponent))
class INVENTORYSYSTEM_API UPickupTriggerComponent : public UBoxComponent
{
    GENERATED_BODY()

public:

    UPROPERTY(BlueprintReadOnly, EditAnywhere)
    bool bAutoUpdateClosestPickable = true;
    UPROPERTY(BlueprintCallable, BlueprintAssignable)
    FPickableEventDelegate OnClosestPickableUpdated;
    UPROPERTY(BlueprintCallable, BlueprintAssignable)
    FPickableEventDelegate OnPickableTriggerEnter;
    UPROPERTY(BlueprintCallable, BlueprintAssignable)
    FPickableEventDelegate OnPickableTriggerExit;

protected:

    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
    TArray<TScriptInterface<IPickable>> available_pickables;
    UPROPERTY(BlueprintReadWrite, VisibleAnywhere)
    TScriptInterface<IPickable> closest_pickable;

public:

    UPickupTriggerComponent();
    virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
    virtual void BeginPlay() override;
    void setAutoUpdateClosestPickable(bool bEnabled);
    /**
     * @return Closest pickable to the trigger, or nullptr if no pickable is available.
     */
    UFUNCTION(BlueprintCallable)
    TScriptInterface<IPickable> getClosestPickable();

protected:

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
    void closestPickableUpdated(const TScriptInterface<IPickable>& pickable);
    UFUNCTION()
    void handleTriggerBeginOverlap(UPrimitiveComponent* overlapped_component, AActor* other_actor, UPrimitiveComponent* other_comp, int32 other_body_index, bool b_from_sweep, const FHitResult& sweep_result);
    UFUNCTION()
    void handleTriggerEndOverlap(UPrimitiveComponent* overlapped_component, AActor* other_actor, UPrimitiveComponent* other_comp, int32 other_body_index);
};
