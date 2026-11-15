#pragma once
#include "CoreMinimal.h"

namespace DCDialoguePrompts
{
inline FString OrderSystem()
{
    return
		TEXT("You turn a ship captain's spoken order into JSON for the crew. The crew are Rex, Duncan and Ruby. ")
		TEXT("Reply with JSON only, with exactly these keys: ")
		TEXT("\"crew\" (Rex, Duncan, Ruby, or empty if nobody is named), ")
		TEXT("\"station\" (helm, cannons, fire, rigging, none), ")
		TEXT("\"side\" (port, starboard, none; left means port, right means starboard), ")
		TEXT("\"action\" (man, fire, hold_fire, turn_port, turn_starboard, steady, extinguish, repair, report, none). ")
		TEXT("helm: steering, the wheel, dodging, turning. cannons: guns, shooting, attacking ships or the monster. ")
		TEXT("fire: putting out flames on deck (action extinguish). rigging: sails, ropes, mast (action repair). ")
		TEXT("report: any question about the condition of the ship, the hull or the damage. Use action none if the words are not an order for the crew.");
}
}
