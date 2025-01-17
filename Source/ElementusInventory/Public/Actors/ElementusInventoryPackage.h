// Author: Lucas Vilas-Boas
// Year: 2023
// Repo: https://github.com/lucoiso/UEElementusInventory

#pragma once

#include <CoreMinimal.h>
#include <GameFramework/Actor.h>
#include <Components/ElementusInventoryComponent.h>
#include "ElementusInventoryPackage.generated.h"

UCLASS(Category = "Elementus Inventory | Classes")
class ELEMENTUSINVENTORY_API AElementusInventoryPackage : public AActor
{
    GENERATED_BODY()

public:
    explicit AElementusInventoryPackage(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    /* The inventory component of this package actor */
    UPROPERTY(Replicated, EditDefaultsOnly, BlueprintReadOnly, Category = "Elementus Inventory")
    UElementusInventoryComponent* PackageInventory;

    /* Put a item in this package */
    UFUNCTION(BlueprintCallable, Category = "Elementus Inventory")
    void PutItemIntoPackage(const TArray<FElementusItemInfo> ItemInfo, UElementusInventoryComponent* FromInventory);

    /* Get a item from this package */
    UFUNCTION(BlueprintCallable, Category = "Elementus Inventory")
    void GetItemFromPackage(const TArray<FElementusItemInfo> ItemInfo, UElementusInventoryComponent* ToInventory);

    /* Set this package to auto destroy when its empty */
    UFUNCTION(BlueprintCallable, Category = "Elementus Inventory")
    void SetDestroyOnEmpty(const bool bDestroy);

    /* Will this package auto destroy when empty? */
    UFUNCTION(BlueprintPure, Category = "Elementus Inventory")
    bool GetDestroyOnEmpty() const;

protected:
    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

    /* Should this package auto destroy when empty? */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Elementus Inventory", meta = (Getter = "GetDestroyOnEmpty", Setter = "SetDestroyOnEmpty"))
    bool bDestroyWhenInventoryIsEmpty;

    /* Destroy this package (Call Destroy()) */
    UFUNCTION(BlueprintNativeEvent, Category = "Elementus Inventory")
    void BeginPackageDestruction();
};
