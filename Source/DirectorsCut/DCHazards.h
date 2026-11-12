#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;

/**
 * Builds the things that loom out of the fog. Everything is made in code from noise and engine
 * primitives, plus the vessel asset's own hull for ghost ships, so no extra art is needed.
 */
namespace DCHazards
{
	/** A sea stack: noise-displaced column. Radius and Height in cm. Jagged gives the spires of the hell channel. */
	AActor* SpawnRock(UWorld* World, float Radius, float Height, bool bJagged, const FLinearColor& Color);

	/** A broken arch or a row of columns. Returns the half-width that blocks the lane. */
	AActor* SpawnRuin(UWorld* World, float Scale, const FLinearColor& Color);

	/** A colossal robed figure, arm raised. Scenery only. */
	AActor* SpawnStatue(UWorld* World, float Height, const FLinearColor& Color);

	/** A spectral copy of our own ship. */
	AActor* SpawnGhostShip(UWorld* World, const FLinearColor& Glow);

	/** Atlantis: a guardian colossus with spear (or trident and crown), a broken aqueduct, the high temple. */
	AActor* SpawnSentinel(UWorld* World, float Height, bool bTrident, const FLinearColor& Color);
	AActor* SpawnAqueduct(UWorld* World, int32 Arches, float Height, const FLinearColor& Color);
	AActor* SpawnTemple(UWorld* World, float Scale, const FLinearColor& Color);

	/** A translucent giant figure. bWading sinks it to the hips (demons in the burning sea). */
	AActor* SpawnSpecter(UWorld* World, float Height, const FLinearColor& Glow, bool bWading);

	/** Light the cracks of anything built from the stone material. */
	void SetGlow(AActor* Actor, const FLinearColor& Glow);
}
