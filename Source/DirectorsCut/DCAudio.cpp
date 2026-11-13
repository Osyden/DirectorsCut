#include "DCAudio.h"
#include "DCRideDirector.h"
#include "DCTypes.h"
#include "DCTroubles.h"
#include "DCCrew.h"
#include "DCKraken.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimationAsset.h"
#include "Sound/SoundBase.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
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
	if (Director)
	{
		Director->OnLightning.AddUObject(this, &ADCAudio::OnLightning);
		Director->OnShipHit.AddUObject(this, &ADCAudio::OnShipHit);
		Director->OnHullStrike.AddUObject(this, &ADCAudio::OnHullStrike);
	}

	// The storm bell: the ship asset has a bell, a strike sound and a swing animation.
	BellSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Brigantine/SFX/Cue/Bell_cue.Bell_cue"));
	BellAnim = LoadObject<UAnimationAsset>(nullptr, TEXT("/Game/Brigantine/Animation/Bell/AS_Bell-brigantine.AS_Bell-brigantine"));
	for (TActorIterator<AActor> It(GetWorld()); It && !BellMesh.IsValid(); ++It)
	{
		TArray<USkeletalMeshComponent*> Skels;
		It->GetComponents(Skels);
		for (USkeletalMeshComponent* C : Skels) { if (C->GetName() == TEXT("SK_Bell")) { BellMesh = C; break; } }
	}
	UE_LOG(LogDC, Log, TEXT("Audio: storm bell sound %s, bell mesh %s"), BellSound ? TEXT("found") : TEXT("missing"), BellMesh.IsValid() ? TEXT("found") : TEXT("missing"));

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
		FOnePole Lp1, Lp2, CrackLp, DcBlock;
		float Brown = 0.f;
		for (int32 i = 0; i < N; ++i)
		{
			Brown = FMath::Clamp(Brown + Rng.FRandRange(-1.f, 1.f) * 0.05f, -1.f, 1.f);
			const float Roar = Lp2.Low(Lp1.Low(Brown, 140.f, Rate), 140.f, Rate) * (0.7f + 0.3f * Lfo(i, 4.f, 0.5f)) * 4.f;
			const float Crack = Rng.FRand() > 0.9992f ? Rng.FRandRange(-1.f, 1.f) * 3.f : 0.f;
			S[i] = DcBlock.High(Roar, 28.f, Rate) + CrackLp.Low(Crack, 3000.f, Rate) * 0.8f; // keep it above what speakers cannot play
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
		FOnePole ThunderDc;
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
			S[i] = Crack + ThunderDc.High(Lp2.Low(Lp1.Low(Brown, Cut, Rate), Cut, Rate), 30.f, Rate) * Env * Roll * 6.f;
		}
		Thunder.Add(ToPcm16(S, 0.95f));
	}

	// Drone: a low minor chord of detuned tones that swells slowly. Mood, kept quiet.
	{
		TArray<float> S; S.SetNumZeroed(N);
		// Whole numbers of cycles over the loop so it repeats cleanly: D, A, D, F over a low D.
		const float Notes[] = { 36.67f, 73.42f, 110.f, 146.83f, 174.61f };
		const float Gains[] = { 0.9f, 0.7f, 0.45f, 0.3f, 0.2f };
		for (int32 n = 0; n < UE_ARRAY_COUNT(Notes); ++n)
		{
			for (int32 d = -1; d <= 1; ++d)
			{
				const float Cycles = FMath::RoundToFloat((Notes[n] * (1.f + d * 0.004f)) * LoopSeconds);
				const float Phase = Rng.FRandRange(0.f, TwoPi);
				for (int32 i = 0; i < N; ++i)
				{
					const float T = static_cast<float>(i) / (Rate * LoopSeconds);
					const float X = FMath::Sin(TwoPi * Cycles * T + Phase);
					// A little soft clipping adds warmth above the pure sine.
					S[i] += Gains[n] * (X + 0.2f * X * X * X) * (0.75f + 0.25f * Lfo(i, 1.f + n, d * 1.3f));
				}
			}
		}
		StartLayer(Drone, MoveTemp(S));
	}
	// Crackle: fire on deck. Sparse sharp pops over a soft roar.
	{
		TArray<float> S; S.SetNumUninitialized(N);
		FOnePole Lp, Hp;
		float Pop = 0.f;
		for (int32 i = 0; i < N; ++i)
		{
			const float X = Rng.FRandRange(-1.f, 1.f);
			if (Rng.FRand() > 0.9988f) { Pop = Rng.FRandRange(0.6f, 1.6f); }
			Pop *= 0.992f;
			S[i] = Lp.Low(X, 700.f, Rate) * 0.5f + Hp.High(X, 1500.f, Rate) * Pop;
		}
		StartLayer(Crackle, MoveTemp(S));
	}
	// Impacts: a hull strike. Low thud, splintering on top.
	for (int32 v = 0; v < 3; ++v)
	{
		const int32 TN = Rate * 2;
		TArray<float> S; S.SetNumUninitialized(TN);
		FOnePole Lp, Hp;
		const float Pitch = Rng.FRandRange(42.f, 62.f);
		for (int32 i = 0; i < TN; ++i)
		{
			const float T = static_cast<float>(i) / Rate;
			const float X = Rng.FRandRange(-1.f, 1.f);
			const float Thud = FMath::Sin(TwoPi * Pitch * T * (1.f + 2.f * FMath::Exp(-T * 30.f))) * FMath::Exp(-T * 5.f) * 1.4f;
			const float Body = Lp.Low(X, 300.f, Rate) * FMath::Exp(-T * 7.f) * 2.5f;
			const float Splinter = Hp.High(X, 1800.f, Rate) * FMath::Exp(-T * 14.f) * (Rng.FRand() > 0.6f ? 1.f : 0.2f) * 0.6f;
			S[i] = Thud + Body + Splinter;
		}
		Impacts.Add(ToPcm16(S, 0.95f));
	}
	// Scrapes: the hull dragged along stone. A low groan that grips and lets go many times a second
	// (timber sticking and slipping), with a band of grit over it that rises as it grips.
	for (int32 v = 0; v < 2; ++v)
	{
		const float Len = 1.7f;
		const int32 TN = static_cast<int32>(Rate * Len);
		TArray<float> S; S.SetNumUninitialized(TN);
		FOnePole Lp; FBandPass Bp;
		float Phase = 0.f;
		const float GripRate = Rng.FRandRange(6.f, 10.f);
		for (int32 i = 0; i < TN; ++i)
		{
			const float T = static_cast<float>(i) / Rate;
			const float X = Rng.FRandRange(-1.f, 1.f);
			const float Whole = FMath::Pow(FMath::Sin(UE_PI * T / Len), 0.6f);
			const float Grip = 0.5f + 0.5f * FMath::Sin(TwoPi * GripRate * T + 3.f * FMath::Sin(TwoPi * 1.3f * T));
			Phase += (58.f + 26.f * Grip) / Rate;
			const float Groan = Lp.Low(2.f * FMath::Frac(Phase) - 1.f, 420.f, Rate) * Grip;
			const float Grit = Bp.Tick(X, 900.f + 700.f * Grip, 2.f, Rate) * (0.35f + 0.65f * Grip);
			S[i] = (Groan * 1.1f + Grit * 0.9f) * Whole;
		}
		Scrapes.Add(ToPcm16(S, 0.9f));
	}
	// Roar: the kraken. A falling growl with a rasp, like a ship's hull groaning under water.
	{
		const float Len = 5.f;
		const int32 TN = static_cast<int32>(Rate * Len);
		TArray<float> S; S.SetNumUninitialized(TN);
		FOnePole Lp;
		FBandPass F1, F2;
		float Phase = 0.f;
		for (int32 i = 0; i < TN; ++i)
		{
			const float T = static_cast<float>(i) / Rate;
			const float U = T / Len;
			const float Freq = FMath::Lerp(70.f, 38.f, U) * (1.f + 0.04f * FMath::Sin(T * 31.f));
			Phase += TwoPi * Freq / Rate;
			// Buzzy source (sum of harmonics) through two moving vowel-like bands.
			float Src = 0.f;
			for (int32 h = 1; h <= 9; ++h) { Src += FMath::Sin(Phase * h) / h; }
			Src += Rng.FRandRange(-1.f, 1.f) * 0.35f;
			const float Voiced = F1.Tick(Src, FMath::Lerp(420.f, 230.f, U), 3.f, Rate) + 0.6f * F2.Tick(Src, FMath::Lerp(900.f, 520.f, U), 4.f, Rate) + 0.5f * Lp.Low(Src, 160.f, Rate);
			const float Env = FMath::Min(1.f, T * 2.5f) * FMath::Pow(1.f - U, 0.7f);
			S[i] = Voiced * Env;
		}
		KrakenRoar = ToPcm16(S, 0.95f);
	}

	DumpWav(TEXT("loop_wind"), *Wind.Pcm); DumpWav(TEXT("loop_sea"), *Sea.Pcm); DumpWav(TEXT("loop_rain"), *Rain.Pcm); DumpWav(TEXT("loop_rumble"), *Rumble.Pcm);
	DumpWav(TEXT("loop_drone"), *Drone.Pcm); DumpWav(TEXT("loop_fire_crackle"), *Crackle.Pcm); DumpWav(TEXT("kraken_roar"), KrakenRoar);
	for (int32 i = 0; i < Thunder.Num(); ++i) { DumpWav(*FString::Printf(TEXT("thunder_%d"), i), Thunder[i]); }
	for (int32 i = 0; i < Impacts.Num(); ++i) { DumpWav(*FString::Printf(TEXT("hull_impact_%d"), i), Impacts[i]); }
	for (int32 i = 0; i < Scrapes.Num(); ++i) { DumpWav(*FString::Printf(TEXT("hull_scrape_%d"), i), Scrapes[i]); }
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
	TickBell(DeltaSeconds);

	// Each bed prefers a recording when one has been imported (Content/Sonniss) and falls back to the
	// synthesised layer otherwise, so the project still makes sound on a machine without the recordings.
	const float WindTarget = FMath::Clamp(0.12f + Storm * 0.55f + Env.Tube * 0.5f + Env.FogDensity * 1.2f, 0.f, 1.f) * (BellDuck > 0.f ? 0.55f : 1.f);
	const float SeaTarget = FMath::Clamp(0.35f + Storm * 0.6f + Env.Tube * 0.4f, 0.f, 1.2f);
	const float Rough = FMath::Clamp((Storm - 0.3f) / 0.5f, 0.f, 1.f);
	const ADCTroubles* Troubles = ADCTroubles::Get(this);
	const float FireTarget = Troubles ? FMath::Min(Troubles->NumFires() * 0.4f, 0.9f) : 0.f;

	const bool bRealWind = SetReal(TEXT("wind_storm"), WindTarget * 0.8f, DeltaSeconds);
	const bool bRealCalm = SetReal(TEXT("sea_calm"), SeaTarget * (1.f - Rough) * 0.9f, DeltaSeconds);
	const bool bRealRough = SetReal(TEXT("sea_rough"), SeaTarget * Rough * 0.9f, DeltaSeconds);
	const bool bRealRain = SetReal(TEXT("rain_storm"), Env.Rain * 0.8f, DeltaSeconds);
	const bool bRealFire = SetReal(TEXT("fire_loop"), FireTarget, DeltaSeconds);

	SetLayer(Wind, bRealWind ? WindTarget * 0.25f : WindTarget, DeltaSeconds);
	SetLayer(Sea, (bRealCalm && bRealRough) ? 0.f : SeaTarget, DeltaSeconds);
	SetLayer(Rain, bRealRain ? 0.f : Env.Rain * 0.7f, DeltaSeconds);
	SetLayer(Rumble, FMath::Clamp(FMath::Min(Env.SeaGlow.R, 1.f) * 0.9f + Env.Whirl * 0.5f + Env.Tube * 0.35f, 0.f, 1.f), DeltaSeconds);
	SetLayer(Drone, FMath::Clamp(0.1f + Env.FogDensity * 1.5f + Env.Whirl * 0.3f + FMath::Min(Env.SeaGlow.R, 1.f) * 0.2f, 0.f, 0.38f), DeltaSeconds);
	SetLayer(Crackle, bRealFire ? 0.f : FireTarget, DeltaSeconds);

	// A horn swell marks each turn for the worse.
	const EDCPhase Phase = Director->GetPhase();
	if (Phase != LastPhase)
	{
		LastPhase = Phase;
		if (Phase == EDCPhase::Slide || Phase == EDCPhase::Hell || Phase == EDCPhase::Maelstrom) { PlayReal(TEXT("braam"), 0.7f); }
	}

	// The kraken announces itself as it rises, again when it is up, and at the end.
	MaelstromTime = Phase == EDCPhase::Maelstrom ? MaelstromTime + DeltaSeconds : 0.f;
	if (MaelstromTime <= 0.f) { RoarsPlayed = 0; }
	static const float RoarTimes[] = { 27.f, 62.f, 110.f, 148.f };
	if (RoarsPlayed < UE_ARRAY_COUNT(RoarTimes) && MaelstromTime > RoarTimes[RoarsPlayed])
	{
		++RoarsPlayed;
		if (KrakenRoar.Num()) { PlayOneShot(KrakenRoar, 1.3f); }
		PlayReal(TEXT("kraken_breath"), 1.f, 0.7f);
		Director->Trauma = FMath::Min(1.f, Director->Trauma + 0.5f);
	}
	// It screams when an arm is shot away.
	if (!Kraken.IsValid())
	{
		for (TActorIterator<ADCKraken> It(GetWorld()); It; ++It) { Kraken = *It; break; }
	}
	if (Kraken.IsValid() && Kraken->ArmsDestroyed != LastArmsDestroyed)
	{
		if (Kraken->ArmsDestroyed > LastArmsDestroyed) { PlayReal(TEXT("kraken_pain"), 0.9f, FMath::FRandRange(0.75f, 0.9f)); }
		LastArmsDestroyed = Kraken->ArmsDestroyed;
	}

	const double Now = FPlatformTime::Seconds();
	for (int32 i = PendingThunder.Num() - 1; i >= 0; --i)
	{
		if (Now >= PendingThunder[i])
		{
			PendingThunder.RemoveAtSwap(i);
			// Mostly the synthesised cracks (short, varied); now and then the long recorded roll.
			if (FMath::FRand() < 0.3f && PlayReal(TEXT("thunder_real"), FMath::FRandRange(0.7f, 1.f), FMath::FRandRange(0.9f, 1.05f))) { continue; }
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

void ADCAudio::OnShipHit(float Amount)
{
	// Water and timber together: the recorded splash under the synthesised crunch.
	PlayReal(TEXT("water_impact"), FMath::Clamp(0.4f + Amount * 0.1f, 0.4f, 0.9f), FMath::FRandRange(0.85f, 1.1f));
	if (Impacts.Num())
	{
		PlayOneShot(Impacts[FMath::RandRange(0, Impacts.Num() - 1)], FMath::Clamp(0.5f + Amount * 0.15f, 0.5f, 1.3f));
	}
}

void ADCAudio::OnHullStrike(const FVector& Where, float Strength)
{
	// The blow itself is OnShipHit (splash and crunch); this is the stone dragging along her side after it.
	if (Scrapes.Num()) { PlayOneShot(Scrapes[FMath::RandRange(0, Scrapes.Num() - 1)], FMath::Clamp(0.35f + 0.4f * Strength, 0.3f, 0.85f)); }
}

void ADCAudio::DumpWav(const TCHAR* Name, const TArray<uint8>& Pcm) const
{
	// -DCDumpAudio writes every synthesised sound to Saved/AudioDump so it can be auditioned outside the game.
	if (!FParse::Param(FCommandLine::Get(), TEXT("DCDumpAudio"))) { return; }
	TArray<uint8> Out;
	auto W32 = [&Out](uint32 V) { Out.Append(reinterpret_cast<const uint8*>(&V), 4); };
	auto W16 = [&Out](uint16 V) { Out.Append(reinterpret_cast<const uint8*>(&V), 2); };
	Out.Append(reinterpret_cast<const uint8*>("RIFF"), 4); W32(36 + Pcm.Num());
	Out.Append(reinterpret_cast<const uint8*>("WAVEfmt "), 8); W32(16); W16(1); W16(1);
	W32(Rate); W32(Rate * 2); W16(2); W16(16);
	Out.Append(reinterpret_cast<const uint8*>("data"), 4); W32(Pcm.Num());
	Out.Append(Pcm);
	FFileHelper::SaveArrayToFile(Out, *(FPaths::ProjectSavedDir() / TEXT("AudioDump") / FString::Printf(TEXT("%s.wav"), Name)));
}

void ADCAudio::TickBell(float DeltaSeconds)
{
	// The ship's bell begins to ring by itself as the weather turns: one lone strike, a long silence,
	// then slow irregular tolling that quickens at the worst of it. Each strike is its own event so
	// the pattern never loops, and it stays slower than an alarm.
	const EDCPhase Phase = Director->GetPhase();
	const bool bHaunted = Phase != EDCPhase::OpenSea && Phase != EDCPhase::Ending;
	BellDuck = FMath::Max(0.f, BellDuck - DeltaSeconds);
	if (!bHaunted)
	{
		BellStrikes = 0;
		NextBellIn = 6.f;
		return;
	}
	NextBellIn -= DeltaSeconds;
	if (NextBellIn > 0.f || !BellSound) { return; }

	const bool bPeak = Phase == EDCPhase::Slide || Phase == EDCPhase::Breach || Phase == EDCPhase::Maelstrom;
	++BellStrikes;
	if (BellStrikes == 1) { NextBellIn = FMath::FRandRange(15.f, 25.f); }
	else { NextBellIn = bPeak ? FMath::FRandRange(4.f, 8.f) : FMath::FRandRange(8.f, 14.f); }

	// Early strikes are quiet; later ones vary a little in strength. Pitched down for weight.
	const float Volume = (BellStrikes <= 2 ? 0.6f : FMath::FRandRange(0.8f, 1.05f)) * MasterVolume * BellVolume;
	const float Pitch = FMath::FRandRange(0.8f, 0.87f);
	const FVector Where = BellMesh.IsValid() ? BellMesh->Bounds.Origin : FVector(-446.f, 0.f, 812.f);
	UGameplayStatics::SpawnSoundAtLocation(this, BellSound, Where, FRotator::ZeroRotator, Volume, Pitch);
	if (BellMesh.IsValid() && BellAnim) { BellMesh->PlayAnimation(BellAnim, false); }
	BellDuck = 1.6f; // pull the wind back so the strike reads

	if (BellStrikes == 2)
	{
		if (UDCCrewSubsystem* Crew = GetWorld()->GetSubsystem<UDCCrewSubsystem>())
		{
			Crew->Report(TEXT("The bell, Captain. Nobody is ringing it."), true);
		}
	}
}

USoundBase* ADCAudio::FindReal(FName Name)
{
	if (TObjectPtr<USoundBase>* Found = RealSounds.Find(Name)) { return Found->Get(); }
	USoundBase* Sound = LoadObject<USoundBase>(nullptr, *FString::Printf(TEXT("/Game/Sonniss/%s.%s"), *Name.ToString(), *Name.ToString()), nullptr, LOAD_NoWarn | LOAD_Quiet);
	UE_LOG(LogDC, Log, TEXT("Audio: recording %s %s"), *Name.ToString(), Sound ? TEXT("loaded") : TEXT("not present, using synthesis"));
	RealSounds.Add(Name, Sound); // remembers a miss too, so a missing file is only looked for once
	return Sound;
}

bool ADCAudio::SetReal(FName Name, float Target, float DeltaSeconds)
{
	USoundBase* Sound = FindReal(Name);
	if (!Sound) { return false; }
	FRealBed& Bed = RealBeds.FindOrAdd(Name);
	if (!Bed.Component.IsValid())
	{
		Bed.Component = UGameplayStatics::SpawnSound2D(this, Sound, 1.f, 1.f, FMath::FRandRange(0.f, 20.f), nullptr, false, false);
		if (UAudioComponent* C = Bed.Component.Get()) { C->SetVolumeMultiplier(0.001f); }
	}
	Bed.Volume = FMath::FInterpTo(Bed.Volume, Target, DeltaSeconds, 1.2f);
	if (UAudioComponent* C = Bed.Component.Get()) { C->SetVolumeMultiplier(FMath::Max(0.001f, Bed.Volume * MasterVolume)); }
	return true;
}

bool ADCAudio::PlayReal(FName Name, float Volume, float Pitch)
{
	USoundBase* Sound = FindReal(Name);
	if (!Sound) { return false; }
	UGameplayStatics::PlaySound2D(this, Sound, Volume * MasterVolume, Pitch);
	return true;
}
