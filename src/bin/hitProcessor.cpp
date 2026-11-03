#include "include/hitProcessor.h"
#include "include/attackHandler.h"
#include "include/blockHandler.h"
#include "include/debuffHandler.h"
#include "include/stunHandler.h"
#include "include/executionHandler.h"
#include "include/settings.h"
#include "include/reactionHandler.h"
#include "include/StaggerHandler.h"
#include "include/Utils.h"
#include "include/perks.h"
#include "include/data.h"
#include "include/AILearning.h"
#include "include/specialAttackHandler.h"

void hitProcessor::processHit(RE::Actor* a_aggressor, RE::Actor* a_victim, RE::HitData& a_hitData) {
	//offset damage from hitdata, based on player difficulty setting.

	if (!a_aggressor || !a_victim) {
		return;
	}

	bool wasParrySuccess = SpecialAttackHandler::GetSingleton()->isParrySuccess(a_aggressor);
	float realDamage = a_hitData.totalDamage;
	inlineUtils::offsetRealDamage(realDamage, a_aggressor, a_victim);
	auto hitFlag = a_hitData.flags;
	
	if (hitFlag.any(HITFLAG::kBlocked)) {

		if (Utils::Actor::isPowerBashing(a_aggressor) && settings::bEnableGuardBash && perks::canGuardBash(a_aggressor)) {

			reactionHandler::applyStagger(a_aggressor, a_victim, reactionHandler::reactionType::kLargest);
			stunHandler::GetSingleton()->processStunDamage(stunHandler::STUNSOURCE::powerBash, nullptr, a_aggressor, a_victim, 0);
			if (settings::bAdvancedLogging) {
				logger::info("hitProcessor::Power Bash - Block Guard Break");
			}
		}
		else {

			blockHandler::GetSingleton()->processPhysicalBlock(a_victim, a_aggressor, hitFlag, a_hitData);
		}
		if (settings::bBlockedHitRegenStamina && settings::uLightAttackStaminaMode != settings::Disabled) {
			if (Utils::Actor::isLightAttacking(a_aggressor)) {
				attackHandler::GetSingleton()->OnLightHit(a_aggressor);
				if (settings::bAdvancedLogging) {
					logger::info("hitProcessor::Light Attack - Block Stamina Restore");
				}
			}
		}
		if (settings::bPoise_PoiseToggle) {
			bool bPoiseBreaksBlock = false;
			if (Utils::Actor::isPowerBashing(a_aggressor) && !settings::bEnableGuardBash) {
				StaggerHandler::ProcessWeaponStagger(a_aggressor, a_victim, &a_hitData);
				stunHandler::GetSingleton()->processStunDamage(stunHandler::STUNSOURCE::powerBash, nullptr, a_aggressor, a_victim, 0);
				if (settings::bAdvancedLogging) {
					logger::info("hitProcessor::ProcessWeaponStagger - Triggered through Block (Power Bash)");
				}
			}
			else if (Utils::Actor::isBashing(a_aggressor)) {
				StaggerHandler::ProcessWeaponStagger(a_aggressor, a_victim, &a_hitData);
				stunHandler::GetSingleton()->processStunDamage(stunHandler::STUNSOURCE::bash, nullptr, a_aggressor, a_victim, 0);
				if (settings::bAdvancedLogging) {
					logger::info("hitProcessor::ProcessWeaponStagger - Triggered through Block (Bash)");
				}
			}
			else if (a_aggressor && a_aggressor->GetGraphVariableBool("Val_PoiseBreaksBlock", bPoiseBreaksBlock) && bPoiseBreaksBlock) {
				StaggerHandler::ProcessWeaponStagger(a_aggressor, a_victim, &a_hitData);
				if (settings::bAdvancedLogging) {
					logger::info("hitProcessor::ProcessWeaponStagger - Triggered through Block (Val_PoiseBreaksBlock)");
				}
			}
			else if (debuffHandler::GetSingleton()->isInDebuff(a_victim)) {
				StaggerHandler::ProcessWeaponStagger(a_aggressor, a_victim, &a_hitData);
				if (settings::bAdvancedLogging) {
					logger::info("hitProcessor::ProcessWeaponStagger - Triggered through Block (Exhausted)");
				}
			}
		}
		else if (settings::bGuardBreak && !a_victim->IsPlayerRef()) {
			if (debuffHandler::GetSingleton()->isInDebuff(a_victim)) {
				reactionHandler::applyStagger(a_aggressor, a_victim, reactionHandler::kLargest);
			}
		}
		if (settings::bToggleAI && !Utils::Actor::isBashing(a_aggressor) && !Utils::Actor::isPowerBashing(a_aggressor)) {
			bool isPower = Utils::Actor::isPowerAttacking(a_aggressor);
			AILearning::GetSingleton()->safeRecordImpact(a_aggressor, a_victim, isPower);
		}
		SpecialAttackHandler::GetSingleton()->changeStateOnHit(a_aggressor);
		return;
	}// from this point on the hit is not blocked
	else {
		auto forcedStagger = SpecialAttackHandler::GetSingleton()->getForcedStagger(a_aggressor);
		if (forcedStagger.has_value()) {
			reactionHandler::applyStagger(a_aggressor, a_victim, reactionHandler::reactionType(forcedStagger.value()));
			if (settings::bAdvancedLogging) {
				logger::info("Forced Stagger Applied - Type: {}", forcedStagger.value());
			}
		}
		else if (settings::bPoise_PoiseToggle) {
			StaggerHandler::ProcessWeaponStagger(a_aggressor, a_victim, &a_hitData);
		}
		else if (wasParrySuccess) {
			reactionHandler::applyStagger(a_aggressor, a_victim, reactionHandler::reactionType::kLargest);
		}
		if (SpecialAttackHandler::GetSingleton()->hasActionOnHit(a_aggressor)) {
			SpecialAttackHandler::GetSingleton()->processActionOnHit(a_aggressor);
		}
		// Parry Followup Spell
		if (wasParrySuccess && SpecialAttackHandler::GetSingleton()->shouldApplyParryFollowup(a_aggressor)) {
			Utils::Actor::safeApplySpell(data::parryFollowupSpell, a_aggressor);
		}
		// Regular Followup Spell
		else if (SpecialAttackHandler::GetSingleton()->shouldApplyFollowup(a_aggressor)) {
			Utils::Actor::safeApplySpell(data::followupSpell, a_aggressor);
		}
		SpecialAttackHandler::GetSingleton()->changeStateOnHit(a_aggressor);
	}
	
	if (Utils::Actor::isPowerBashing(a_aggressor)) {
			stunHandler::GetSingleton()->processStunDamage(stunHandler::STUNSOURCE::powerBash, nullptr, a_aggressor, a_victim, 0);
			if (settings::bAdvancedLogging) {
				logger::info("hitProcessor::Power Bash - Hit");
			}
			return;
	}
	if (Utils::Actor::isBashing(a_aggressor)) {
			stunHandler::GetSingleton()->processStunDamage(stunHandler::STUNSOURCE::bash, nullptr, a_aggressor, a_victim, 0);
			if (settings::bAdvancedLogging) {
				logger::info("hitProcessor::Bash - Hit");
			}
			return;
	}
	
	if (settings::uLightAttackStaminaMode != settings::Disabled) {
		if (Utils::Actor::isLightAttacking(a_aggressor)) {
			attackHandler::GetSingleton()->OnLightHit(a_aggressor);
			if (settings::bAdvancedLogging) {
				logger::info("hitProcessor::Light Attack - Stamina Restore");
			}
		}
	}
	bool isMeleeHit = a_hitData.weapon && a_hitData.weapon->IsMelee();
	if (stunHandler::GetSingleton()->getIsStunBroken(a_victim) && isMeleeHit) {
		if (a_aggressor->IsPlayerRef()) {
			if (settings::bAutoExecution) {//player only auto-execute if auto execution is on
				executionHandler::GetSingleton()->attemptExecute(a_aggressor, a_victim);
				if (settings::bAdvancedLogging) {
					logger::info("hitProcessor::Player attempts Execution");
				}
			}
		}
		else { //NPC auto-execute always.
			executionHandler::GetSingleton()->attemptExecute(a_aggressor, a_victim);
			if (settings::bAdvancedLogging) {
				logger::info("hitProcessor::NPC attempts Execution");
			}
		}
	}

	if (Utils::Actor::isPowerAttacking(a_aggressor)) {
		bool b;
		//TODO:redo stun damage for elden counter
		if (a_aggressor->GetGraphVariableBool(data::GraphBool_IsGuardCountering, b) && b) {
			stunHandler::GetSingleton()->processStunDamage(stunHandler::STUNSOURCE::powerAttack, a_hitData.weapon, a_aggressor, a_victim, realDamage);
		} else {
			stunHandler::GetSingleton()->processStunDamage(stunHandler::STUNSOURCE::powerAttack, a_hitData.weapon, a_aggressor, a_victim, realDamage);
			if (settings::bAdvancedLogging) {
				logger::info("hitProcessor::Power Attack - Hit");
			}
		}
	}
	else {
		stunHandler::GetSingleton()->processStunDamage(stunHandler::STUNSOURCE::lightAttack, a_hitData.weapon, a_aggressor, a_victim, realDamage);
		if (settings::bAdvancedLogging) {
			logger::info("hitProcessor::Light Attack - Hit");
		}
	}
	if (settings::bToggleAI && !Utils::Actor::isBashing(a_aggressor) && !Utils::Actor::isPowerBashing(a_aggressor)) {
		bool isPower = Utils::Actor::isPowerAttacking(a_aggressor);
		AILearning::GetSingleton()->safeRecordImpact(a_aggressor, a_victim, isPower);
	}
}
