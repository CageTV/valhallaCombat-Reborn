#include "include/events.h"
#include "include/AI.h"

EventResult combatEventHandler::ProcessEvent(const RE::TESCombatEvent* a_event, RE::BSTEventSource<RE::TESCombatEvent>*)
{
	if (!a_event || !a_event->actor || !a_event->targetActor) {
		return EventResult::kContinue;
	}

	auto actor = a_event->actor->As<RE::Actor>();
	auto target = a_event->targetActor->As<RE::Actor>();

	if (actor && target) {
		// Forward the combat state change to your AI manager
		AI::GetSingleton()->ProcessCombatState(actor, target, a_event->newState.get());
	}

	return EventResult::kContinue;
}