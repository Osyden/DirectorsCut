#include "DCWaveCurl.h"
#include "DCMaelstrom.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "DCRideDirector.h"
#include "DCTypes.h"
#include "ProceduralMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"

ADCWaveCurl::ADCWaveCurl()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
	for (int32 i = 0; i < 2; ++i)
	{
		Walls[i] = CreateDefaultSubobject<UProceduralMeshComponent>(i == 0 ? TEXT("Starboard") : TEXT("Port"));
		Walls[i]->SetupAttachment(RootComponent);
		Walls[i]->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		// They would put the whole deck in shade, and the sun on the deck is the point of the scene.
		Walls[i]->SetCastShadow(false);
	}
}

void ADCWaveCurl::BeginPlay()
{
	Super::BeginPlay();
	if (!FParse::Param(FCommandLine::Get(), TEXT("DCLegacyMaelstrom")))
	{
		GetWorld()->SpawnActor<ADCMaelstrom>();
		SetActorTickEnabled(false);
		return;
	}
	if (UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Wave.M_Wave")))
	{
		MID = UMaterialInstanceDynamic::Create(Base, this);
	}
	if (UMaterialInterface* Cloud = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Cloud.M_Cloud")))
	{
		SprayMID = UMaterialInstanceDynamic::Create(Cloud, this);
		SprayMID->SetScalarParameterValue(TEXT("Erode"), 1.f);
	}
	BuildSide(0, 1.f, HeightStarboard);
	BuildSide(1, -1.f, HeightPort);
	for (int32 i = 0; i < 2; ++i)
	{
		if (MID) { Walls[i]->SetMaterial(0, MID); }
		Walls[i]->SetVisibility(false, true);
	}
}

void ADCWaveCurl::BuildSide(int32 Index, float Sign, float Height)
{
	const int32 NX = 72;   // along the ship
	const int32 NP = 26;   // up the face
	const float Phase = Index * 2.1f;
	TArray<FVector> Verts;
	TArray<FVector> Normals;
	TArray<FVector2D> UVs;
	TArray<FColor> Colors;
	TArray<int32> Tris;
	Verts.Reserve((NX + 1) * (NP + 1));

	for (int32 ix = 0; ix <= NX; ++ix)
	{
		const float X = FMath::Lerp(-Astern, Ahead, static_cast<float>(ix) / NX);
		// Full height abeam, nothing at either end.
		const float Env = FMath::SmoothStep(-Astern, -Astern + 7000.f, X) * (1.f - FMath::SmoothStep(1500.f, Ahead, X));
		// The crest is uneven along its length, and the lip reaches further over in places.
		const float Crest = 1.f + 0.14f * FMath::Sin(X * 0.00085f + Phase) + 0.07f * FMath::Sin(X * 0.0023f + Phase * 2.f);
		const float H = Height * Env * Crest;
		const float R = Hollow * FMath::Max(Env, 0.02f) * (1.f + 0.1f * FMath::Sin(X * 0.0012f + Phase + 1.f));
		const float Over = FMath::DegreesToRadians(118.f + 24.f * FMath::Sin(X * 0.0016f + Phase * 3.f));
		for (int32 ip = 0; ip <= NP; ++ip)
		{
			const float P = static_cast<float>(ip) / NP;
			FVector V;
			float Foam, Thin, Up = 0.f;
			if (ip == 0)
			{
				// A skirt well under the surface, so the heaving sea never shows the wall's foot.
				V = FVector(X, Sign * (Distance - R * 2.2f), -9000.f);
				Foam = 0.f; Thin = 0.f;
			}
			else
			{
				// The face is an arc: from the trough, out and up to the back of the wall, then over
				// the top and down toward the ship.
				const float A = FMath::Lerp(-UE_HALF_PI, Over, (P - 1.f / NP) / (1.f - 1.f / NP));
				V = FVector(X, Sign * (Distance - R + R * FMath::Cos(A)), H * 0.5f * (1.f + FMath::Sin(A)));
				Up = (A + UE_HALF_PI) / (Over + UE_HALF_PI);
				Foam = FMath::Max(FMath::Square(1.f - FMath::Min(1.f, Up * 5.f)), FMath::Pow(Up, 7.f));
				Thin = FMath::Pow(Up, 2.2f);
			}
			Verts.Add(V);
			if (ip == NP && ix % 3 == 1 && Env > 0.2f) { Spray(Index, V, H * (0.24f + 0.1f * FMath::Sin(X * 0.004f + Phase))); }
			UVs.Add(FVector2D(X / 5000.f, P * 2.4f + 0.2f * FMath::Sin(X * 0.0011f + Phase)));
			Colors.Add(FLinearColor(Foam, Thin, Up, 1.f).ToFColor(false));
		}
	}
	// Normals from the grid, turned to face the ship.
	Normals.SetNum(Verts.Num());
	auto At = [&](int32 ix, int32 ip) { return Verts[FMath::Clamp(ix, 0, NX) * (NP + 1) + FMath::Clamp(ip, 0, NP)]; };
	for (int32 ix = 0; ix <= NX; ++ix)
	{
		for (int32 ip = 0; ip <= NP; ++ip)
		{
			FVector N = FVector::CrossProduct(At(ix + 1, ip) - At(ix - 1, ip), At(ix, ip + 1) - At(ix, ip - 1)).GetSafeNormal();
			const FVector V = At(ix, ip);
			const FVector Inward = FVector(V.X, Sign * (Distance - Hollow), Height * 0.5f) - V;
			if (FVector::DotProduct(N, Inward) < 0.f) { N = -N; }
			Normals[ix * (NP + 1) + ip] = N;
		}
	}
	for (int32 ix = 0; ix < NX; ++ix)
	{
		for (int32 ip = 0; ip < NP; ++ip)
		{
			const int32 A = ix * (NP + 1) + ip;
			const int32 B = A + NP + 1;
			Tris.Append({ A, B, A + 1, A + 1, B, B + 1 });
		}
	}
	Walls[Index]->CreateMeshSection(0, Verts, Tris, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);
}

void ADCWaveCurl::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Director) { Director = ADCRideDirector::Get(this); }
	if (!Director) { return; }
	const FDCEnvState& Env = Director->GetEnv();
	// Up once the whirlpool has opened and the tunnel has let go of the ship.
	const float Target = FMath::Clamp((Env.Whirl - 0.55f) / 0.4f, 0.f, 1.f) * FMath::Clamp(1.f - Env.Tube * 2.f, 0.f, 1.f);
	Rise = FMath::FInterpTo(Rise, Target, DeltaSeconds, 0.5f);
	const bool bShow = Rise > 0.01f;
	const float T = Director->RideTime;
	for (int32 i = 0; i < 2; ++i)
	{
		Walls[i]->SetVisibility(bShow, true);
		if (!bShow) { continue; }
		// Each wall heaves slowly, out of step with the other.
		const float Heave = 1.f + 0.07f * FMath::Sin(T * 0.45f + i * 2.4f);
		const float Sway = 500.f * FMath::Sin(T * 0.3f + i * 1.3f);
		Walls[i]->SetRelativeScale3D(FVector(1.f, 1.f, FMath::Max(0.01f, Rise * Heave)));
		Walls[i]->SetRelativeLocation(FVector(1500.f * FMath::Sin(T * 0.21f + i), (i == 0 ? 1.f : -1.f) * Sway, Director->SeaLevelZ - 600.f * (1.f - Rise) - 0.15f * Director->SwellLift));
	}
	if (SprayMID)
	{
		const FLinearColor Mist = FLinearColor(0.7f, 0.8f, 0.85f) * (0.12f + 0.085f * Env.SunIntensity);
		SprayMID->SetVectorParameterValue(TEXT("Top"), Mist);
		SprayMID->SetVectorParameterValue(TEXT("Bottom"), Mist);
		SprayMID->SetVectorParameterValue(TEXT("Flash"), FLinearColor(0.f, 0.f, 0.f, 0.75f * Rise));
	}
	if (MID)
	{
		// The water takes the sea's colour, and the lip glows with the sun behind it.
		const FLinearColor Sea = Env.SeaColor;
		MID->SetVectorParameterValue(TEXT("Deep"), Sea * 0.9f);
		MID->SetVectorParameterValue(TEXT("Scatter"), FLinearColor(Sea.R * 3.f + 0.005f, Sea.G * 3.2f, Sea.B * 3.f, 0.045f * Env.SunIntensity));
	}
}

void ADCWaveCurl::Spray(int32 Index, const FVector& Location, float Size)
{
	UStaticMesh* Sphere = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (!Sphere || !SprayMID) { return; }
	// Spindrift torn off the lip: it hides the edge of the sheet and makes the crest read as breaking.
	UStaticMeshComponent* C = NewObject<UStaticMeshComponent>(this);
	C->SetMobility(EComponentMobility::Movable);
	C->SetStaticMesh(Sphere);
	C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	C->SetCastShadow(false);
	C->SetMaterial(0, SprayMID);
	C->SetupAttachment(Walls[Index]);
	C->SetRelativeLocation(Location);
	C->SetRelativeScale3D(FVector(Size * 1.6f, Size, Size * 0.8f) / 50.f);
	C->RegisterComponent();
}
