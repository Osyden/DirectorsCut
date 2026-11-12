#include "DCAudio.h"
#include "DCRideDirector.h"
#include "DCTypes.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "Kismet/GameplayStatics.h"
#include "Math/RandomStream.h"

namespace
{
	const float TwoPi = 6.2831853f;

	TArray<uint8> ToPcm16(const TArray<float>& Samples, float Gain)
	{
		// Normalise to the loudest sample so every layer starts at a known level.
		float Peak = 0.001f;
		for (float S : Samples) { Peak = FMath::Max(Peak, FMath::Abs(S)); }
		const float Scale = Gain / Peak * 32767.f;
		TArray<uint8> Out;
		Out.SetNumUninitialized(Samples.Num() * 2);
		int16* Dst = reinterpret_cast<int16*>(Out.GetData());
		for (int32 i = 0; i < Samples.Num(); ++i) { Dst[i] = static_cast<int16>(FMath::Clamp(Samples[i] * Scale, -32767.f, 32767.f)); }
		return Out;
	}

	/** Blend the tail into the head so the buffer loops without a click. */
	void MakeSeamless(TArray<float>& S, int32 Fade)
	{
		const int32 N = S.Num() - Fade;
		for (int32 i = 0; i < Fade; ++i)
		{
			const float T = static_cast<float>(i) / Fade;
			S[i] = S[i] * T + S[N + i] * (1.f - T);
		}
		S.SetNum(N);
	}

	struct FOnePole
	{
		float Z = 0.f;
		float Low(float X, float Cutoff, int32 Rate) { const float A = 1.f - FMath::Exp(-TwoPi * Cutoff / Rate); Z += A * (X - Z); return Z; }
		float High(float X, float Cutoff, int32 Rate) { return X - Low(X, Cutoff, Rate); }
	};

	/** State-variable band-pass; the centre can move every sample. */
	struct FBandPass
	{
		float L = 0.f, B = 0.f;
		float Tick(float X, float Freq, float Q, int32 Rate)
		{
			const float F = 2.f * FMath::Sin(UE_PI * FMath::Min(Freq, Rate * 0.2f) / Rate);
			L += F * B;
			const float H = X - L - B / Q;
			B += F * H;
			return B;
		}
	};
}

ADCAudio::ADCAudio()
{
	PrimaryActorTick.bCanEverTick = true;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ADCAudio::StartLayer(FLayer& Layer, TArray<float>&& Samples)
{
	MakeSeamless(Samples, Rate);
	Layer.Pcm = MakeShared<TArray<uint8>, ESPMode::ThreadSafe>(ToPcm16(Samples, 0.9f));

	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(this);
	Wave->SetSampleRate(Rate);
	Wave->NumChannels = 1;
	Wave->Duration = INDEFINITELY_LOOPING_DURATION;
	Wave->SoundGroup = SOUNDGROUP_Effects;
	Wave->bLooping = true;
	// Refill from the same buffer whenever the mixer runs dry: an endless loop.
	TSharedPtr<TArray<uint8>, ESPMode::ThreadSafe> Pcm = Layer.Pcm;
	Wave->OnSoundWaveProceduralUnderflow.BindLambda([Pcm](USoundWaveProcedural* W, int32)
	{
		W->QueueAudio(Pcm->GetData(), Pcm->Num());
	});
	Wave->QueueAudio(Pcm->GetData(), Pcm->Num());
	Waves.Add(Wave);

	Layer.Component = UGameplayStatics::SpawnSound2D(this, Wave, 1.f, 1.f, 0.f, nullptr, false, false);
	if (UAudioComponent* C = Layer.Component.Get()) { C->SetVolumeMultiplier(0.001f); }
}

void ADCAudio::BeginPlay()
{
	Super::BeginPlay();
	Director = ADCRideDirector::Get(this);
	if (Director) { Director->OnLightning.AddUObject(this, &ADCAudio::OnLightning); }

	const double Start = FPlatformTime::Seconds();
	FRandomStream Rng(1717);
	const int32 LoopSeconds = 12;
	const int32 N = Rate * (LoopSeconds + 1);
	auto Lfo = [LoopSeconds](int32 i, float Cycles, float Phase) { return FMath::Sin(TwoPi * Cycles * i / (Rate * LoopSeconds) + Phase); };

	// Wind: band-passed noise whose pitch and loudness wander in gusts.
	{
		TArray<float> S; S.SetNumUninitialized(N);
		FBandPass A, B;
		for (int32 i = 0; i < N; ++i)
		{
			const float X = Rng.FRandRange(-1.f, 1.f);
			const float Gust = 0.55f + 0.3f * Lfo(i, 2.f, 0.3f) + 0.15f * Lfo(i, 5.f, 1.7f);
			const float F1 = 420.f + 220.f * Lfo(i, 3.f, 0.f) + 90.f * Lfo(i, 7.f, 2.f);
			const float F2 = 1100.f + 400.f * Lfo(i, 2.f, 1.f);
			S[i] = (A.Tick(X, F1, 2.5f, Rate) + 0.35f * B.Tick(X, F2, 4.f, Rate)) * Gust;
		}
		StartLayer(Wind, MoveTemp(S));
	}
	// Sea: deep wash that swells and breaks, with hiss riding the crests.
	{
		TArray<float> S; S.SetNumUninitialized(N);
		FOnePole Lp1, Lp2, Hp;
		for (int32 i = 0; i < N; ++i)
		{
			const float X = Rng.FRandRange(-1.f, 1.f);
			const float Swell = FMath::Pow(0.5f + 0.5f * Lfo(i, 2.f, 0.f), 1.6f) * 0.7f + FMath::Pow(0.5f + 0.5f * Lfo(i, 3.f, 2.1f), 2.f) * 0.3f;
			const float Body = Lp2.Low(Lp1.Low(X, 480.f, Rate), 480.f, Rate) * (0.45f + 0.55f * Swell) * 3.f;
			const float Hiss = Hp.High(X, 2500.f, Rate) * Swell * Swell * 0.22f;
			S[i] = Body + Hiss;
		}
		StartLayer(Sea, MoveTemp(S));
	}
	// Rain: steady bright hiss with a little patter.
	{
		TArray<float> S; S.SetNumUninitialized(N);
		FOnePole Hp; FBandPass Bp;
		for (int32 i = 0; i < N; ++i)
		{
			const float X = Rng.FRandRange(-1.f, 1.f);
			const float Patter = Rng.FRand() > 0.9985f ? Rng.FRandRange(0.5f, 1.5f) : 0.f;
			S[i] = Hp.High(X, 1800.f, Rate) * 0.5f + Bp.Tick(X + Patter * 6.f, 5200.f, 1.2f, Rate) * 0.6f;
		}
		StartLayer(Rain, MoveTemp(S));
	}
	// Rumble: sub-bass roar with crackle, for the burning channel.
	{
		TArray<float> S; S.SetNumUninitialized(N);
		FOnePole Lp1, Lp2, CrackLp;
		float Brown = 0.f;
		for (int32 i = 0; i < N; ++i)
		{
			Brown = FMath::Clamp(Brown + Rng.FRandRange(-1.f, 1.f) * 0.05f, -1.f, 1.f);
			const float Roar = Lp2.Low(Lp1.Low(Brown, 140.f, Rate), 140.f, Rate) * (0.7f + 0.3f * Lfo(i, 4.f, 0.5f)) * 4.f;
			const float Crack = Rng.FRand() > 0.9992f ? Rng.FRandRange(-1.f, 1.f) * 3.f : 0.f;
			S[i] = Roar + CrackLp.Low(Crack, 3000.f, Rate) * 0.8f;
		}
		StartLayer(Rumble, MoveTemp(S));
	}
	// Thunder: a sharp crack, then rolling low rumble that dies away. A few variants.
	for (int32 v = 0; v < 4; ++v)
	{
		const float Len = Rng.FRandRange(3.5f, 6.f);
		const int32 TN = static_cast<int32>(Rate * Len);
		TArray<float> S; S.SetNumUninitialized(TN);
		FOnePole Lp1, Lp2, Hp;
		float Brown = 0.f;
		const float RollRate = Rng.FRandRange(2.5f, 5.f);
		const float RollPhase = Rng.FRandRange(0.f, TwoPi);
		const float CrackLen = Rng.FRandRange(0.03f, 0.09f);
		for (int32 i = 0; i < TN; ++i)
		{
			const float T = static_cast<float>(i) / Rate;
			const float X = Rng.FRandRange(-1.f, 1.f);
			Brown = FMath::Clamp(Brown + X * 0.08f, -1.f, 1.f);
			const float Crack = Hp.High(X, 900.f, Rate) * FMath::Exp(-T / CrackLen) * 1.2f;
			const float Roll = 0.55f + 0.45f * FMath::Sin(T * RollRate * TwoPi * 0.5f + RollPhase);
			const float Env = FMath::Min(1.f, T * 12.f) * FMath::Exp(-T * 3.2f / Len);
			const float Cut = FMath::Lerp(260.f, 70.f, T / Len);
			S[i] = Crack + Lp2.Low(Lp1.Low(Brown, Cut, Rate), Cut, Rate) * Env * Roll * 6.f;
		}
		Thunder.Add(ToPcm16(S, 0.95f));
	}

	UE_LOG(LogDC, Log, TEXT("Audio: synthesised weather layers in %.0f ms"), (FPlatformTime::Seconds() - Start) * 1000.0);
}

void ADCAudio::OnLightning()
{
	// Sound arrives after the light: near strikes almost at once, far ones seconds later.
	PendingThunder.Add(FPlatformTime::Seconds() + FMath::FRandRange(0.15f, 2.2f));
}

void ADCAudio::PlayOneShot(const TArray<uint8>& Pcm, float Volume)
{
	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(this);
	Wave->SetSampleRate(Rate);
	Wave->NumChannels = 1;
	Wave->Duration = INDEFINITELY_LOOPING_DURATION;
	Wave->SoundGroup = SOUNDGROUP_Effects;
	Wave->QueueAudio(Pcm.GetData(), Pcm.Num());
	Waves.Add(Wave);
	if (UAudioComponent* C = UGameplayStatics::SpawnSound2D(this, Wave, Volume * MasterVolume, FMath::FRandRange(0.85f, 1.1f)))
	{
		FOneShot O;
		O.Component = C;
		O.EndTime = FPlatformTime::Seconds() + static_cast<double>(Pcm.Num()) / (2.0 * Rate) + 0.5;
		OneShots.Add(O);
	}
}

void ADCAudio::SetLayer(FLayer& Layer, float Target, float DeltaSeconds)
{
	Layer.Volume = FMath::FInterpTo(Layer.Volume, Target, DeltaSeconds, 1.2f);
	if (UAudioComponent* C = Layer.Component.Get())
	{
		C->SetVolumeMultiplier(FMath::Max(0.001f, Layer.Volume * MasterVolume));
	}
}

void ADCAudio::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!Director) { return; }
	const FDCEnvState& Env = Director->GetEnv();

	const float Storm = FMath::Clamp(Env.WaveHeight / 320.f, 0.f, 1.2f);
	SetLayer(Wind, FMath::Clamp(0.12f + Storm * 0.55f + Env.Tube * 0.5f + Env.FogDensity * 1.2f, 0.f, 1.f), DeltaSeconds);
	SetLayer(Sea, FMath::Clamp(0.35f + Storm * 0.6f + Env.Tube * 0.4f, 0.f, 1.2f), DeltaSeconds);
	SetLayer(Rain, Env.Rain * 0.7f, DeltaSeconds);
	SetLayer(Rumble, FMath::Clamp(FMath::Min(Env.SeaGlow.R, 1.f) * 0.9f + Env.Whirl * 0.5f + Env.Tube * 0.35f, 0.f, 1.f), DeltaSeconds);

	const double Now = FPlatformTime::Seconds();
	for (int32 i = PendingThunder.Num() - 1; i >= 0; --i)
	{
		if (Now >= PendingThunder[i])
		{
			PendingThunder.RemoveAtSwap(i);
			if (Thunder.Num()) { PlayOneShot(Thunder[FMath::RandRange(0, Thunder.Num() - 1)], FMath::FRandRange(0.6f, 1.2f)); }
		}
	}
	for (int32 i = OneShots.Num() - 1; i >= 0; --i)
	{
		if (Now > OneShots[i].EndTime)
		{
			if (UAudioComponent* C = OneShots[i].Component.Get())
			{
				Waves.Remove(Cast<USoundWaveProcedural>(C->Sound));
				C->Stop();
				C->DestroyComponent();
			}
			OneShots.RemoveAtSwap(i);
		}
	}
}
