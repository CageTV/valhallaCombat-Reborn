#include "include/debuffHandler.h"
#include "include/data.h"
#include "include/settings.h"
#include "include/offsets.h"
#include "include/Utils.h"
#include "valhallaCombat.hpp"

using uniqueLocker = std::unique_lock<std::shared_mutex>;
using sharedLocker = std::shared_lock<std::shared_mutex>;

void debuffHandler::applyMovementRestrictions(RE::Actor* a_actor) {
	if (!a_actor || !settings::bExhaustionBlockSprintRun) {
		return;
	}

	auto actorState = a_actor->AsActorState();
	if (!actorState) {
		return;
	}

	if (actorState->IsSprinting()) {
		a_actor->NotifyAnimationGraph("sprintStop");
	}
	a_actor->SetGraphVariableBool("bSprintOK", false);

	Utils::Actor::safeApplySpell(data::exhaustionDebuffSpell_2, a_actor);
	if (settings::bAdvancedLogging) {
		logger::info("Successfully applied Exhaustion Debuff Spell (Movement Restrictions) to actor: {:X}", a_actor->GetFormID());
	}
}

void debuffHandler::clearMovementRestrictions(RE::Actor* a_actor) {
	if (!a_actor || !settings::bExhaustionBlockSprintRun) {
		return;
	}
	a_actor->SetGraphVariableBool("bSprintOK", true);
	Utils::Actor::safeRemoveSpell(data::exhaustionDebuffSpell_2, a_actor);
	if (settings::bAdvancedLogging) {
		logger::info("Successfully removed Exhaustion Debuff Spell (Movement Restrictions) from actor: {:X}", a_actor->GetFormID());
	}
}

bool debuffHandler::processExhaustionAnimEvent(RE::Actor* a_actor, const RE::BSFixedString& a_eventName, bool& a_outShouldProcess)
{
	if (!a_actor || !isInDebuff(a_actor)) {
		return false;
	}

	std::string_view eventStr = a_eventName.c_str();
	if (eventStr.empty()) {
		return false;
	}

	// Fast lowercase transformation
	std::string lowerStr = inlineUtils::to_lower_string(eventStr);

	bool isBash = (lowerStr == "bashstart" || lowerStr == "bashpowerstart");
	bool isPowerAttack = (lowerStr.find("attackpowerstart") != std::string::npos) ||
		(lowerStr.find("attackstart") != std::string::npos && lowerStr.find("power") != std::string::npos);

	// Player Exhaustion blocks Bashes and Power Attacks
	if (isBash || isPowerAttack) {
		a_outShouldProcess = false;
		return true;
	}

	return false;
}

bool debuffHandler::processExhaustionAnimEventNPC(RE::Actor* a_actor, const RE::BSFixedString& a_eventName, bool& a_outShouldProcess)
{
	if (!a_actor || !isInDebuff(a_actor)) {
		return false;
	}

	std::string_view eventStr = a_eventName.c_str();
	if (eventStr.empty()) {
		return false;
	}

	std::string lowerStr = inlineUtils::to_lower_string(eventStr);

	bool isBash = (lowerStr == "bashstart" || lowerStr == "bashpowerstart");
	bool isAnyAttack = (lowerStr.find("attackstart") != std::string::npos) ||
		(lowerStr.find("attackpowerstart") != std::string::npos);

	// NPC Exhaustion blocks Bashes, Light Attacks, and Power Attacks
	if (isBash || isAnyAttack) {
		a_outShouldProcess = false;
		return true;
	}

	return false;
}

bool debuffHandler::suppressExhaustionAnimEvent(RE::Actor* a_actor, const RE::BSFixedString& a_eventName, bool& a_outShouldProcess)
{
	if (!a_actor || !isInDebuff(a_actor)) {
		return false;
	}

	// Check against the map/set configured in settings
	if (settings::suppressedExhaustionAnimEvents.contains(a_eventName)) {
		a_outShouldProcess = false; // Sink/block event
		return true;               // Handled
	}

	return false;
}

bool debuffHandler::ShouldSinkEvent(RE::Actor* a_actor, const RE::BSFixedString& a_eventName)
{
	if (!a_actor) {
		return false;
	}

	if (a_eventName == "blockStart") {
		bool lockedOut = isGuardBreakLockedOut(a_actor);
		if (settings::bAdvancedLogging && lockedOut) {
			logger::info("Sinking 'blockStart' for locked-out actor {:X}", a_actor->GetFormID());
		}
		return lockedOut;
	}

	return false;
}

/*Called every frame.
Iterate through the set of actors debuffing.
Check the actors' stamina. If the actor's stamina has fully recovered, remove the actor from the set.
Check the actor's UI counter, if the counter is less than 0, flash the actor's UI.*/
void debuffHandler::update()
{
	// 1. Thread-safe UI Flashing
	if (pcStaminaMeterFlash_b) {
		static auto lastFlashTime = std::chrono::steady_clock::now();
		auto now = std::chrono::steady_clock::now();

		if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFlashTime).count() >= 500) {
			lastFlashTime = now;
			if (auto pc = RE::PlayerCharacter::GetSingleton()) {
				TrueHUDUtils::flashActorValue(pc, RE::ActorValue::kStamina);
			}
		}
	}
	float deltaTime = *RE::Offset::g_deltaTime;
	// 2. Decrement Guard Break Lockout Timers
	{
		uniqueLocker lockoutLock(mtx_guardBreakLockouts);
		for (auto it = guardBreakLockouts.begin(); it != guardBreakLockouts.end(); ) {
			it->second -= deltaTime;
			if (it->second <= 0.0f) {
				//logger::info("[Update] Guard break lockout EXPIRED for actor {:X}", it->first);
				it = guardBreakLockouts.erase(it); // Timer expired, clean up
			}
			else {
				++it;
			}
		}
	}
	// 3. Decrement Exhaustion Block Timers
	std::vector<RE::FormID> expiredBlockers;
	{
		uniqueLocker blockTimerLock(mtx_exhaustionBlockTimers);
		for (auto it = exhaustionBlockTimers.begin(); it != exhaustionBlockTimers.end(); ) {
			it->second -= deltaTime;
			if (it->second <= 0.0f) {
				expiredBlockers.push_back(it->first);
				it = exhaustionBlockTimers.erase(it); // Expired, erase from tracking
			}
			else {
				++it;
			}
		}
	}

	// 4. Force NPCs who held block for too long to stop
	for (const auto& formID : expiredBlockers) {
		if (auto actor = RE::TESForm::LookupByID<RE::Actor>(formID)) {
			if (Utils::Actor::isBlocking(actor)) {
				//logger::info("Forcing 'blockStop' and starting lockout for actor {:X}", formID);
				actor->NotifyAnimationGraph("blockStop");
				startGuardBreakLockout(actor);
			}
		}
	}

	// 5. Snapshot phase: Copy FormIDs under a quick read lock
	std::vector<RE::FormID> formsToProcess;
	{
		sharedLocker debuffLock(mtx_actorInDebuff);
		if (actorInDebuff.empty()) {
			ValhallaCombat::GetSingleton()->deactivateUpdate(ValhallaCombat::debuffHandler);
			return;
		}
		formsToProcess.reserve(actorInDebuff.size());
		for (const auto& formID : actorInDebuff) {
			formsToProcess.push_back(formID);
		}
	}
	// Lock released

	std::vector<RE::FormID> formsToRemove;

	// 6. Processing phase: Safe to make any engine/anim calls outside the lock
	for (const auto& formID : formsToProcess) {

		auto actor = RE::TESForm::LookupByID<RE::Actor>(formID);

		// Fallback for actors that despawned while in a debuffed state
		if (!actor || !actor->Is3DLoaded()) {
			formsToRemove.push_back(formID);
			continue;
		}

		// Check stamina recovery
		float current = actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kStamina);
		float effectiveMax =
			actor->AsActorValueOwner()->GetPermanentActorValue(RE::ActorValue::kStamina) +
			actor->GetActorValueModifier(RE::ACTOR_VALUE_MODIFIER::kTemporary, RE::ActorValue::kStamina);

		bool clearDebuff = false;

		if (actor->IsPlayerRef() && data::incrementalFatigueCap) {
			float cap = std::clamp(data::incrementalFatigueCap->value, 0.05f, 1.0f);
			clearDebuff = current >= (effectiveMax * cap);
		}
		else {
			clearDebuff = current >= effectiveMax;
		}

		if (clearDebuff) {
			clearMovementRestrictions(actor);
			revertDebuffUI(actor);
			if (!settings::bExhaustionBlockSprintRun) {
				Utils::Actor::safeRemoveSpell(data::exhaustionDebuffSpell_1, actor);
				if (settings::bAdvancedLogging) {
					logger::info("Successfully removed Exhaustion Debuff Spell from actor: {:X}", actor->GetFormID());
				}
			}

			formsToRemove.push_back(formID);
		}
	}

	// 7. Cleanup phase: Remove recovered actors under a brief write lock
	if (!formsToRemove.empty()) {
		std::scoped_lock doubleLock(mtx_actorInDebuff, mtx_guardBreakLockouts, mtx_exhaustionBlockTimers);
		for (const auto& formID : formsToRemove) {
			actorInDebuff.erase(formID);
			guardBreakLockouts.erase(formID);
			exhaustionBlockTimers.erase(formID);
		}
	}
}

void debuffHandler::startGuardBreakLockout(RE::Actor* a_actor) {
	if (!a_actor) {
		return;
	}
	uniqueLocker lock(mtx_guardBreakLockouts);
	guardBreakLockouts[a_actor->GetFormID()] = settings::fBlockLockoutDuration_NPC;
}

void debuffHandler::onBlockStart(RE::Actor* a_actor) {
	if (!a_actor) {
		return;
	}
	uniqueLocker lock(mtx_exhaustionBlockTimers);
	exhaustionBlockTimers[a_actor->GetFormID()] = settings::fExhaustionBlockDuration_NPC;
}

void debuffHandler::onBlockStop(RE::Actor* a_actor) {
	if (!a_actor) {
		return;
	}
	uniqueLocker lock(mtx_exhaustionBlockTimers);
	exhaustionBlockTimers.erase(a_actor->GetFormID());
}

void debuffHandler::initStaminaDebuff(RE::Actor* a_actor) {
	if (!a_actor || !settings::bStaminaDebuffToggle) {
		return;
	}

	RE::FormID formID = a_actor->GetFormID();
	{
		uniqueLocker lock(mtx_actorInDebuff);
		if (actorInDebuff.contains(formID)) {
			return;
		}
		actorInDebuff.insert(formID);
	}

	if (!settings::bExhaustionBlockSprintRun) {
		Utils::Actor::safeApplySpell(data::exhaustionDebuffSpell_1, a_actor);
		if (settings::bAdvancedLogging) {
			logger::info("Successfully applied Exhaustion Debuff Spell to actor: {:X}", a_actor->GetFormID());
		}
	}

	applyMovementRestrictions(a_actor);

	// Catch actors who were holding block before entering exhaustion
	if (!a_actor->IsPlayerRef()) {
		if (Utils::Actor::isBlocking(a_actor)) {
			onBlockStart(a_actor);
		}
	}

	if (settings::bUIAlert) {
		initDebuffUI(a_actor);
	}

	ValhallaCombat::GetSingleton()->activateUpdate(ValhallaCombat::HANDLER::debuffHandler);
}

void debuffHandler::initDebuffUI(RE::Actor* a_actor) {
	TrueHUDUtils::greyoutAvMeter(a_actor, RE::ActorValue::kStamina);
	if (a_actor->IsPlayerRef()) {
		pcStaminaMeterFlash_b = true;
	}
}

void debuffHandler::revertDebuffUI(RE::Actor* a_actor) {
	if (!settings::bUIAlert) {
		return;
	}
	if (a_actor->IsPlayerRef()) {
		pcStaminaMeterFlash_b = false;
	}
	TrueHUDUtils::revertAvMeter(a_actor, RE::ActorValue::kStamina);
}

void debuffHandler::stopDebuff(RE::FormID a_formID) {
	std::scoped_lock doubleLock(mtx_actorInDebuff, mtx_guardBreakLockouts, mtx_exhaustionBlockTimers);
	actorInDebuff.erase(a_formID);
	guardBreakLockouts.erase(a_formID);
	exhaustionBlockTimers.erase(a_formID);
}

void debuffHandler::stopDebuff(RE::Actor* a_actor) {
	if (!a_actor) {
		return;
	}

	// Fast set removal
	stopDebuff(a_actor->GetFormID());

	// Physical engine removal
	clearMovementRestrictions(a_actor);
	revertDebuffUI(a_actor);

	if (!settings::bExhaustionBlockSprintRun) {
		Utils::Actor::safeRemoveSpell(data::exhaustionDebuffSpell_1, a_actor);
		if (settings::bAdvancedLogging) {
			logger::info("Successfully removed Exhaustion Debuff Spell from actor: {:X}", a_actor->GetFormID());
		}
	}
}

bool debuffHandler::isInDebuff(RE::FormID a_formID) {
	sharedLocker lock(mtx_actorInDebuff);
	return actorInDebuff.contains(a_formID);
}

bool debuffHandler::isInDebuff(RE::Actor* a_actor) {
	if (!a_actor) {
		return false;
	}
	return isInDebuff(a_actor->GetFormID());
}

bool debuffHandler::isGuardBreakLockedOut(RE::FormID a_formID) {
	sharedLocker lock(mtx_guardBreakLockouts);
	auto it = guardBreakLockouts.find(a_formID);
	return it != guardBreakLockouts.end() && it->second > 0.0f;
}

bool debuffHandler::isGuardBreakLockedOut(RE::Actor* a_actor) {
	if (!a_actor) {
		return false;
	}
	return isGuardBreakLockedOut(a_actor->GetFormID());
}

void debuffHandler::onActorDeath(RE::Actor* a_actor) {
	if (!a_actor) {
		return;
	}

	// If the actor was actively in a debuff state when they died, instantly stop it.
	if (isInDebuff(a_actor)) {
		stopDebuff(a_actor);
	}
}

void debuffHandler::reset() {
	std::vector<RE::FormID> formsToRevert;
	{
		std::scoped_lock doubleLock(mtx_actorInDebuff, mtx_guardBreakLockouts, mtx_exhaustionBlockTimers);

		formsToRevert.reserve(actorInDebuff.size());
		for (const auto& formID : actorInDebuff) {
			formsToRevert.push_back(formID);
		}

		actorInDebuff.clear();
		guardBreakLockouts.clear();
		exhaustionBlockTimers.clear();
	} // Locks released here

	for (const auto& formID : formsToRevert) {

		if (auto actor = RE::TESForm::LookupByID<RE::Actor>(formID)) {
			clearMovementRestrictions(actor);
			revertDebuffUI(actor);

			if (!settings::bExhaustionBlockSprintRun) {
				Utils::Actor::safeRemoveSpell(data::exhaustionDebuffSpell_1, actor);
				if (settings::bAdvancedLogging) {
					logger::info("Successfully removed Exhaustion Debuff Spell from actor on reset: {:X}", actor->GetFormID());
				}
			}
		}
	}
}
