#include "ValhallaCombat.hpp"
#include "include/stunHandler.h"
#include "include/hitProcessor.h"
#include "include/reactionHandler.h"
#include "include/Utils.h"

using uniqueLocker = std::unique_lock<std::shared_mutex>;
using sharedLocker = std::shared_lock<std::shared_mutex>;

void stunHandler::safeErase_ActorStunDataMap(RE::FormID a_formID) {
	uniqueLocker lock(mtx_ActorStunDataMap);
	actorStunDataMap.erase(a_formID);
}
void stunHandler::safeErase_ActorStunDataMap(RE::Actor* a_actor) {
	if (a_actor) {
		safeErase_ActorStunDataMap(a_actor->GetFormID());
	}
}

void stunHandler::safeErase_StunBrokenActors(RE::FormID a_formID) {
	uniqueLocker lock(mtx_StunBrokenActors);
	stunBrokenActors.erase(a_formID);
}
void stunHandler::safeErase_StunBrokenActors(RE::Actor* a_actor) {
	if (a_actor) {
		safeErase_StunBrokenActors(a_actor->GetFormID());
	}
}

void stunHandler::safeInsert_StunBrokenActors(RE::FormID a_formID) {
	uniqueLocker lock(mtx_StunBrokenActors);
	stunBrokenActors.insert(a_formID);
}
void stunHandler::safeInsert_StunBrokenActors(RE::Actor* a_actor) {
	if (a_actor) {
		safeInsert_StunBrokenActors(a_actor->GetFormID());
	}
}

bool stunHandler::safeGet_isStunBroken(RE::FormID a_formID) {
	sharedLocker lock(mtx_StunBrokenActors);
	return stunBrokenActors.contains(a_formID);
}

bool stunHandler::safeGet_isStunBroken(RE::Actor* a_actor) {
	if (!a_actor) {
		return false;
	}
	return safeGet_isStunBroken(a_actor->GetFormID());
}

stunHandler::actorStunData_ptr stunHandler::safeInsert_ActorStunDataMap(RE::Actor* a_actor) {
	if (!a_actor) {
		return nullptr;
	}
	RE::FormID formID = a_actor->GetFormID();
	uniqueLocker lock(mtx_ActorStunDataMap);

	// Check again inside the unique lock to prevent race condition duplicates
	auto it = actorStunDataMap.find(formID);
	if (it != actorStunDataMap.end()) {
		return it->second;
	}

	actorStunData_ptr stunDataPtr(new actorStunData(a_actor));
	actorStunDataMap.insert({ formID, stunDataPtr });
	return stunDataPtr;
}

stunHandler::actorStunData_ptr stunHandler::safeGet_ActorStunData(RE::Actor* a_actor) {
	if (!a_actor) {
		return nullptr;
	}
	RE::FormID formID = a_actor->GetFormID();
	{
		sharedLocker lock(mtx_ActorStunDataMap);
		auto it = actorStunDataMap.find(formID);
		if (it != actorStunDataMap.end()) {
			return it->second;  //return found stundata
		}
	}

	return safeInsert_ActorStunDataMap(a_actor); //otherwise, insert and return inserted ptr
}

stunHandler::actorStunData_ptr stunHandler::trackStun(RE::Actor* a_actor) {
	return safeInsert_ActorStunDataMap(a_actor);
}

void stunHandler::untrackStun(RE::Actor* a_actor) {
	safeErase_ActorStunDataMap(a_actor);
	safeErase_StunBrokenActors(a_actor);
}

void stunHandler::refillStun(RE::Actor* a_actor) {
	if (!a_actor) {
		return;
	}
	sharedLocker lock(mtx_ActorStunDataMap);
	auto it = actorStunDataMap.find(a_actor->GetFormID());
	if (it == actorStunDataMap.end()) {
		return;
	}
	it->second->refillStun();
}

bool stunHandler::isEligibleForExecution(RE::Actor* a_actor) {
	if (!a_actor) {
		return false;
	}
	// Condition 1: Player Execution Guard
	if (a_actor->IsPlayerRef()) {
		if (!settings::bPlayerStunToggle || !settings::bPlayerExecutionToggle) {
			return false;
		}
	}
	// Condition 2: Teammate Execution Guard
	if (a_actor->IsPlayerTeammate()) {
		if (!settings::bFriendlyStunToggle) {
			return false;
		}
	}
	// Condition 3: Health Threshold Check (Only if bExecutionLimit is enabled)
	if (settings::bExecutionLimit) {
		float maxHealth = a_actor->AsActorValueOwner()->GetBaseActorValue(RE::ActorValue::kHealth);
		if (maxHealth > 0.0f) {
			float currentHealth = a_actor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kHealth);
			float healthPercentage = currentHealth / maxHealth;
			if (healthPercentage > settings::fExecutionHealthThreshold) {
				return false;
			}
		}
	}
	// Condition 4: Keyword Immunity Check (Always checked)
	if (data::executionImmunityKeyword && a_actor->HasKeyword(data::executionImmunityKeyword)) {
		return false;
	}

	return true;
}

bool stunHandler::getIsStunBroken(RE::Actor* a_actor) {
	return safeGet_isStunBroken(a_actor);
}

void stunHandler::onActorDeath(RE::Actor* a_actor) {
	if (!a_actor) {
		return;
	}

	RE::FormID formID = a_actor->GetFormID();

	// Check if this actor was actively being tracked by the stun system
	bool wasTracked = false;
	{
		sharedLocker lock(mtx_ActorStunDataMap);
		wasTracked = actorStunDataMap.contains(formID);
	}

	if (wasTracked) {
		// Revert HUD if TrueHUD override is not enabled
		if (!settings::facts::TrueHudAPI_PoiseControlsSpecialBar) {
			TrueHUDUtils::revertSpecialMeter(a_actor);
		}
		Utils::Actor::safeRemoveSpell(data::stunDebuffSpell, a_actor);
		safeErase_StunBrokenActors(formID);
		safeErase_ActorStunDataMap(formID);
	}
}

void stunHandler::update() {
	float deltaTime = *RE::Offset::g_deltaTime;

	// 1. Engine-Safe UI Flashing
	{
		static auto lastFlashTime = std::chrono::steady_clock::now();
		auto now = std::chrono::steady_clock::now();

		// Flash every 500ms
		if (std::chrono::duration_cast<std::chrono::milliseconds>(now - lastFlashTime).count() >= 500) {
			lastFlashTime = now;

			// Snapshot phase for flashing
			std::vector<RE::FormID> formsToFlash;
			{
				sharedLocker lock(mtx_StunBrokenActors);
				formsToFlash.reserve(stunBrokenActors.size());
				for (const auto& formID : stunBrokenActors) {
					formsToFlash.push_back(formID);
				}
			} // Lock released before engine/API calls

			// Processing phase
			for (const auto& formID : formsToFlash) {
				auto actor = RE::TESForm::LookupByID<RE::Actor>(formID);
				if (actor && isEligibleForExecution(actor)) {
					TrueHUDUtils::flashActorValue(actor, RE::ActorValue::kHealth);
				}
			}
		}
	}

	// 2. Snapshot Phase
	std::vector<std::pair<RE::FormID, actorStunData_ptr>> snapshot;
	{
		uniqueLocker lock(mtx_ActorStunDataMap);

		if (actorStunDataMap.empty()) {
			ValhallaCombat::GetSingleton()->deactivateUpdate(ValhallaCombat::stunHandler);
			return;
		}

		snapshot.reserve(actorStunDataMap.size());
		for (const auto& [formID, stunData] : actorStunDataMap) {
			snapshot.push_back({ formID, stunData });
		}
	}

	struct PendingAction {
		RE::Actor* actor = nullptr;
		bool recoverDowned = false;
		bool triggerDowned = false;
		bool removeSpell = false;
		bool revertHUD = false;
	};

	std::vector<PendingAction> actionsToProcess;
	std::vector<RE::FormID> formsToRemove;

	// 3. Evaluation Phase
	for (const auto& [formID, stunData] : snapshot) {
		auto actor = RE::TESForm::LookupByID<RE::Actor>(formID);
		if (!actor) {
			formsToRemove.push_back(formID);
			continue;
		}

		if (stunData->getRegenCountDown() > 0) {
			stunData->modRegenCountDown(-deltaTime);
			continue;
		}

		if (stunData->canRegen(actor)) {
			if (stunData->regenStun()) {
				stunData->setCombatRegeneration(false);

				PendingAction action{ actor };
				if (!settings::facts::TrueHudAPI_PoiseControlsSpecialBar) {
					action.revertHUD = true;
				}
				action.removeSpell = true;
				if (settings::bDownedStateToggle) {
					action.recoverDowned = true;
				}

				actionsToProcess.push_back(action);
				formsToRemove.push_back(formID);
				continue;
			}
		}

		// Mid-stun dynamic check
		if (safeGet_isStunBroken(formID) && isEligibleForExecution(actor)) {
			PendingAction action{ actor };
			if (settings::bDownedStateToggle) {
				action.triggerDowned = true;
			}
			actionsToProcess.push_back(action);
		}
	}

	// 4. Cleanup Phase
	if (!formsToRemove.empty()) {
		{
			std::scoped_lock lock(mtx_ActorStunDataMap, mtx_StunBrokenActors);
			for (const auto& formID : formsToRemove) {
				actorStunDataMap.erase(formID);
				stunBrokenActors.erase(formID);
			}
		}
	}

	// 5. Execution Phase
	for (const auto& action : actionsToProcess) {
		if (action.revertHUD) {
			TrueHUDUtils::revertSpecialMeter(action.actor);
		}
		if (action.removeSpell) {
			Utils::Actor::safeRemoveSpell(data::stunDebuffSpell, action.actor);
			if (settings::bAdvancedLogging) {
				logger::info("Successfully removed Stun Debuff Spell from actor: {:X}", action.actor->GetFormID());
			}
		}
		if (action.recoverDowned) {
			reactionHandler::recoverDownedState(action.actor);
		}
		if (action.triggerDowned) {
			reactionHandler::triggerDownedState(action.actor);
		}
	}
}

/*Get the max stun of a actor. If the actor is not tracked, return 1.*/
float stunHandler::getMaxStun(RE::Actor* a_actor) {
	if (!a_actor) return 1.0f;
	sharedLocker lock(mtx_ActorStunDataMap);
	auto it = actorStunDataMap.find(a_actor->GetFormID());
	if (it == actorStunDataMap.end()) {
		return 1.0f;
	}
	else {
		return it->second->getMaxStun();
	}
}

float stunHandler::getMaxStun_static(RE::Actor* a_actor) {
	return stunHandler::GetSingleton()->getMaxStun(a_actor);
}

/*Get the current stun of a actor. If the actor is not tracked, return 1.*/
float stunHandler::getCurrentStun(RE::Actor* a_actor) {
	if (!a_actor) return 1.0f;
	sharedLocker lock(mtx_ActorStunDataMap);
	auto it = actorStunDataMap.find(a_actor->GetFormID());
	if (it == actorStunDataMap.end()) {
		return 1.0f;
	}
	else {
		return it->second->getCurrentStun();
	}
}

float stunHandler::getCurrentStun_static(RE::Actor* a_actor) {
	return stunHandler::GetSingleton()->getCurrentStun(a_actor);
}

void stunHandler::onStunBroken(RE::Actor* a_actor) {
	// Acquire ActorStunData
	if (auto data = safeGet_ActorStunData(a_actor)) {
		data->setCombatRegeneration(true);
	}

	// Acquire StunBrokenActors
	safeInsert_StunBrokenActors(a_actor);

	ValhallaUtils::playSound(a_actor, data::soundStunBreak);

	//apply downed state
	if (settings::bDownedStateToggle) {
		if (!a_actor->IsInKillMove() && isEligibleForExecution(a_actor)) {
			reactionHandler::triggerDownedState(a_actor);
		}
	}

	//apply the stun debuff spell
	Utils::Actor::safeApplySpell(data::stunDebuffSpell, a_actor);
	if (settings::bAdvancedLogging) {
		logger::info("Successfully applied Stun Debuff Spell to actor: {:X}", a_actor->GetFormID());
	}

	//gray out this actor's stun meter.
	if (settings::bStunMeterToggle && !settings::facts::TrueHudAPI_PoiseControlsSpecialBar) {
		TrueHUDUtils::greyOutSpecialMeter(a_actor);
	}
}

void stunHandler::damageStun(RE::Actor* a_aggressor, RE::Actor* a_victim, float a_damage) {
	(void)a_aggressor;
	if (auto data = safeGet_ActorStunData(a_victim)) {
		if (data->damageStun(a_damage)) {
			if (!safeGet_isStunBroken(a_victim)) { //the current damage depletes actor stun, but actor's stunned state is not yet tracked.
				onStunBroken(a_victim);
			}
		}
	}
	ValhallaCombat::GetSingleton()->activateUpdate(ValhallaCombat::stunHandler);
}

void stunHandler::processStunDamage(STUNSOURCE a_stunSource, RE::TESObjectWEAP* a_weapon, RE::Actor* a_aggressor, RE::Actor* a_victim, float a_baseDamage) {
	if (!settings::bStunToggle) { //stun damage will not be applied with stun turned off.
		return;
	}
	if (a_victim->IsPlayerRef() && !settings::bPlayerStunToggle) { //player does not receive stun damage.
		return;
	}
	if (a_victim->IsPlayerTeammate() && !settings::bFriendlyStunToggle) { //player's team do not receive stun damage.
		return;
	}
	if (safeGet_isStunBroken(a_victim)) {
		return;
	}
	float stunDamage = 0;
	switch (a_stunSource) {
	case STUNSOURCE::timedBlock:
		stunDamage = a_baseDamage * settings::fStunTimedBlockMult;
		break;
	case STUNSOURCE::bash:
		stunDamage = a_aggressor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kBlock) * settings::fStunBashMult;
		inlineUtils::offsetRealDamage(stunDamage, a_aggressor, a_victim);
		break;
	case STUNSOURCE::powerBash:
		stunDamage = a_aggressor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kBlock) * settings::fStunPowerBashMult;
		inlineUtils::offsetRealDamage(stunDamage, a_aggressor, a_victim);
		break;
	case STUNSOURCE::parry:
		stunDamage = a_aggressor->AsActorValueOwner()->GetActorValue(RE::ActorValue::kBlock) * settings::fStunParryMult;
		inlineUtils::offsetRealDamage(stunDamage, a_aggressor, a_victim);
		break;
	case STUNSOURCE::lightAttack:
		stunDamage = a_baseDamage * settings::fStunNormalAttackMult;
		if (!a_weapon) {
			stunDamage *= settings::fStunUnarmedMult;
		}
		else {
			switch (a_weapon->GetWeaponType()) {
			case RE::WEAPON_TYPE::kHandToHandMelee: stunDamage *= settings::fStunUnarmedMult; break;
			case RE::WEAPON_TYPE::kOneHandDagger: stunDamage *= settings::fStunDaggerMult; break;
			case RE::WEAPON_TYPE::kOneHandSword: stunDamage *= settings::fStunSwordMult; break;
			case RE::WEAPON_TYPE::kOneHandAxe: stunDamage *= settings::fStunWarAxeMult; break;
			case RE::WEAPON_TYPE::kOneHandMace: stunDamage *= settings::fStunMaceMult; break;
			case RE::WEAPON_TYPE::kTwoHandAxe: stunDamage *= settings::fStun2HBluntMult; break;
			case RE::WEAPON_TYPE::kTwoHandSword: stunDamage *= settings::fStunGreatSwordMult; break;
			}
		}
		break;
	case STUNSOURCE::powerAttack:
		stunDamage = a_baseDamage * settings::fStunPowerAttackMult;
		if (!a_weapon) {
			stunDamage *= settings::fStunUnarmedMult;
		}
		else {
			switch (a_weapon->GetWeaponType()) {
			case RE::WEAPON_TYPE::kHandToHandMelee: stunDamage *= settings::fStunUnarmedMult; break;
			case RE::WEAPON_TYPE::kOneHandDagger: stunDamage *= settings::fStunDaggerMult; break;
			case RE::WEAPON_TYPE::kOneHandSword: stunDamage *= settings::fStunSwordMult; break;
			case RE::WEAPON_TYPE::kOneHandAxe: stunDamage *= settings::fStunWarAxeMult; break;
			case RE::WEAPON_TYPE::kOneHandMace: stunDamage *= settings::fStunMaceMult; break;
			case RE::WEAPON_TYPE::kTwoHandAxe: stunDamage *= settings::fStun2HBluntMult; break;
			case RE::WEAPON_TYPE::kTwoHandSword: stunDamage *= settings::fStunGreatSwordMult; break;
			}
		}
		break;
	}
	if (stunDamage != 0) { //only damage when there is damage.
		damageStun(a_aggressor, a_victim, stunDamage);
	}
}

void stunHandler::modStun(RE::Actor* a_actor, float a_mod)
{
	if (auto data = safeGet_ActorStunData(a_actor)) {
		data->modStun(a_mod);
	}
}

robin_hood::unordered_set<RE::FormID> stunHandler::getStunBrokenActors() {
	sharedLocker lock(mtx_StunBrokenActors);
	return stunBrokenActors;
}

void stunHandler::reset() {
	std::vector<RE::FormID> formsToRevert;

	{
		std::scoped_lock lock(mtx_ActorStunDataMap, mtx_StunBrokenActors);

		formsToRevert.reserve(stunBrokenActors.size());
		for (const auto& formID : stunBrokenActors) {
			formsToRevert.push_back(formID);
		}

		actorStunDataMap.clear();
		stunBrokenActors.clear();
	} // Both locks released here safely

	// Perform engine lookups and side effects outside the locks
	for (const auto& formID : formsToRevert) {
		if (auto actor = RE::TESForm::LookupByID<RE::Actor>(formID)) {
			if (!settings::facts::TrueHudAPI_PoiseControlsSpecialBar) {
				TrueHUDUtils::revertSpecialMeter(actor);
			}
			Utils::Actor::safeRemoveSpell(data::stunDebuffSpell, actor);
		}
	}
}