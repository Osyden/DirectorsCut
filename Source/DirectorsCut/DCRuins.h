#pragma once

#include "CoreMinimal.h"

class AActor;
class UWorld;

/**
 * Atlantis built from the Modular Ancient Ruins pack (Content/AncientRuins, not in the repo).
 * Each builder assembles a structure at the pack's real scale around its own base centre and
 * then scales the whole actor up, because the drowned city is seen from hundreds of metres off.
 * Available() is false when the pack is not installed; callers then fall back to DCHazards.
 */
namespace DCRuins
{
	bool Available();

	/** A robed statue on a pedestal. Height is the full height in cm. */
	AActor* SpawnColossus(UWorld* World, float Height);
	/** A run of standing colonnade, Segments long, with broken ends. Height in cm. */
	AActor* SpawnColonnade(UWorld* World, int32 Segments, float Height);
	/** A temple: columned front and back under pediments, on a stepped base. Scale 1 = 38 m wide. */
	AActor* SpawnTemple(UWorld* World, float Scale);
	/** One of the pack's four monumental arches (Variant 1..4). Scale 1 = 20 to 30 m tall. */
	AActor* SpawnArch(UWorld* World, int32 Variant, float Scale);
	/** A gateway: two towers of masonry with an arch between and statues on top. Scale 1 = about 30 m tall. */
	AActor* SpawnGate(UWorld* World, float Scale);
	/** Lane obstacle: a cluster of broken columns and fallen blocks standing in the water, about Radius across (cm). */
	AActor* SpawnBrokenColumns(UWorld* World, float Radius);
	/** Lane obstacle: a crag of rock. */
	AActor* SpawnCrag(UWorld* World, float Radius, float Height);
}
