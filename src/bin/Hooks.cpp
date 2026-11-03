#include "include/Hooks.h"
#include "ValhallaCombat.hpp"
#include "include/stunHandler.h"
#include "include/hitProcessor.h"
#include "include/AI.h"
#include "include/AILearning.h"
#include "include/settings.h"
#include "include/attackHandler.h"
#include "include/StaggerHandler.h"
#include "include/blockHandler.h"
#include "include/debuffHandler.h"
#include "include/parryHandler.h"
#include "include/perks.h"
#include "include/specialAttackHandler.h"

inline bool isAttackStartEvent(std::string_view a_event) {
	if (a_event.empty()) {
		return false;
	}

	// Quick early check: skip non-attack events instantly
	if (a_event[0] != 'a' && a_event[0] != 'A') {
		return false;
	}

	std::string lowerStr = inlineUtils::to_lower_string(a_event);

	// Matches any event containing lowercase "attackstart" or "attackpowerstart"
	return lowerStr.find("attackstart") != std::string::npos ||
		lowerStr.find("attackpowerstart") != std::string::npos;
}

namespace Hooks
{
#pragma region GetStaminaCost

	float Hook_OnGetAttackStaminaCost::getAttackStaminaCost(RE::ActorValueOwner* avOwner, RE::BGSAttackData* atkData)
	{
		auto* actor = skyrim_cast<RE::Actor*>(avOwner);
		if (!actor) {
			return _getAttackStaminaCost(avOwner, atkData);
		}

		if (!settings::bNonCombatStaminaCost && !actor->IsInCombat()) {
			return 0.0f;
		}

		const bool isPowerAttack = Utils::Actor::isPowerAttacking(actor);
		const bool isPowerBash = Utils::Actor::isPowerBashing(actor);
		const bool isBash = Utils::Actor::isBashing(actor);

		if (isPowerAttack) {

			if (settings::bUseCustomPowerAttackCost) {
				return 0.0f;
			}

			return _getAttackStaminaCost(avOwner, atkData);
		}


		if (isPowerBash) {

			if (settings::bUseCustomPowerBashCost) {
				return 0.0f;
			}

			return _getAttackStaminaCost(avOwner, atkData);
		}


		if (isBash) {

			if (settings::bUseCustomLightBashCost) {
				return 0.0f;
			}

			return _getAttackStaminaCost(avOwner, atkData);
		}

		return _getAttackStaminaCost(avOwner, atkData);
	}
#pragma endregion

#pragma region StaminaRegen
	void Hook_OnRestoreActorValue::RestoreActorValue(RE::Actor* a_actor, RE::ActorValue a_actorValue, float a_val)
	{
		switch (a_actorValue) {
		case RE::ActorValue::kStamina:
			if (Utils::Actor::isBlocking(a_actor)) {
				a_val *= settings::fBlockingStaminaRegenMult;
			} else {
				RE::ATTACK_STATE_ENUM atkState = a_actor->AsActorState()->GetAttackState();
				if (atkState > RE::ATTACK_STATE_ENUM::kNone && atkState <= RE::ATTACK_STATE_ENUM::kBowFollowThrough) {
					return;  //don't regen stamina if attacking
				}
			}
			break;
		case RE::ActorValue::kHealth:
			stunHandler::GetSingleton()->modStun(a_actor, a_val);//stun regen on health regen
		}

		_RestoreActorValue(a_actor, a_actorValue, a_val);
	}
#pragma endregion

#pragma region MeleeHit
	void Hook_OnMeleeHit::processHit(RE::Actor* victim, RE::HitData& hitData)
	{
		auto aggressor = hitData.aggressor.get().get();
		if (!victim || !aggressor || victim->IsDead()) {
			_ProcessHit(victim, hitData);
			return;
		}

		if (settings::bPoise_PoiseToggle) {
			hitData.stagger = 0.0f;
		}

		if (Utils::Actor::isUnblockable(aggressor)) {
			if (hitData.flags.any(RE::HitData::Flag::kBlocked)) {
				hitData.flags.reset(RE::HitData::Flag::kBlocked);
				hitData.percentBlocked = 0.0f;
				victim->NotifyAnimationGraph("blockStop");
			}
		}

		auto fixedDmg = SpecialAttackHandler::GetSingleton()->getFixedDamage(aggressor);
		if (fixedDmg.has_value()) {
			hitData.physicalDamage = fixedDmg.value();
			hitData.totalDamage = fixedDmg.value();
		}

		float specialDmgMult = SpecialAttackHandler::GetSingleton()->getDamageMultiplier(aggressor);
		float hyperArmorDmgMult = SpecialAttackHandler::GetSingleton()->getHyperArmorDamageMultiplier(victim);
		float mult = specialDmgMult * hyperArmorDmgMult;
		hitData.totalDamage *= mult;
		hitData.physicalDamage *= mult;

		if (hitProcessor::claimHit(aggressor, victim)) {
			hitProcessor::GetSingleton()->processHit(aggressor, victim, hitData);
		}

		_ProcessHit(victim, hitData);
	}
#pragma endregion

#pragma region projectileHit
	/*Decide whether the collision is a actor-projectile collision. If it is, initialize a deflection attempt by the actor.
@param a_projectile: the projectile to be deflected.
@param a_AllCdPointCollector: pointer to the container storing all collision points.
@return whether a successful deflection is performed by the actor.*/
	
	inline bool shouldIgnoreHit(RE::Projectile* a_projectile, RE::hkpAllCdPointCollector* a_AllCdPointCollector)
	{
		if (a_AllCdPointCollector) {
			for (auto& hit : a_AllCdPointCollector->hits) {
				if (hit.rootCollidableA && hit.rootCollidableB) {
					auto refrA = RE::TESHavokUtilities::FindCollidableRef(*hit.rootCollidableA);
					auto refrB = RE::TESHavokUtilities::FindCollidableRef(*hit.rootCollidableB);
					if (refrA && refrA->formType == RE::FormType::ActorCharacter) {
						auto actorA = refrA->As<RE::Actor>();

						if (SpecialAttackHandler::GetSingleton()->isInvulnerable(actorA)) {
							return true;
						}

						bool projectileParryAllowedA = false;
						if (actorA->IsPlayerRef()) {
							projectileParryAllowedA = settings::bEnableProjectileParry && perks::canProjectileParry(actorA);
						}
						else {
							projectileParryAllowedA = settings::bEnableProjectileParry && settings::bEnableProjectileParry_NPC
								&& perks::canProjectileParry(actorA);
						}

						if (projectileParryAllowedA && Utils::Actor::isBashing(actorA)) {
							return parryHandler::GetSingleton()->processProjectileParry(actorA, a_projectile, const_cast<RE::hkpCollidable*>(hit.rootCollidableB));
						}
						else if (settings::bBlockProjectileToggle || settings::bTimedBlockProjectileToggle) {
							return blockHandler::GetSingleton()->processProjectileBlock(actorA, a_projectile, const_cast<RE::hkpCollidable*>(hit.rootCollidableB));
						}
					}
					if (refrB && refrB->formType == RE::FormType::ActorCharacter) {
						auto actorB = refrB->As<RE::Actor>();

						if (SpecialAttackHandler::GetSingleton()->isInvulnerable(actorB)) {
							return true;
						}

						bool projectileParryAllowedB = false;
						if (actorB->IsPlayerRef()) {
							projectileParryAllowedB = settings::bEnableProjectileParry && perks::canProjectileParry(actorB);
						}
						else {
							projectileParryAllowedB = settings::bEnableProjectileParry && settings::bEnableProjectileParry_NPC
								&& perks::canProjectileParry(actorB);
						}

						if (projectileParryAllowedB && Utils::Actor::isBashing(actorB)) {
							return parryHandler::GetSingleton()->processProjectileParry(actorB, a_projectile, const_cast<RE::hkpCollidable*>(hit.rootCollidableA));
						}
						else if (settings::bBlockProjectileToggle || settings::bTimedBlockProjectileToggle) {
							return blockHandler::GetSingleton()->processProjectileBlock(actorB, a_projectile, const_cast<RE::hkpCollidable*>(hit.rootCollidableA));
						}
					}
				}
			}
		}
		return false;
	}

	void Hook_OnProjectileCollision::OnArrowCollision(RE::Projectile* a_this, RE::hkpAllCdPointCollector* a_AllCdPointCollector)
	{
		if (shouldIgnoreHit(a_this, a_AllCdPointCollector)) {
			return;
		};
		_arrowCollission(a_this, a_AllCdPointCollector);
	};

	void Hook_OnProjectileCollision::OnMissileCollision(RE::Projectile* a_this, RE::hkpAllCdPointCollector* a_AllCdPointCollector)
	{
		if (shouldIgnoreHit(a_this, a_AllCdPointCollector)) {
			return;
		};
		_missileCollission(a_this, a_AllCdPointCollector);
	}
#pragma endregion

	void Hook_OnMeleeCollision::processHit(RE::Actor* a_aggressor, RE::Actor* a_victim, std::int64_t a_int1, bool a_bool, void* a_unkptr)
	{
		if (!a_aggressor || !a_victim) {
			_ProcessHit(a_aggressor, a_victim, a_int1, a_bool, a_unkptr);
			return;
		}

		if (SpecialAttackHandler::GetSingleton()->isInvulnerable(a_victim)) {
			return;
		}

		if (SpecialAttackHandler::GetSingleton()->processIncomingHit(a_victim, a_aggressor)) {
			return;
		}

		// 1. Determine if the parry system is allowed to run for the attacker and defender
		const bool parryEnabledForAttacker = settings::bParryToggle && (a_aggressor->IsPlayerRef() || settings::bParryToggle_NPC);
		const bool parryEnabledForDefender = settings::bParryToggle && (a_victim->IsPlayerRef() || settings::bParryToggle_NPC);

		// 2. Negate outgoing bash damage and stagger for permitted actors
		if (parryEnabledForAttacker && Utils::Actor::isBashing(a_aggressor)) {
			return; // Stop the bash from hitting the opponent
		}

		// 3. Process melee parry on defender (negates incoming damage/stagger on success)
		if (parryEnabledForDefender && Utils::Actor::isBashing(a_victim)) {
			if (parryHandler::GetSingleton()->processMeleeParry(a_aggressor, a_victim)) {
				return; // Hit successfully parried, bypass original hit processing
			}
		}

		// 4. Timed Block
		if (settings::bTimedBlockToggle && blockHandler::GetSingleton()->processMeleeTimedBlock(a_victim, a_aggressor)) {
			return;
		}

		// 5. Tackle
		if (settings::bTackleToggle && blockHandler::GetSingleton()->processMeleeTackle(a_victim, a_aggressor)) {
			return;
		}

		_ProcessHit(a_aggressor, a_victim, a_int1, a_bool, a_unkptr);
	}

#pragma region MagicPoise
	void Hook_MagicStagger::processStagger(RE::Actor* a_target, float a_staggerMult, RE::Actor* a_aggressor)
	{
		if (!a_target) {
			return;
		}
		// If Poise is disabled, run the vanilla engine function normally and exit.
		if (!settings::bPoise_PoiseToggle || !a_aggressor || a_target->IsDead()) {
			_ProcessStagger(a_target, a_staggerMult, a_aggressor);
			return;
		}
		// Evaluate custom poise health reduction and rules
		StaggerHandler::ProcessMagicStagger(a_target, a_staggerMult, a_aggressor);
	}
#pragma endregion

#pragma region EventSink
	bool Hook_NotifyAnimationGraph::NotifyAnimationGraph_Char(RE::IAnimationGraphManagerHolder* a_holder, const RE::BSFixedString& a_eventName)
	{
		return ProcessNotifyAnimationGraph(a_holder, a_eventName, [&]() {
			return _NotifyAnimationGraph_Char(a_holder, a_eventName);
			});
	}

	bool Hook_NotifyAnimationGraph::NotifyAnimationGraph_Player(RE::IAnimationGraphManagerHolder* a_holder, const RE::BSFixedString& a_eventName)
	{
		return ProcessNotifyAnimationGraph(a_holder, a_eventName, [&]() {
			return _NotifyAnimationGraph_Player(a_holder, a_eventName);
			});
	}

	bool Hook_NotifyAnimationGraph::ProcessNotifyAnimationGraph(RE::IAnimationGraphManagerHolder* a_holder, const RE::BSFixedString& a_eventName, std::function<bool()> a_originalCall)
	{
		if (!a_holder) {
			return a_originalCall();
		}

		auto actor = skyrim_cast<RE::Actor*>(a_holder);

		if (actor) {
			// 1. Block Commitment
			if (settings::bBlockCommitmentToggle) {
				if (BlockCommitment::GetSingleton()->ShouldSinkEvent(actor, a_eventName)) {
					return false; // Event sunk
				}
			}

			// 2. Exhaustion checks
			bool shouldProcess = true;
			if (actor->IsPlayerRef()) {
				if (settings::bExhaustionBlockAttacks && debuffHandler::GetSingleton()->processExhaustionAnimEvent(actor, a_eventName, shouldProcess)) {
					return shouldProcess;
				}
			}
			else {
				if (debuffHandler::GetSingleton()->ShouldSinkEvent(actor, a_eventName)) {
					return false;
				}
				if (debuffHandler::GetSingleton()->processExhaustionAnimEventNPC(actor, a_eventName, shouldProcess)) {
					return shouldProcess;
				}
			}
			if (debuffHandler::GetSingleton()->suppressExhaustionAnimEvent(actor, a_eventName, shouldProcess)) {
				return shouldProcess;
			}

			std::string_view eventStr = a_eventName.c_str();

			// 3. AI checks
			if (settings::bToggleAI) {
				if (isAttackStartEvent(eventStr)) {
					AILearning::GetSingleton()->safeRecordAttackStart(actor);
					AI::GetSingleton()->EvaluateAttackReaction(actor);
				}
			}

			if (!actor->IsPlayerRef()) {
				if (AI::GetSingleton()->ShouldSinkEvent(actor, a_eventName)) {
					return false;
				}
			}

			// 4. Engine-Level Event Processing
			if (a_eventName == "blockStart") {
				blockHandler::GetSingleton()->onBlockStart(actor);
				bool inDebuff = debuffHandler::GetSingleton()->isInDebuff(actor);
				if (!actor->IsPlayerRef() && inDebuff) {
					debuffHandler::GetSingleton()->onBlockStart(actor);
				}
			}
			else if (a_eventName == "blockStop") {
				blockHandler::GetSingleton()->onBlockStop(actor);
				if (!actor->IsPlayerRef()) {
					debuffHandler::GetSingleton()->onBlockStop(actor);
				}
			}
			else if (a_eventName == "bashStart") {
				if (!actor->IsPlayerRef() && settings::bParryToggle && settings::bParryToggle_NPC) {
					parryHandler::GetSingleton()->startTimingParry(actor);
				}
			}
			else if (a_eventName == "bashRelease") {
				if (actor->IsPlayerRef() && settings::bParryToggle) {
					parryHandler::GetSingleton()->startTimingParry(actor);
				}
			}
		}
		return a_originalCall();
	}
#pragma endregion

#pragma region Update
	void Hook_OnPlayerUpdate::Update(RE::PlayerCharacter* a_this, float a_delta)
	{

		ValhallaCombat::GetSingleton()->update();

		_Update(a_this, a_delta);
	}

	/*void Hook_OnCharacterUpdate::Update(RE::Character* a_this, float a_delta)
	{
		if (settings::bPoise_PoiseToggle) {
			PoiseRegenHandler::GetSingleton()->update(a_this, RE::GetSecondsSinceLastFrame());
		}

		_Update(a_this, a_delta);
	}*/
#pragma endregion

#pragma region PoisePerks
	void Hook_PerkEntryBypass::AggressorThunk(int32_t a_entry, RE::Actor* a_target, RE::Actor* a_aggressor, float& a_staggerMult)
	{
		(void)a_entry;
		(void)a_target;
		(void)a_aggressor;
		(void)a_staggerMult;
		return;
	}

	void Hook_PerkEntryBypass::TargetThunk(int32_t a_entry, RE::Actor* a_target, RE::Actor* a_aggressor, float& a_staggerMult)
	{
		(void)a_entry;
		(void)a_target;
		(void)a_aggressor;
		(void)a_staggerMult;
		return;
	}
#pragma endregion
}
