#include "DCPlayer.h"
#include "DCRideDirector.h"
#include "DCTroubles.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"

void ADCPlayerCharacter::BuildRag()
{
	Rag = NewObject<UProceduralMeshComponent>(this, TEXT("CaptainRag"));
	Rag->SetupAttachment(Camera);
	Rag->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Rag->SetCastShadow(false);
	Rag->SetOnlyOwnerSee(true);
	Rag->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
	Camera->SetEnableFirstPersonScale(true);
	Camera->SetFirstPersonScale(.5f);
	Rag->RegisterComponent();
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Stone.M_Stone"));
	auto Material = [this, Base](FLinearColor Color)
	{
		UMaterialInstanceDynamic* M = Base ? UMaterialInstanceDynamic::Create(Base, this) : nullptr;
		if (M) { M->SetVectorParameterValue(TEXT("Color"), Color); M->SetVectorParameterValue(TEXT("Glow"), FLinearColor::Black); }
		return M;
	};
	// Reuse the ship pack's woven fabric on private runtime instances. The old
	// flat stone material read as paper and plastic this close to the camera.
	UMaterialInterface* ClothBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Brigantine/Material/Root/M_2.M_2"));
	UTexture* FabricColor = LoadObject<UTexture>(nullptr, TEXT("/Game/Brigantine/Texture/Tiling/T_Fabric_1_BaseColor.T_Fabric_1_BaseColor"));
	UTexture* FabricNormal = LoadObject<UTexture>(nullptr, TEXT("/Game/Brigantine/Texture/Tiling/T_Fabric_1_Normal.T_Fabric_1_Normal"));
	UTexture* FabricMasks = LoadObject<UTexture>(nullptr, TEXT("/Game/Brigantine/Texture/Tiling/T_Fabric_1_OcclusionRoughnessMetallic.T_Fabric_1_OcclusionRoughnessMetallic"));
	auto Cloth = [this, ClothBase, FabricColor, FabricNormal, FabricMasks, &Material](FLinearColor Color, float Tiling)
	{
		if (!ClothBase || !FabricColor || !FabricNormal || !FabricMasks) { return Material(Color); }
		UMaterialInstanceDynamic* M = UMaterialInstanceDynamic::Create(ClothBase, this);
		M->SetTextureParameterValue(TEXT("1_Base_Texture"), FabricColor);
		M->SetTextureParameterValue(TEXT("2_Masks (Occ,Rough,Met)"), FabricMasks);
		M->SetTextureParameterValue(TEXT("3_Normal"), FabricNormal);
		M->SetVectorParameterValue(TEXT("1_Tint_base_texture"), Color);
		M->SetScalarParameterValue(TEXT("1_Brightness"), 1.f);
		M->SetScalarParameterValue(TEXT("Tiling"), Tiling);
		M->SetScalarParameterValue(TEXT("Tiling-normal"), Tiling);
		M->SetScalarParameterValue(TEXT("Roughness Min"), .8f);
		M->SetScalarParameterValue(TEXT("Roughness Max"), 1.f);
		M->SetScalarParameterValue(TEXT("Metallic Min"), 0.f);
		M->SetScalarParameterValue(TEXT("Metallic Max"), 0.f);
		M->SetScalarParameterValue(TEXT("Specular"), .1f);
		M->SetScalarParameterValue(TEXT("3_Normal_Strengh"), .7f);
		return M;
	};
	Rag->SetMaterial(0, Cloth(FLinearColor(.45f, .34f, .20f), 2.f));
	auto Grip = [this, &Material](const TCHAR* Name, FLinearColor Color)
	{
		UStaticMeshComponent* Part = NewObject<UStaticMeshComponent>(this, Name);
		Part->SetupAttachment(Camera);
		Part->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere")));
		Part->SetMaterial(0, Material(Color));
		Part->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Part->SetCastShadow(false);
		Part->SetOnlyOwnerSee(true);
		Part->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
		Part->RegisterComponent();
		return Part;
	};
	RagHand = Grip(TEXT("RagGlove"), FLinearColor(.055f, .033f, .019f));
	RagSleeve = Grip(TEXT("RagSleeve"), FLinearColor(.026f, .033f, .038f));
	UMaterialInstanceDynamic* GloveMaterial = Cloth(FLinearColor(.12f, .075f, .04f), 4.f);
	RagHand->SetMaterial(0, GloveMaterial);
	UMaterialInstanceDynamic* SleeveMaterial = Cloth(FLinearColor(.04f, .055f, .065f), 5.f);
	RagSleeve->SetMaterial(0, SleeveMaterial);
	// Subtle stitched ribs break up the smooth work-glove surface without adding
	// an unrelated first-person rig.
	UMaterialInstanceDynamic* StitchMaterial = Cloth(FLinearColor(.18f, .11f, .06f), 4.f);
	for (int32 Rib = 0; Rib < 3; ++Rib)
	{
		UStaticMeshComponent* Seam = Grip(*FString::Printf(TEXT("RagGloveSeam%d"), Rib), FLinearColor::Black);
		Seam->SetMaterial(0, StitchMaterial);
		Seam->AttachToComponent(RagHand, FAttachmentTransformRules::KeepRelativeTransform);
		Seam->SetRelativeLocation(FVector(-2.f, -18.f + Rib * 18.f, 44.f - FMath::Abs(Rib - 1) * 4.f));
		Seam->SetRelativeScale3D(FVector(.48f, .025f, .035f));
	}
	// Closed fingers and a curled thumb hold the gathered end of the cloth.
	for (int32 Finger = 0; Finger < 4; ++Finger)
	{
		UStaticMeshComponent* Knuckle = Grip(*FString::Printf(TEXT("RagFinger%d"), Finger), FLinearColor(.045f, .028f, .016f));
		Knuckle->SetMaterial(0, GloveMaterial);
		Knuckle->AttachToComponent(RagHand, FAttachmentTransformRules::KeepRelativeTransform);
		Knuckle->SetRelativeLocation(FVector(30.f, -31.f + Finger * 20.f, 2.f));
		Knuckle->SetRelativeScale3D(FVector(.52f, .25f, .8f));
	}
	UStaticMeshComponent* Thumb = Grip(TEXT("RagThumb"), FLinearColor(.05f, .03f, .017f));
	Thumb->SetMaterial(0, GloveMaterial);
	Thumb->AttachToComponent(RagHand, FAttachmentTransformRules::KeepRelativeTransform);
	Thumb->SetRelativeLocation(FVector(8.f, -43.f, 5.f));
	Thumb->SetRelativeRotation(FRotator(0.f, -35.f, 0.f));
	Thumb->SetRelativeScale3D(FVector(.8f, .32f, .38f));
}

bool ADCPlayerCharacter::TrySwatFire()
{
	if (SwatAge < .64f || IsAtStation() || bShipView || bTyping) { return false; }
	ADCTroubles* Troubles = ADCTroubles::Get(this);
	const ADCRideDirector* Director = ADCRideDirector::Get(this);
	if (!Troubles || !Director || !Camera) { return false; }
	const FVector DeckPosition = Director->DeckTransform.InverseTransformPosition(GetActorLocation());
	FVector Fire;
	if (!Troubles->NearestFire(DeckPosition, Fire) || FVector::Distance(Fire, DeckPosition) > 330.f) { return false; }
	const FVector Toward = (Director->DeckToWorld(Fire) - Camera->GetComponentLocation()).GetSafeNormal();
	if (FVector::DotProduct(Camera->GetForwardVector(), Toward) < .05f) { return false; }
	SwatTarget = Fire;
	FireFeedback.Reset();
	FireFeedbackTime = 0.f;
	SwatAge = 0.f;
	bSwatContact = false;
	if (!Rag) { BuildRag(); }
	return true;
}

void ADCPlayerCharacter::CancelSwat()
{
	// Changing interaction mode permanently cancels this stroke, even if the
	// player returns before its contact time. Never resume a hidden swing.
	SwatAge = FMath::Max(SwatAge, .64f);
	bSwatContact = true;
	FireFeedback.Reset();
	FireFeedbackTime = 0.f;
}

void ADCPlayerCharacter::TickRag(float DeltaSeconds)
{
	SwatAge += DeltaSeconds;
	FireFeedbackTime = FMath::Max(0.f, FireFeedbackTime - DeltaSeconds);
	if (FireFeedbackTime == 0.f) { FireFeedback.Reset(); }
	if (IsAtStation() || bShipView || bTyping) { CancelSwat(); }
	const bool bVisible = SwatAge < .62f && !IsAtStation() && !bShipView && !bTyping;
	if (Rag) { Rag->SetVisibility(bVisible); }
	if (RagHand) { RagHand->SetVisibility(bVisible, true); }
	if (RagSleeve) { RagSleeve->SetVisibility(bVisible); }
	// Contact belongs to the input stroke, not its visible animation window.
	// A delayed frame can cross contact and recovery together; consume the
	// pending hit once, with the same facing/range and cancellation checks.
	if (!bSwatContact && SwatAge >= .23f)
	{
		bSwatContact = true;
		ADCTroubles* Troubles = ADCTroubles::Get(this);
		const ADCRideDirector* Director = ADCRideDirector::Get(this);
		if (Troubles && Director)
		{
			const FVector Here = Director->DeckTransform.InverseTransformPosition(GetActorLocation());
			const FVector Toward = (Director->DeckToWorld(SwatTarget) - Camera->GetComponentLocation()).GetSafeNormal();
			if (FVector::Distance(Here, SwatTarget) <= 350.f
				&& FVector::DotProduct(Camera->GetForwardVector(), Toward) > .2f
				&& Troubles->SwatNear(SwatTarget, 100.f))
			{
				const int32 Remaining = Troubles->SwatsRemaining(SwatTarget);
				FireFeedback = Remaining > 0
					? FString::Printf(TEXT("FIRE WEAKENED   %d swat%s left"), Remaining, Remaining == 1 ? TEXT("") : TEXT("s"))
					: TEXT("FIRE OUT   +50");
				FireFeedbackTime = Remaining > 0 ? .9f : 1.5f;
				UE_LOG(LogDC, Log, TEXT("Rag contact: remaining=%d at %s"), Remaining, *SwatTarget.ToCompactString());
			}
		}
	}
	if (!bVisible || !Rag) { return; }

	// A short backswing, a fast downward stroke, then a soft recovery. The fabric
	// trails the grip and folds across a grid; it is never a single rigid card.
	const float Strike = FMath::SmoothStep(.06f, .25f, SwatAge);
	const float Recovery = FMath::SmoothStep(.30f, .62f, SwatAge);
	const float Swing = Strike * (1.f - Recovery);
	const FVector Grip(45.f + 17.f * Swing, 22.f - 23.f * Swing, -4.f - 10.f * Swing);
	RagHand->SetRelativeLocation(Grip);
	RagHand->SetRelativeRotation(FRotator(-20.f - Swing * 25.f, -20.f, 10.f));
	RagHand->SetRelativeScale3D(FVector(.085f, .085f, .065f));
	const FVector Elbow(10.f, 29.f, -33.f);
	const FVector Arm = Grip - Elbow;
	RagSleeve->SetRelativeLocation((Elbow + Grip) * .5f);
	RagSleeve->SetRelativeRotation(Arm.Rotation());
	RagSleeve->SetRelativeScale3D(FVector(Arm.Size() / 100.f, .085f, .09f));
	TArray<FVector> Verts, Normals;
	TArray<FVector2D> UV;
	TArray<int32> Tris;
	TArray<FColor> Colors;
	TArray<FProcMeshTangent> Tangents;
	constexpr int32 Across = 12, Along = 16;
	for (int32 Y = 0; Y <= Along; ++Y)
	{
		const float V = float(Y) / Along;
		for (int32 X = 0; X <= Across; ++X)
		{
			const float U = float(X) / Across;
			const float Fold = FMath::Sin(U * 17.f + V * 8.f - SwatAge * 19.f) * (1.f + 2.f * V);
			Verts.Add(Grip + FVector(5.f + V * (10.f + 10.f * Swing) + Fold, (U - .5f) * (4.f + 32.f * V) - V * 6.f, V * (18.f - 8.f * Swing) + Fold * .65f));
			UV.Add(FVector2D(U, V));
			Colors.Add(FColor(180, 160, 120));
			if (X < Across && Y < Along)
			{
				const int32 A = Y * (Across + 1) + X, B = A + Across + 1;
				Tris.Append({A, A + 1, B, A + 1, B + 1, B});
			}
		}
	}
	UKismetProceduralMeshLibrary::CalculateTangentsForMesh(Verts, Tris, UV, Normals, Tangents);
	// Separate back-face vertices retain opposing normals. Reversing triangles on
	// the same vertices cancelled lighting and made the folded rag look shredded.
	const int32 FrontVertices = Verts.Num(), FrontIndices = Tris.Num();
	for (int32 I = 0; I < FrontVertices; ++I)
	{
		const FVector P = Verts[I], N = -Normals[I];
		const FVector2D Tex = UV[I]; const FColor C = Colors[I];
		const FProcMeshTangent Tangent = Tangents[I];
		Verts.Add(P); Normals.Add(N); UV.Add(Tex); Colors.Add(C); Tangents.Add(Tangent);
	}
	for (int32 I = 0; I < FrontIndices; I += 3)
	{
		const int32 A = Tris[I], B = Tris[I + 1], C = Tris[I + 2];
		Tris.Append({FrontVertices + A, FrontVertices + C, FrontVertices + B});
	}
	if (!Rag->GetNumSections()) { Rag->CreateMeshSection(0, Verts, Tris, Normals, UV, Colors, Tangents, false); }
	else { Rag->UpdateMeshSection(0, Verts, Normals, UV, Colors, Tangents); }
}
