#include "DCPirateCast.h"
#include "DCAceFace.h"
#include "DCVoice.h"
#include "DCCrew.h"
#include "DCPlayer.h"
#include "Animation/AnimationAsset.h"
#include "Components/CapsuleComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/LODSyncComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GroomComponent.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

bool UDCPirateCastSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	return GetDefault<UDCPirateCastSettings>()->bEnabled && Super::ShouldCreateSubsystem(Outer);
}

bool UDCPirateCastSubsystem::DoesSupportWorldType(EWorldType::Type Type) const
{
	return Type == EWorldType::Game || Type == EWorldType::PIE;
}

TStatId UDCPirateCastSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UDCPirateCastSubsystem, STATGROUP_Tickables);
}

bool UDCPirateCastSubsystem::Bind(AActor* Owner, USkeletalMeshComponent* Driver,
	const TSoftClassPtr<AActor>& Asset, bool bPlayer)
{
	Attempted.Add(Owner);
	UClass* VisualClass = Asset.LoadSynchronous();
	if (!VisualClass || !Driver || !Driver->GetSkeletalMeshAsset())
	{
		UE_LOG(LogTemp, Warning, TEXT("Pirate cast: keeping original appearance for %s (missing actor or pose driver)"), *Owner->GetName());
		return false;
	}
	UChildActorComponent* Visual = NewObject<UChildActorComponent>(Owner, TEXT("PirateAppearance"));
	Owner->AddInstanceComponent(Visual);
	Visual->SetupAttachment(Driver);
	Visual->RegisterComponent();
	Visual->SetChildActorClass(VisualClass);
	AActor* Actor = Visual->GetChildActor();
	USkeletalMeshComponent* Body = nullptr;
	if (Actor)
	{
		TInlineComponentArray<USkeletalMeshComponent*> Meshes(Actor);
		for (USkeletalMeshComponent* Mesh : Meshes)
		{
			if (Mesh->GetName().Equals(TEXT("Body"), ESearchCase::IgnoreCase)) { Body = Mesh; break; }
		}
	}
	if (!Body || !Body->GetSkeletalMeshAsset())
	{
		UE_LOG(LogTemp, Error, TEXT("Pirate cast: %s has no assembled Body mesh; retaining original"), *Asset.ToString());
		Visual->DestroyComponent();
		return false;
	}
	Actor->SetOwner(Owner);
	Actor->SetActorEnableCollision(false);
	const FString GroomFolder = FPaths::GetPath(Asset.ToSoftObjectPath().GetLongPackageName()) / TEXT("Grooms");
	UGroomAsset* HairAsset = LoadObject<UGroomAsset>(nullptr, *(GroomFolder / TEXT("Gr_Hair_Strands.Gr_Hair_Strands")));
	UGroomBindingAsset* HairBinding = LoadObject<UGroomBindingAsset>(nullptr, *(GroomFolder / TEXT("GB_Hair_Strands.GB_Hair_Strands")));
	if (HairAsset && HairBinding)
	{
		TInlineComponentArray<UGroomComponent*> Grooms(Actor);
		for (UGroomComponent* Groom : Grooms)
		{
			if (Groom->GetName() == TEXT("Hair")) { Groom->SetGroomAsset(HairAsset, HairBinding); }
		}
	}
	// These vendor hairstyles supply strands rather than cards. Use the restored
	// strand LODs with distance reduction while the body retains Medium-quality LODs.
	if (ULODSyncComponent* Sync = Actor->FindComponentByClass<ULODSyncComponent>())
	{
		FLODMappingData HairMapping;
		HairMapping.Mapping = { 1, 3, 5 };
		Sync->CustomLODMapping.Add(TEXT("Hair"), HairMapping);
		Sync->RefreshSyncComponents();
	}
	Driver->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	Driver->SetVisibility(false, false);
	Driver->SetCastShadow(false);
	Body->SetLeaderPoseComponent(Driver, true, true);
	Body->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
	if ((GetDefault<ADCVoice>()->bUseACE || FParse::Param(FCommandLine::Get(), TEXT("DCEnableACE"))) && !FParse::Param(FCommandLine::Get(), TEXT("DCNoACE")))
	{
		DCAceFace::Bind(Actor);
	}
	FDCPirateVisual Binding;
	Binding.Owner = Owner;
	Binding.Visual = Visual;
	Binding.Driver = Driver;
	Binding.Body = Body;
	Binding.bPlayer = bPlayer;
	if (bPlayer)
	{
		Binding.Idle = LoadObject<UAnimationAsset>(nullptr, TEXT("/Game/Brigantine/Demo/Characters/Mannequins/Animations/Quinn/MF_Idle.MF_Idle"));
		Binding.Walk = LoadObject<UAnimationAsset>(nullptr, TEXT("/Game/Brigantine/Demo/Characters/Mannequins/Animations/Quinn/MF_Walk_Fwd.MF_Walk_Fwd"));
		if (Binding.Idle) { Driver->PlayAnimation(Binding.Idle, true); }
	}
	Bindings.Add(Binding);
	UE_LOG(LogTemp, Log, TEXT("Pirate cast: bound %s -> %s, body=%s"), *Owner->GetName(), *Asset.ToString(), *Body->GetName());
	return true;
}

void UDCPirateCastSubsystem::Tick(float DeltaTime)
{
	ScanTimer -= DeltaTime;
	if (ScanTimer <= 0.f)
	{
		ScanTimer = .5f;
		const UDCPirateCastSettings* Settings = GetDefault<UDCPirateCastSettings>();
		for (TActorIterator<ADCCrewMember> It(GetWorld()); It; ++It)
		{
			ADCCrewMember* Crew = *It;
			if (Attempted.Contains(Crew) || Crew->CrewName.IsEmpty()) { continue; }
			const TSoftClassPtr<AActor>* Asset = nullptr;
			if (Crew->CrewName == TEXT("Rex")) { Asset = &Settings->Finch; }
			else if (Crew->CrewName == TEXT("Duncan")) { Asset = &Settings->Marlow; }
			else if (Crew->CrewName == TEXT("Ruby")) { Asset = &Settings->Odessa; }
			if (Asset) { Bind(Crew, Crew->FindComponentByClass<USkeletalMeshComponent>(), *Asset, false); }
		}
		for (TActorIterator<ADCPlayerCharacter> It(GetWorld()); It; ++It)
		{
			ADCPlayerCharacter* Player = *It;
			if (Attempted.Contains(Player)) { continue; }
			USkeletalMeshComponent* Driver = Player->GetMesh();
			if (!Driver->GetSkeletalMeshAsset())
			{
				Driver->SetSkeletalMeshAsset(LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Brigantine/Demo/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple")));
				Driver->SetRelativeLocation(FVector(0, 0, -Player->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()));
				Driver->SetRelativeRotation(FRotator(0, -90, 0));
			}
			Bind(Player, Driver, Settings->Captain, true);
		}
	}
	for (FDCPirateVisual& Binding : Bindings)
	{
		if (!IsValid(Binding.Owner) || !IsValid(Binding.Visual) || !Binding.bPlayer) { continue; }
		ADCPlayerCharacter* Player = CastChecked<ADCPlayerCharacter>(Binding.Owner);
		const bool bWalking = Player->GetCharacterMovement()->Velocity.SizeSquared2D() > 25.f;
		if (bWalking != Binding.bWalking)
		{
			Binding.bWalking = bWalking;
			if (UAnimationAsset* Anim = bWalking ? Binding.Walk.Get() : Binding.Idle.Get()) { Binding.Driver->PlayAnimation(Anim, true); }
		}
		APlayerController* PC = Cast<APlayerController>(Player->GetController());
		const bool bHideHead = PC && PC->GetViewTarget() == Player;
		if (bHideHead != Binding.bHeadHidden)
		{
			Binding.bHeadHidden = bHideHead;
			AActor* Actor = Binding.Visual->GetChildActor();
			if (!Actor) { continue; }
			// The purchased full-body outfits include hat brims and shoulder accessories at eye level.
			// Show the complete captain for external cameras; keep the existing unobstructed first-person view.
			TInlineComponentArray<UPrimitiveComponent*> Meshes(Actor);
			for (UPrimitiveComponent* Mesh : Meshes)
			{
				Mesh->SetHiddenInGame(bHideHead);
				Mesh->SetCastHiddenShadow(true);
			}
		}
	}
}
