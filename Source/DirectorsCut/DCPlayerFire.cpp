#include "DCPlayer.h"
#include "DCRideDirector.h"
#include "DCTroubles.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
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
	Rag->SetMaterial(0, Material(FLinearColor(.18f, .14f, .085f)));
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
	// A closed leather grip: knuckles and a curled thumb break up the silhouette.
	for (int32 Finger = 0; Finger < 4; ++Finger)
	{
		UStaticMeshComponent* Knuckle = Grip(*FString::Printf(TEXT("RagFinger%d"), Finger), FLinearColor(.045f, .028f, .016f));
		Knuckle->AttachToComponent(RagHand, FAttachmentTransformRules::KeepRelativeTransform);
		Knuckle->SetRelativeLocation(FVector(30.f, -31.f + Finger * 20.f, 2.f));
		Knuckle->SetRelativeScale3D(FVector(.52f, .25f, .8f));
	}
	UStaticMeshComponent* Thumb = Grip(TEXT("RagThumb"), FLinearColor(.05f, .03f, .017f));
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
	SwatAge = 0.f;
	bSwatContact = false;
	if (!Rag) { BuildRag(); }
	return true;
}

void ADCPlayerCharacter::TickRag(float DeltaSeconds)
{
	SwatAge += DeltaSeconds;
	const bool bVisible = SwatAge < .62f && !IsAtStation() && !bShipView && !bTyping;
	if (Rag) { Rag->SetVisibility(bVisible); }
	if (RagHand) { RagHand->SetVisibility(bVisible, true); }
	if (RagSleeve) { RagSleeve->SetVisibility(bVisible); }
	if (!bVisible || !Rag) { return; }
	if (!bSwatContact && SwatAge >= .23f)
	{
		bSwatContact = true;
		ADCTroubles* Troubles = ADCTroubles::Get(this);
		const ADCRideDirector* Director = ADCRideDirector::Get(this);
		if (Troubles && Director)
		{
			const FVector Here = Director->DeckTransform.InverseTransformPosition(GetActorLocation());
			if (FVector::Distance(Here, SwatTarget) <= 350.f && Troubles->SwatNear(SwatTarget, 100.f))
			{
				UE_LOG(LogDC, Log, TEXT("Rag contact: remaining=%d at %s"), Troubles->SwatsRemaining(SwatTarget), *SwatTarget.ToCompactString());
			}
		}
	}

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
	constexpr int32 Across = 6, Along = 9;
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
