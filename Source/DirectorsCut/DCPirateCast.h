#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DCPirateCast.generated.h"

class UChildActorComponent;
class USkeletalMeshComponent;
class UAnimationAsset;

/** Presentation-only binding. Existing crew orders, stations and player movement keep ownership. */
UCLASS(Config=Game, DefaultConfig)
class UDCPirateCastSettings : public UObject
{
	GENERATED_BODY()
public:
	UPROPERTY(Config) bool bEnabled = false;
	UPROPERTY(Config) TSoftClassPtr<AActor> Captain;
	UPROPERTY(Config) TSoftClassPtr<AActor> Finch;
	UPROPERTY(Config) TSoftClassPtr<AActor> Marlow;
	UPROPERTY(Config) TSoftClassPtr<AActor> Odessa;
};

USTRUCT()
struct FDCPirateVisual
{
	GENERATED_BODY()
	UPROPERTY() TObjectPtr<AActor> Owner;
	UPROPERTY() TObjectPtr<UChildActorComponent> Visual;
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> Driver;
	UPROPERTY() TObjectPtr<USkeletalMeshComponent> Body;
	UPROPERTY() TObjectPtr<UAnimationAsset> Idle;
	UPROPERTY() TObjectPtr<UAnimationAsset> Walk;
	bool bPlayer = false;
	bool bWalking = false;
	bool bHeadHidden = false;
};

UCLASS()
class UDCPirateCastSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual bool DoesSupportWorldType(EWorldType::Type WorldType) const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
private:
	bool Bind(AActor* Owner, USkeletalMeshComponent* Driver, const TSoftClassPtr<AActor>& Asset, bool bPlayer);
	UPROPERTY() TArray<FDCPirateVisual> Bindings;
	TSet<TWeakObjectPtr<AActor>> Attempted;
	float ScanTimer = 0.f;
};
