#include "include/events.h"
#include "include/attackHandler.h"
#include "include/blockHandler.h"
#include "include/parryHandler.h"
#include "include/stunHandler.h"
#include "include/debuffHandler.h"
#include "include/poiseHandler.h"
#include "include/AI.h"
#include "include/specialAttackHandler.h"

EventResult actorDeathEventHandler::ProcessEvent(const RE::TESDeathEvent* a_event, RE::BSTEventSource<RE::TESDeathEvent>* a_eventSource)
{
	(void)a_eventSource;

	if (!a_event || !a_event->actorDying) {
		return EventResult::kContinue;
	}

	auto actor = a_event->actorDying.get()->As<RE::Actor>();
	if (actor) {
		// Clear the dead actor from the attack context cache
		attackHandler::GetSingleton()->onActorDeath(actor);
		blockHandler::GetSingleton()->onActorDeath(actor);
		BlockCommitment::GetSingleton()->onActorDeath(actor);
		parryHandler::GetSingleton()->onActorDeath(actor);
		stunHandler::GetSingleton()->onActorDeath(actor);
		debuffHandler::GetSingleton()->onActorDeath(actor);
		poiseHandler::GetSingleton()->onActorDeath(actor);
		AI::GetSingleton()->onActorDeath(actor);
		SpecialAttackHandler::GetSingleton()->onActorDeath(actor);
	}

	return EventResult::kContinue;
}
