#include "include/reactionHandler.h"
#include "include/debuffHandler.h"
#include "include/settings.h"
#include "include/Utils.h"
#include "include/data.h"
#include "include/offsets.h"

static inline const RE::BSFixedString poise_largest = "poise_largest_start";
static inline const RE::BSFixedString poise_largest_fwd = "poise_largest_start_fwd";
static inline const RE::BSFixedString poise_large = "poise_med_start";
static inline const RE::BSFixedString poise_large_fwd = "poise_med_start_fwd";
static inline const RE::BSFixedString poise_med = "poise_med_start";
static inline const RE::BSFixedString poise_med_fwd = "poise_med_start_fwd";
static inline const RE::BSFixedString poise_small = "poise_small_start";
static inline const RE::BSFixedString poise_small_fwd = "poise_small_start_fwd";

static inline const RE::BSFixedString staggerDirection = "staggerDirection";
static inline const RE::BSFixedString StaggerMagnitude = "StaggerMagnitude";
static inline const RE::BSFixedString staggerStart = "staggerStart";
static inline const RE::BSFixedString staggerStop = "staggerStop";
static inline const RE::BSFixedString bleedOutStart = "BleedoutStart";
static inline const RE::BSFixedString bleedOutStop = "BleedOutStop";
static inline const RE::BSFixedString bleedOutGraphBool = "IsBleedingOut";

static inline const RE::BSFixedString recoilLargeStart = "recoilLargeStart";

namespace {

	// Helper to sample active block state and trigger guard break lockout prior to stagger execution
	void checkAndApplyGuardBreakLockout(RE::Actor* a_reactor) {
		if (!a_reactor || a_reactor->IsPlayerRef()) {
			return;
		}

		bool inDebuff = debuffHandler::GetSingleton()->isInDebuff(a_reactor);
		if (inDebuff) {
			bool isLockedOut = debuffHandler::GetSingleton()->isGuardBreakLockedOut(a_reactor);
			if (Utils::Actor::isBlocking(a_reactor)) {
				if (!isLockedOut) {
					if (settings::bAdvancedLogging) {
						logger::info("Applying guard break lockout to reactor {:X}", a_reactor->GetFormID());
					}
					debuffHandler::GetSingleton()->startGuardBreakLockout(a_reactor);
				}
			}
		}
	}
}

void reactionHandler::triggerStagger(RE::Actor* a_aggressor, RE::Actor* a_reactor, float a_reactionMagnitude) {
	if (!a_reactor) {
		return;
	}

	checkAndApplyGuardBreakLockout(a_reactor);

	float direction = 0.0f;
	if (a_aggressor) {
		auto headingAngle = a_reactor->GetHeadingAngle(a_aggressor->GetPosition(), false);
		direction = (headingAngle >= 0.0f) ? headingAngle / 360.0f : (360.0f + headingAngle) / 360.0f;
	}
	a_reactor->SetGraphVariableFloat(staggerDirection, direction);
	a_reactor->SetGraphVariableFloat(StaggerMagnitude, a_reactionMagnitude);
	Utils::Actor::performAction(data::actionStaggerStart, a_reactor);
	//RE::Offset::triggerNativeStagger(a_reactor, a_reactionMagnitude, a_aggressor);
	//a_reactor->NotifyAnimationGraph(staggerStart);
}

void reactionHandler::triggerRecoil(RE::Actor* a_reactor) {
	if (!a_reactor) {
		return;
	}
	Utils::Actor::performAction(data::actionRecoilLarge, a_reactor);
	//a_reactor->NotifyAnimationGraph(recoilLargeStart);
}

void reactionHandler::triggerPoiseReaction(RE::Actor* a_aggressor, RE::Actor* a_reactor, reactionType a_reactionType) {
	if (!a_reactor) {
		return;
	}

	checkAndApplyGuardBreakLockout(a_reactor);

	RE::BSFixedString reaction;
	bool isBackFacing = (a_aggressor != nullptr) && Utils::Actor::isBackFacing(a_reactor, a_aggressor);

	if (isBackFacing) {
		switch (a_reactionType) {
		case reactionType::kSmall: reaction = poise_small_fwd; break;
		case reactionType::kMedium: reaction = poise_med_fwd; break;
		case reactionType::kLarge: reaction = poise_large_fwd; break;
		case reactionType::kLargest: reaction = poise_largest_fwd; break;
		}
	}
	else {
		switch (a_reactionType) {
		case reactionType::kSmall: reaction = poise_small; break;
		case reactionType::kMedium: reaction = poise_med; break;
		case reactionType::kLarge: reaction = poise_large; break;
		case reactionType::kLargest: reaction = poise_largest; break;
		}
	}
	a_reactor->NotifyAnimationGraph(reaction);
}

void reactionHandler::applyStagger(RE::Actor* a_causer, RE::Actor* a_reactor, reactionType a_reactionType) {
	if (!a_reactor || a_reactor->AsActorState()->IsSwimming() || a_reactor->IsInKillMove() || !a_reactor->Is3DLoaded()|| Utils::Actor::isJumping(a_reactor)) {
		return;
	}
	if (settings::bPoiseCompatibility
		&& data::isRaceType(a_reactor, data::raceCatagory::Humanoid)) {
		triggerPoiseReaction(a_causer, a_reactor, a_reactionType);
	}
	else {
		switch (a_reactionType) {
		case kSmall: triggerStagger(a_causer, a_reactor, settings::fReactionTypeSmallTHLD); break;
		case kMedium: triggerStagger(a_causer, a_reactor, settings::fReactionTypeMediumTHLD); break;
		case kLarge: triggerStagger(a_causer, a_reactor, settings::fReactionTypeLargeTHLD); break;
		case kLargest: triggerStagger(a_causer, a_reactor, settings::fReactionTypeLargestTHLD); break;
		}
	}

}

void reactionHandler::triggerDownedState(RE::Actor* a_actor) {
	if (!a_actor || a_actor->AsActorState()->IsSwimming() || a_actor->IsInKillMove() || !a_actor->Is3DLoaded()) {
		return;
	}
	bool isAlreadyBleedingOut = false;
	if (a_actor->GetGraphVariableBool(bleedOutGraphBool, isAlreadyBleedingOut) && isAlreadyBleedingOut) {
		return;
	}
	auto race = a_actor->GetRace();
	if (data::raceMapping.contains(race)) {
		switch (data::raceMapping[race]) {
		case data::Humanoid:
		case data::Undead:
			if (a_actor->IsInKillMove()) {
				return;
			}
			a_actor->SetGraphVariableBool(bleedOutGraphBool, true);
			a_actor->NotifyAnimationGraph(bleedOutStart);
		}
	}
}

void reactionHandler::recoverDownedState(RE::Actor* a_actor) {
	if (!a_actor || a_actor->AsActorState()->IsSwimming() || a_actor->IsInKillMove() || !a_actor->Is3DLoaded()) {
		return;
	}
	auto race = a_actor->GetRace();
	if (data::raceMapping.contains(race)) {
		switch (data::raceMapping[race]) {
		case data::Humanoid:
		case data::Undead:
			if (a_actor->IsInKillMove()) {
				return;
			}
			a_actor->SetGraphVariableBool(bleedOutGraphBool, false);
			a_actor->NotifyAnimationGraph(bleedOutStop);
		}
	}
}
