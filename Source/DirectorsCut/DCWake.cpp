#include "DCWake.h"
#include "DCRideDirector.h"
#include "DCTypes.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"

namespace
{
	const int32 WakeStationsPerSide = 30;
	const float WakeBowX = 2550.f;      // where the cutwater meets the sea
	const float WakeSternX = -2000.f;
	const float WakeEndX = -11000.f;    // the wake is followed 90 m astern
}

ADCWake::ADCWake()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ADCWake::Build()
{
	bBuilt = true;
	UMaterialInterface* Base = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/DC/Materials/M_Wake.M_Wake"));
	if (!Base) { UE_LOG(LogDC, Warning, TEXT("Wake: M_Wake missing; no hull wash")); return; }
	WaterLibrary = FindObject<UClass>(nullptr, TEXT("/Script/DynamicRealWater.DRW_BlueprintLibrary"));
	SurfaceFn = WaterLibrary ? WaterLibrary->FindFunctionByName(TEXT("GetWaterSurfaceAtLocation")) : nullptr;

	// Down each side from the bow, round the quarter and out astern. The inner edge hugs the hull's
	// waterline (a blunt-ended oval about 47 m by 11 m) and closes to the centreline behind the
	// stern; the outer edge spreads as the wash widens.
	for (int32 Side = -1; Side <= 1; Side += 2)
	{
		for (int32 i = 0; i < WakeStationsPerSide; ++i)
		{
			const float T = static_cast<float>(i) / (WakeStationsPerSide - 1);
			// More stations along the hull than astern.
			const float X = T < 0.55f ? FMath::Lerp(WakeBowX + 250.f, WakeSternX, T / 0.55f) : FMath::Lerp(WakeSternX, WakeEndX, FMath::Square((T - 0.55f) / 0.45f));
			float HalfBeam;
			if (X > WakeSternX)
			{
				const float U = FMath::Clamp((X - WakeSternX) / (WakeBowX - WakeSternX), 0.f, 1.f);   // 0 stern .. 1 bow
				HalfBeam = 540.f * FMath::Pow(FMath::Clamp(FMath::Sin(U * UE_PI) * 1.25f, 0.f, 1.f), 0.6f);
				HalfBeam = FMath::Max(HalfBeam, U < 0.5f ? 330.f : 0.f);                               // a square stern, a sharp bow
			}
			else
			{
				HalfBeam = FMath::Lerp(330.f, 0.f, FMath::Clamp((WakeSternX - X) / 900.f, 0.f, 1.f));
			}
			// Where the real hull can be found at the waterline, use it: probe in from abeam.
			if (X > WakeSternX - 200.f)
			{
				FCollisionQueryParams Probe(TEXT("DCWake"), true);
				const float ProbeZ = Director->SeaLevelZ + 25.f;
				float From = Side * 1600.f;
				for (int32 Step = 0; Step < 4; ++Step)
				{
					FHitResult Hit;
					if (!GetWorld()->LineTraceSingleByObjectType(Hit, FVector(X, From, ProbeZ), FVector(X, 0.f, ProbeZ), FCollisionObjectQueryParams(FCollisionObjectQueryParams::AllObjects), Probe)) { if (Step == 0 && X > WakeSternX) { HalfBeam = 0.f; } break; }
					if (Hit.GetComponent() && Hit.GetComponent()->GetName() == TEXT("SM_Root")) { From = Hit.ImpactPoint.Y - Side * 5.f; continue; }
					HalfBeam = FMath::Abs(Hit.ImpactPoint.Y);
					break;
				}
			}
			const float Aft = FMath::Clamp((WakeBowX - X) / (WakeBowX - WakeEndX), 0.f, 1.f);
			const float Width = FMath::Lerp(260.f, 2600.f, FMath::Pow(Aft, 1.3f));
			FStation S;
			S.Inner = FVector2D(X, Side * FMath::Max(HalfBeam - 60.f, 0.f));
			S.Outer = FVector2D(X - Width * 0.35f, Side * (HalfBeam + Width));
			// Strongest at the bow wave and under the quarters, dying away astern.
			S.Strength = FMath::Clamp(FMath::Min(Aft * 14.f, 1.f) * (1.f - FMath::Pow(Aft, 1.6f)), 0.f, 1.f);
			Stations.Add(S);
		}
	}
	Heights.Init(Director->SeaLevelZ, Stations.Num() * 2);

	TArray<int32> Tris; TArray<FVector> Normals; TArray<FVector2D> UVs; TArray<FColor> Colors;
	for (int32 s = 0; s < Stations.Num(); ++s)
	{
		const FStation& S = Stations[s];
		const float Along = (WakeBowX - S.Inner.X) / 1000.f;
		for (int32 e = 0; e < 3; ++e)   // inner edge, middle, outer edge
		{
			const float U = e * 0.5f;
			Verts.Add(FVector(FMath::Lerp(S.Inner, S.Outer, U), Director->SeaLevelZ));
			UVs.Add(FVector2D(U, Along));
			Colors.Add(FColor(255, 255, 255, static_cast<uint8>(255.f * S.Strength)));
		}
		const bool bLastOfSide = (s % WakeStationsPerSide) == WakeStationsPerSide - 1;
		if (!bLastOfSide)
		{
			const int32 B = s * 3;
			for (int32 e = 0; e < 2; ++e)
			{
				Tris.Append({ B + e, B + e + 3, B + e + 1, B + e + 1, B + e + 3, B + e + 4 });
			}
		}
	}
	Normals.Init(FVector::UpVector, Verts.Num());
	Mesh = NewObject<UProceduralMeshComponent>(this, TEXT("Wake"));
	Mesh->SetMobility(EComponentMobility::Movable);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);
	Mesh->SetHiddenInSceneCapture(true);
	Mesh->SetupAttachment(RootComponent);
	Mesh->RegisterComponent();
	Mesh->CreateMeshSection(0, Verts, Tris, Normals, UVs, Colors, TArray<FProcMeshTangent>(), false);
	Mesh->SetBoundsScale(3.f);
	MID = UMaterialInstanceDynamic::Create(Base, this);
	Mesh->SetMaterial(0, MID);
	UE_LOG(LogDC, Log, TEXT("Wake: hull wash and wake built (%d stations, surface query %s)"), Stations.Num(), SurfaceFn ? TEXT("found") : TEXT("missing"));
}

float ADCWake::SurfaceAt(const FVector2D& Where, int32 Channel)
{
	if (!SurfaceFn || !WaterLibrary) { return Director->SeaLevelZ; }
	uint8* Params = static_cast<uint8*>(FMemory_Alloca(FMath::Max<int32>(SurfaceFn->ParmsSize, 1)));
	FMemory::Memzero(Params, SurfaceFn->ParmsSize);
	FStructProperty* Sample = nullptr;
	for (TFieldIterator<FProperty> It(SurfaceFn); It && It->HasAnyPropertyFlags(CPF_Parm); ++It)
	{
		It->InitializeValue_InContainer(Params);
		if (FObjectProperty* O = CastField<FObjectProperty>(*It)) { O->SetObjectPropertyValue_InContainer(Params, this); }
		else if (FIntProperty* I = CastField<FIntProperty>(*It)) { I->SetPropertyValue_InContainer(Params, Channel); }
		else if (FStructProperty* SP = CastField<FStructProperty>(*It))
		{
			if (SP->Struct == TBaseStructure<FVector>::Get()) { *SP->ContainerPtrToValuePtr<FVector>(Params) = FVector(Where, Director->SeaLevelZ); }
			else { Sample = SP; }
		}
	}
	WaterLibrary->GetDefaultObject()->ProcessEvent(SurfaceFn, Params);
	float Result = Director->SeaLevelZ;
	if (Sample)
	{
		const void* Data = Sample->ContainerPtrToValuePtr<void>(Params);
		const FBoolProperty* Valid = FindFProperty<FBoolProperty>(Sample->Struct, TEXT("bValid"));
		const FFloatProperty* Height = FindFProperty<FFloatProperty>(Sample->Struct, TEXT("Height"));
		if (Valid && Height && Valid->GetPropertyValue_InContainer(Data)) { Result = Height->GetPropertyValue_InContainer(Data); }
	}
	for (TFieldIterator<FProperty> It(SurfaceFn); It && It->HasAnyPropertyFlags(CPF_Parm); ++It) { It->DestroyValue_InContainer(Params); }
	return Result;
}

void ADCWake::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Director) { Director = ADCRideDirector::Get(this); }
	if (!Director || FParse::Param(FCommandLine::Get(), TEXT("DCNoWake"))) { return; }
	if (!bBuilt)
	{
		if (Director->RideTime < 0.5f || !Director->HasPluginSea()) { return; }
		Build();
	}
	if (!Mesh || !MID) { return; }

	// Only on the plugin sea: the tunnel and the maelstrom have water of their own.
	const FDCEnvState& Env = Director->GetEnv();
	const float Want = (Env.Tube > 0.3f || Env.Whirl > 0.3f) ? 0.f : 1.f;
	Shown = FMath::FInterpConstantTo(Shown, Want, DeltaSeconds, 0.7f);
	Mesh->SetVisibility(Shown > 0.01f);
	if (Shown <= 0.01f) { return; }

	// Lay the ribbon on the waves. Every third station asks the plugin for the surface height where
	// it lies (the answer trails by a frame or two); the stations between take the heights either side.
	static const bool bFlat = FParse::Param(FCommandLine::Get(), TEXT("DCWakeFlat"));
	// Two heights per station, toward the inner and the outer edge (Heights holds them in pairs).
	for (int32 s = 0; s < Stations.Num(); ++s)
	{
		const FStation& S = Stations[s];
		for (int32 e = 0; e < 2; ++e)
		{
			// Astern the two sides meet on the centreline: both ask at that very point, so they agree.
			const bool bCentre = FMath::Abs(S.Inner.Y) < 1.f;
			const float Target = bFlat ? Director->SeaLevelZ : SurfaceAt(FMath::Lerp(S.Inner, S.Outer, e == 0 ? (bCentre ? 0.f : 0.2f) : 0.8f), s * 2 + e);
			Heights[s * 2 + e] = FMath::FInterpTo(Heights[s * 2 + e], Target, DeltaSeconds, 14.f);
		}
		const float In = Heights[s * 2], Out = Heights[s * 2 + 1];
		// A hand's breadth proud of the surface asked for, so that small waves between the points do not cut through it.
		const float Lift = 34.f;
		Verts[s * 3 + 0].Z = (FMath::Abs(S.Inner.Y) < 1.f ? In : In + (In - Out) * 0.33f) + Lift;
		Verts[s * 3 + 1].Z = (In + Out) * 0.5f + Lift;
		Verts[s * 3 + 2].Z = Out + (Out - In) * 0.33f + Lift;
	}
	Mesh->UpdateMeshSection(0, Verts, TArray<FVector>(), TArray<FVector2D>(), TArray<FColor>(), TArray<FProcMeshTangent>());

	// More white water the faster she goes and the rougher the sea; lit by the day.
	const float Speed = FMath::Clamp(Env.ShipSpeed / 1500.f, 0.4f, 1.6f);
	const float Day = FMath::Clamp(Env.SunIntensity / 6.f, 0.f, 1.f);
	const float Light = FMath::Lerp(0.16f, 1.f, Day);
	MID->SetVectorParameterValue(TEXT("Tint"), FLinearColor(0.82f * Light, 0.9f * Light, 0.96f * Light, Shown * FMath::Clamp(0.55f + 0.3f * Speed, 0.f, 1.f)));
	MID->SetScalarParameterValue(TEXT("Speed"), 1200.f * Speed);
}

ADCWake* ADCWake::Get(const UObject* WorldContext)
{
	UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
	if (!World) { return nullptr; }
	for (TActorIterator<ADCWake> It(World); It; ++It) { return *It; }
	return nullptr;
}

bool ADCWake::GetSeaUnderHull(float& OutRise, float& OutSlopeX, float& OutSlopeY) const
{
	if (!bBuilt || !Mesh || Shown < 0.5f || Heights.Num() < Stations.Num() * 2) { return false; }
	// Least squares for z = a + b x, and the difference between the sides for the roll, over the
	// stations that lie along the hull (not the wake astern).
	double N = 0, Sx = 0, Sz = 0, Sxx = 0, Sxz = 0, Port = 0, Star = 0, PortN = 0, StarN = 0, Beam = 0;
	for (int32 s = 0; s < Stations.Num(); ++s)
	{
		const FStation& S = Stations[s];
		if (S.Inner.X < WakeSternX || S.Inner.X > WakeBowX) { continue; }
		const double X = S.Inner.X, Z = Heights[s * 2] - Director->SeaLevelZ;
		N += 1; Sx += X; Sz += Z; Sxx += X * X; Sxz += X * Z;
		const double Y = FMath::Lerp(S.Inner.Y, S.Outer.Y, 0.2f);
		if (Y < 0) { Port += Z; PortN += 1; Beam -= Y; } else { Star += Z; StarN += 1; Beam += Y; }
	}
	const double Den = N * Sxx - Sx * Sx;
	if (N < 6 || PortN < 3 || StarN < 3 || FMath::Abs(Den) < 1.0) { return false; }
	const double B = (N * Sxz - Sx * Sz) / Den;
	OutSlopeX = static_cast<float>(B);
	OutRise = static_cast<float>((Sz - B * Sx) / N);
	OutSlopeY = static_cast<float>((Star / StarN - Port / PortN) / FMath::Max(Beam / N * 2.0, 100.0));
	return true;
}
