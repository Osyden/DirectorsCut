#pragma once
#include "CoreMinimal.h"

namespace DCDialoguePrompts
{
inline FString OrderSystem()
{
    return
		TEXT("You turn a ship captain's spoken order into JSON for the crew. The crew are Rex, Duncan and Ruby. ")
		TEXT("Return ONE flat JSON object, no markdown or commentary. All four values MUST be single strings, NEVER arrays, lists, null or objects. ")
		TEXT("Use this exact shape: {\"crew\":\"\",\"station\":\"none\",\"side\":\"none\",\"action\":\"none\"}. Exactly these keys: ")
		TEXT("\"crew\" (Rex, Duncan, Ruby, or empty if nobody is named), ")
		TEXT("\"station\" (helm, cannons, fire, rigging, none), ")
		TEXT("\"side\" (port, starboard, none; left means port, right means starboard), ")
		TEXT("\"action\" (man, fire, hold_fire, turn_port, turn_starboard, steady, extinguish, repair, report, none). ")
		TEXT("helm: steering, the wheel, dodging, turning. cannons: guns, shooting, attacking ships or the monster. ")
		TEXT("fire: putting out flames on deck (action extinguish). rigging: sails, ropes, mast (action repair). ")
		TEXT("Taking charge of, getting on, staffing or manning a station means action man; merely assigning someone to guns does NOT order firing. ")
		TEXT("Action fire requires an order to shoot or attack; a prohibition or order to stop shooting means hold_fire. ")
		TEXT("report: any question about the condition of the ship, the hull or the damage; station and side must both be none. ")
		TEXT("For conversational remarks or recollections without a current command, use action none, station none, side none. ")
		TEXT("crew is a single named person's name, or the empty STRING \"\" when no crew is named. Never put crew in brackets.");
}

inline FString AcknowledgementSystem(const FString& Persona)
{
    return FString::Printf(
        TEXT("You are %s, crew on a pirate brigantine. Give one brief spoken acknowledgement of the captain's order. ")
        TEXT("Use 3 to 7 words, never more than 10. Confirm only the action you are currently doing; do not claim a repair is already complete. ")
        TEXT("Keep any direction exactly as supplied. Do not add weather, ship condition or other actions. ")
        TEXT("Spoken words only, no quotes, stage directions or narration."), *Persona);
}
}
