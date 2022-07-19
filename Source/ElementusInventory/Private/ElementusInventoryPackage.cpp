// Author: Lucas Vilas-Boas
// Year: 2022
// Repo: https://github.com/lucoiso/UEElementusInventory

#include "ElementusInventoryPackage.h"
#include "ElementusInventoryComponent.h"
#include "ElementusInventoryFunctions.h"

AElementusInventoryPackage::AElementusInventoryPackage(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer),
	  bDestroyWhenInventoryIsEmpty(false)
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;
	PrimaryActorTick.bStartWithTickEnabled = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));

	PackageMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PackageMesh"));
	PackageMesh->SetupAttachment(RootComponent);

	PackageInventory = CreateDefaultSubobject<UElementusInventoryComponent>(TEXT("PackageInventory"));
	PackageInventory->SetIsReplicated(true);
}

void AElementusInventoryPackage::PutItemIntoPackage(TMap<FPrimaryAssetId, int32>& ItemInfo,
                                                    UElementusInventoryComponent* FromInventory) const
{
	UElementusInventoryFunctions::TradeElementusItem(ItemInfo, FromInventory, PackageInventory);
}

void AElementusInventoryPackage::GetItemFromPackage(TMap<FPrimaryAssetId, int32>& ItemInfo,
                                                    UElementusInventoryComponent* ToInventory) const
{
	UElementusInventoryFunctions::TradeElementusItem(ItemInfo, PackageInventory, ToInventory);
}

void AElementusInventoryPackage::SetDestroyOnEmpty(const bool bDestroy)
{
	if (bDestroyWhenInventoryIsEmpty == bDestroy)
	{
		return;
	}

	bDestroyWhenInventoryIsEmpty = bDestroy;
	FElementusInventoryEmpty& Delegate = PackageInventory->OnInventoryEmpty;

	if (const bool bIsAlreadyBound =
			Delegate.IsAlreadyBound(this, &AElementusInventoryPackage::BeginPackageDestruction);
		bDestroy && !bIsAlreadyBound)
	{
		Delegate.AddDynamic(this, &AElementusInventoryPackage::BeginPackageDestruction);
	}
	else if (!bDestroy && bIsAlreadyBound)
	{
		Delegate.RemoveDynamic(this, &AElementusInventoryPackage::BeginPackageDestruction);
	}
}

bool AElementusInventoryPackage::GetDestroyOnEmpty() const
{
	return bDestroyWhenInventoryIsEmpty;
}

void AElementusInventoryPackage::BeginPlay()
{
	Super::BeginPlay();

	if (bDestroyWhenInventoryIsEmpty)
	{
		PackageInventory->OnInventoryEmpty.AddDynamic(this, &AElementusInventoryPackage::BeginPackageDestruction);
	}
}

void AElementusInventoryPackage::BeginPackageDestruction_Implementation()
{
	// Check if this option is still active
	if (bDestroyWhenInventoryIsEmpty)
	{
		Destroy();
	}
	else
	{
		UE_LOG(LogElementusInventory, Warning,
		       TEXT("ElementusInventory - %s: Package %s was not destroyed because the "
			       "option 'bDestroyWhenInventoryIsEmpty' was disabled"),
		       *FString(__func__), *GetName());
	}
}