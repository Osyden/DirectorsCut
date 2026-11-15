#include "DCConversationContext.h"

#include "DCBossCombat.h"
#include "DCCrew.h"
#include "DCEarlyEncounters.h"
#include "DCGunnery.h"
#include "DCLeviathan.h"
#include "DCPlayer.h"
#include "DCRideDirector.h"
#include "DCTreadmill.h"
#include "DCTroubles.h"
#include "Components/ChildActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "ProceduralMeshComponent.h"

namespace DCConversationContextPrivate
{
    const TCHAR* SideName(EDCSide Side)
    {
        return Side == EDCSide::Port ? TEXT("port") : Side == EDCSide::Starboard ? TEXT("starboard") : TEXT("unspecified side");
    }

    FString StationName(const FDCStation* Station)
    {
        if (!Station) { return TEXT("no assigned station"); }
        switch (Station->Type)
        {
        case EDCStationType::Helm: return TEXT("wheel");
        case EDCStationType::Cannons: return FString::Printf(TEXT("%s cannon"), SideName(Station->Side));
        case EDCStationType::Fire: return TEXT("deck fire duty, swatting flames with a rag");
        case EDCStationType::Rigging: return TEXT("marked rigging repair lines on deck");
        default: return TEXT("no assigned station");
        }
    }

    FString Duty(const ADCCrewMember* Member, const FDCStation* Station, const ADCTroubles* Troubles, const ADCGunnery* Guns)
    {
        if (Member->State == EDCCrewState::Idle) { return TEXT("standing by, available for an order"); }
        const FString Post = StationName(Station);
        if (Member->State == EDCCrewState::Moving) { return TEXT("walking to ") + Post; }
        if (!Station) { return TEXT("station status unavailable"); }
        if (Station->Type == EDCStationType::Cannons)
        {
            const FString Ready = Guns && Station->GunIndex != INDEX_NONE
                ? (Guns->IsGunReady(Station->GunIndex) ? TEXT("loaded") : TEXT("reloading")) : TEXT("reload status unknown");
            return Post + (Member->IsHoldingFire() ? TEXT("; holding fire; ") : TEXT("; permitted to fire at valid targets; ")) + Ready;
        }
        if (Station->Type == EDCStationType::Fire && Troubles && Troubles->NumFires() == 0)
        {
            return TEXT("standing by on fire duty; no deck fire currently burning");
        }
        if (Station->Type == EDCStationType::Rigging && Troubles && !Troubles->IsRiggingLoose())
        {
            return TEXT("standing by at deck repair lines; rigging currently secure");
        }
        return TEXT("working at ") + Post;
    }

    // Verified against taskboard/PIRATE-CAST.md and Saved/PirateCast/FinalReview/00-cast.png.
    // Display names changed; the assembled asset names remain Finch/Marlow/Odessa.
    const TCHAR* Appearance(const FString& Name)
    {
        if (Name.Equals(TEXT("Rex"), ESearchCase::IgnoreCase))
        { return TEXT("man with moustache and short beard, black tricorn, long dark gold-trimmed coat and fringed shoulders"); }
        if (Name.Equals(TEXT("Duncan"), ESearchCase::IgnoreCase))
        { return TEXT("man with moustache and short beard, red headband, faded red open-neck shirt, crossed leather straps and tall boots"); }
        if (Name.Equals(TEXT("Ruby"), ESearchCase::IgnoreCase))
        { return TEXT("woman with long auburn hair and headband, pale blouse, dark fitted bodice, belts, gloves and boots"); }
        if (Name.Equals(TEXT("Captain"), ESearchCase::IgnoreCase))
        { return TEXT("woman with auburn hair, feathered black tricorn, long dark buckled coat, shoulder guard, gloves and boots"); }
        return TEXT("not confirmed");
    }

    bool HasPirateAppearance(const AActor* Owner)
    {
        if (!Owner) { return false; }
        TArray<UChildActorComponent*> Children;
        Owner->GetComponents(Children);
        for (const UChildActorComponent* Child : Children)
        {
            if (Child && Child->GetFName() == FName(TEXT("PirateAppearance")) && IsValid(Child->GetChildActor())) { return true; }
        }
        return false;
    }

    bool HasVisibleCreature(UWorld* World)
    {
        for (TActorIterator<ADCLeviathan> It(World); It; ++It)
        {
            if (It->IsHidden()) { continue; }
            TArray<UPrimitiveComponent*> Primitives;
            It->GetComponents(Primitives);
            for (UPrimitiveComponent* Primitive : Primitives)
            {
                if (!Primitive || !Primitive->IsRegistered() || !Primitive->IsVisible() || Primitive->bHiddenInGame) { continue; }
                if (const USkeletalMeshComponent* Body = Cast<USkeletalMeshComponent>(Primitive))
                {
                    if (Body->GetSkeletalMeshAsset()) { return true; }
                }
                if (UProceduralMeshComponent* Arms = Cast<UProceduralMeshComponent>(Primitive))
                {
                    for (int32 Index = 0; Index < Arms->GetNumSections(); ++Index)
                    {
                        const FProcMeshSection* Section = Arms->GetProcMeshSection(Index);
                        if (Section && Section->bSectionVisible && Section->ProcIndexBuffer.Num() > 0) { return true; }
                    }
                }
            }
        }
        return false;
    }

    FString Scene(const ADCRideDirector& Director)
    {
        switch (Director.GetPhase())
        {
        case EDCPhase::OpenSea:
            return Director.bCloudWall && Director.StormGrowth > .3f
                ? TEXT("open sea; a bank of storm cloud is visible ahead") : TEXT("open sea beneath daylight");
        case EDCPhase::Fog:
            return Director.RideTime < 76.f ? TEXT("within a passage through rolling storm cloud")
                : TEXT("mist among the stone ruins and banks of a drowned city");
        case EDCPhase::Slide: return TEXT("descending through a tunnel of water");
        case EDCPhase::Hell: return TEXT("a fiery channel with an orange glow on the water");
        case EDCPhase::Breach: return TEXT("rushing through the bright opening at the end of the water tunnel");
        case EDCPhase::Maelstrom: return TEXT("a vast caldera lake enclosed by cliffs and falling water");
        case EDCPhase::Ending: return TEXT("open sea again in low golden light");
        default: return TEXT("location not established");
        }
    }
}

FString DCConversationContext::Persona(const FString& Speaker)
{
    FString Card;
    if (Speaker.Equals(TEXT("Rex"), ESearchCase::IgnoreCase))
    {
        Card = TEXT("You are Rex, the old helmsman: a Bristol coastal pilot turned pirate. Years ago you survived a winter wreck after a master ignored your soundings; you trust patience and good bearings over bravado. You keep scraps of tide notes and want to bring every hand home. Gruff, watchful, quietly kind; superstition is your personal belief, never proof of a curse. You respect Ruby's seamanship and mentor Duncan, grumbling when he rushes but defending him when it matters. Respect the captain's authority; give candid practical advice without inventing their past or feelings. Grounded British voice, short measured sentences, dry understatement; no theatrical pirate catchphrases. A calmer moment can reveal warmth, an old harbour memory or an interest in tides. Your usual role is helm, but the LIVE DUTY below overrides that role.");
    }
    else if (Speaker.Equals(TEXT("Duncan"), ESearchCase::IgnoreCase))
    {
        Card = TEXT("You are Duncan, the young Scottish gunner. You grew up beside a Clyde foundry and learned gunnery aboard a merchant escort before joining this crew. You are saving for a small boat of your own, though you spend too much of your share in port. Eager, competitive and quick with a boast; underneath you fear letting the crew down. You admire Rex's calm, enjoy trading jokes with Ruby, and want the captain to trust you with responsibility. Enthusiasm never means inventing a shot or ignoring hold fire. Natural Scottish phrasing with the occasional aye, readable English rather than phonetic spelling. Lively, concrete, good-humoured; when frightened you become more precise, not foolish. Talk about guns, metalwork, home or ambitions when asked. Your usual role is gunner, but the LIVE DUTY below overrides that role.");
    }
    else if (Speaker.Equals(TEXT("Ruby"), ESearchCase::IgnoreCase))
    {
        Card = TEXT("You are Ruby, the fearless, dry-witted bosun. You learned ropework in a Portsmouth dockyard and left a ship whose owner bought rotten cordage to save money. Competence and fair shares matter more to you than titles. You mend your own clothes, remember who pulls their weight, and quietly look after tired shipmates. Rex has earned your respect; Duncan's boasting earns a sharp joke, but you trust his hands at a gun. Be direct with the captain without taking away their decisions. Grounded British voice, crisp sentences and restrained sarcasm, never cruelty. Fearless means doing the necessary work despite fear, not claiming invulnerability. Speak freely about seamanship, fair dealing or small comforts ashore when asked. Your usual role is bosun, but the LIVE DUTY below overrides that role.");
    }
    else
    {
        return TEXT("No crew character card matches this speaker. Do not impersonate the player captain or invent another crew member.");
    }
    return Card + TEXT(" Speak only for yourself. Treat the captain as a person, not a source of orders alone. Personal memories are character backstory; weather, hazards and others' actions must come from current facts. Fires are smothered by swatting with a rag. Rigging is repaired at marked lines on deck; remain on deck.");
}

FString DCConversationContext::SceneKey(const UObject* WorldContext)
{
    if (!WorldContext || !WorldContext->GetWorld()) { return TEXT("unknown"); }
    const ADCRideDirector* Director = ADCRideDirector::Get(WorldContext);
    if (!Director) { return TEXT("unknown"); }
    return FString::Printf(TEXT("%d:%d"), static_cast<int32>(Director->GetPhase()),
        Director->GetPhase() == EDCPhase::Fog && Director->RideTime >= 76.f ? 1 : 0);
}

FString DCConversationContext::Build(const UObject* WorldContext, const FString& Speaker)
{
    using namespace DCConversationContextPrivate;
    check(IsInGameThread());
    FString Out = TEXT("SHIP'S COMPANY (character background, not live activity): Rex is the cautious old helmsman, Duncan the eager Scottish gunner, Ruby the practical dry-witted bosun. All three can change duties. The captain is the player and commands this pirate ship; their history, intentions and feelings are theirs to define.\n");
    UWorld* World = WorldContext ? WorldContext->GetWorld() : nullptr;
    if (!World) { return Out + TEXT("Live scene unavailable. Do not invent current weather, hazards, appearance or duties."); }
    const ADCRideDirector* Director = ADCRideDirector::Get(WorldContext);
    const ADCTroubles* Troubles = ADCTroubles::Get(WorldContext);
    const ADCGunnery* Guns = ADCGunnery::Get(WorldContext);
    UDCCrewSubsystem* Crew = World->GetSubsystem<UDCCrewSubsystem>();

    FString SelfAppearance = TEXT("not confirmed");
    Out += TEXT("LIVE DUTIES AND APPEARANCES (known shipmates, not a claim of direct line of sight):\n");
    if (Crew)
    {
        for (const ADCCrewMember* Member : Crew->GetCrew())
        {
            if (!IsValid(Member)) { continue; }
            const FDCStation* Station = Crew->GetStation(Member->StationIndex);
            if (Member->CrewName.Equals(Speaker, ESearchCase::IgnoreCase) && HasPirateAppearance(Member))
            {
                SelfAppearance = Appearance(Member->CrewName);
            }
            Out += FString::Printf(TEXT("%s%s: %s. Appearance: %s.\n"),
                *Member->CrewName, Member->CrewName.Equals(Speaker, ESearchCase::IgnoreCase) ? TEXT(" (you)") : TEXT(""),
                *Duty(Member, Station, Troubles, Guns), HasPirateAppearance(Member) ? Appearance(Member->CrewName) : TEXT("not confirmed"));
        }
    }
    else { Out += TEXT("Crew duty information unavailable.\n"); }
    const ADCPlayerCharacter* Captain = nullptr;
    for (TActorIterator<ADCPlayerCharacter> It(World); It; ++It) { Captain = *It; break; }
    FString CaptainDuty = Captain ? TEXT("on foot on deck") : TEXT("position unavailable");
    if (Crew && Captain)
    {
        if (Captain->IsAtStation()) { CaptainDuty = TEXT("at a station; exact station unconfirmed"); }
        for (const FDCStation& Station : Crew->GetStations())
        {
            if (Station.bPlayerOperated) { CaptainDuty = TEXT("operating the ") + StationName(&Station); break; }
        }
    }
    Out += FString::Printf(TEXT("Captain: %s. Appearance: %s.\n"), *CaptainDuty,
        HasPirateAppearance(Captain) ? Appearance(TEXT("Captain")) : TEXT("not confirmed"));
    Out += FString::Printf(TEXT("IDENTITY CHECK: You are %s, not the captain. YOUR own appearance is: %s. Keep each person's features separate.\n"), *Speaker, *SelfAppearance);

    if (Director)
    {
        const FDCEnvState& Env = Director->GetEnv();
        Out += FString::Printf(TEXT("CURRENT SCENE: %s. Hull condition %d%%.\n"), *Scene(*Director), FMath::RoundToInt(Director->ShipHealth));
        Out += FString::Printf(TEXT("CURRENT WEATHER STATE: %s; %s; %s; %s. Wind direction/speed unmeasured.\n"),
            Env.Rain > .5f ? TEXT("heavy rain") : Env.Rain > .05f ? TEXT("rain") : TEXT("no rain"),
            Env.Mist > .5f ? TEXT("thick mist") : Env.Mist > .1f ? TEXT("patches of mist") : TEXT("little sea mist"),
            Env.CloudCover > .75f ? TEXT("heavy cloud") : Env.CloudCover > .2f ? TEXT("broken cloud") : TEXT("mostly clear sky"),
            Env.SeaRoughness > 5.f ? TEXT("violent ship motion") : Env.SeaRoughness > 2.f ? TEXT("rough water") : TEXT("gentle swell"));
        if (Director->LightningNow > .1f) { Out += TEXT("Lightning is flashing now.\n"); }
        else if (Env.LightningRate > .1f) { Out += TEXT("Intermittent lightning in the current weather; no flash confirmed this instant.\n"); }
        if (const ADCTreadmill* Sea = ADCTreadmill::Get(WorldContext))
        {
            const float Turn = Sea->GetTurn();
            Out += Turn < -.1f ? TEXT("Ship currently turning to port.\n") : Turn > .1f
                ? TEXT("Ship currently turning to starboard.\n") : TEXT("Ship currently nearly steady.\n");
        }
        // This API excludes concealed/unannounced threats. Do not use a spawn list
        // or FindObstacleAhead: those can include scenery hidden behind the tunnel.
        if (Director->GetPhase() == EDCPhase::Fog)
        {
            Out += TEXT("NAVIGATION IN THIS SCENE: keep the ship in the clear channel and give stone banks, pillars and obstacles room. Never advise approaching the banks to see through fog.\n");
            if (const ADCEarlyEncounters* Encounters = ADCEarlyEncounters::Get(WorldContext))
            {
                const FString Threat = Encounters->GetThreatText();
                if (!Threat.IsEmpty()) { Out += TEXT("CURRENT VISIBLE THREAT: ") + Threat + TEXT(".\n"); }
            }
        }
        if (Director->GetPhase() == EDCPhase::Maelstrom && HasVisibleCreature(World))
        {
            Out += TEXT("A huge sea creature or its appendages are visible in the caldera.\n");
            for (TActorIterator<ADCBossCombat> It(World); It; ++It)
            {
                if (It->IsMouthCharging()) { Out += TEXT("CURRENT THREAT: the creature's mouth is charging an attack.\n"); }
                else
                {
                    FVector Base, Bend, Joint, Tip;
                    if (It->GetAppendagePose(Base, Bend, Joint, Tip)) { Out += TEXT("CURRENT THREAT: an attacking appendage is exposed.\n"); }
                }
                break;
            }
        }
    }
    else { Out += TEXT("Current location and weather unavailable.\n"); }
    if (Troubles)
    {
        Out += FString::Printf(TEXT("DECK: %d active fires. Rigging %s.\n"), Troubles->NumFires(),
            Troubles->IsRiggingLoose() ? TEXT("loose; needs repair at the marked lines on deck") : TEXT("secure"));
    }
    Out += TEXT("GENERAL MECHANICS (not a statement that a hazard is present): smother deck fires by swatting with a rag; repair rigging at marked lines on deck, without climbing. Wind is not measured: do not claim still air, calm wind or a wind direction. Describe only the recorded rain, mist, cloud and water conditions. Conversation itself changes no duties. Distinguish personal hunches or an ominous mood from the facts above. Never predict unseen encounters, invent a current hazard, or claim an action has happened merely because you said it.");
    return Out;
}
