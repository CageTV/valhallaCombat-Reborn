#include "include/parryHandler.h"
#include "include/Utils.h"
#include "include/data.h"
#include "include/offsets.h"       
#include "include/settings.h"
#include "include/reactionHandler.h"
#include "include/stunHandler.h"
#include "include/perks.h"
#include "include/attackHandler.h"
#include "include/lib/BlockSpark.h"
#include "ValhallaCombat.hpp"

void parryHandler::update()
{
	std::vector<RE::FormID> expiredFormIDs;

	{
		std::unique_lock<std::shared_mutex> lock(mtx_parryTimer);
		auto it = _parryTimer.begin();

		if (it == _parryTimer.end()) {
			ValhallaCombat::GetSingleton()->deactivateUpdate(ValhallaCombat::HANDLER::parryHandler);
			return;
		}

		float deltaTime = *RE::Offset::g_deltaTime;

		while (it != _parryTimer.end()) {
			if (it->second.timer > settings::fParryWindow_End) {
				if (settings::bSuccessfulParryNoCost && !it->second.successful) {
					expiredFormIDs.push_back(it->first);
				}
				it = _parryTimer.erase(it);
				continue;
			}

			it->second.timer += deltaTime;
			it++;
		}
	}

	for (auto formID : expiredFormIDs) {
		if (auto actor = RE::TESForm::LookupByID<RE::Actor>(formID)) {
			attackHandler::GetSingleton()->onLightBashHitFrame(actor);
		}
	}
}

void parryHandler::startTimingParry(RE::Actor* a_parrier)
{
	if (!a_parrier) {
		return;
	}
	std::unique_lock<std::shared_mutex> lock(mtx_parryTimer);
	RE::FormID formID = a_parrier->GetFormID();

	auto it = _parryTimer.find(formID);
	if (it != _parryTimer.end()) {
		it->second.timer = 0.0f;
		it->second.successful = false;
	} else {
		_parryTimer[formID] = { 0.0f, false };
	}
	ValhallaCombat::GetSingleton()->activateUpdate(ValhallaCombat::HANDLER::parryHandler);
}

void parryHandler::finishTimingParry(RE::Actor* a_parrier)
{
	if (!a_parrier) {
		return;
	}

	bool applyCost = false;
	{
		std::unique_lock<std::shared_mutex> lock(mtx_parryTimer);
		auto it = _parryTimer.find(a_parrier->GetFormID());
		if (it != _parryTimer.end()) {
			if (settings::bSuccessfulParryNoCost && !it->second.successful) {
				applyCost = true;
			}
			_parryTimer.erase(it);
		}
	}

	if (applyCost) {
		attackHandler::GetSingleton()->onLightBashHitFrame(a_parrier);
	}
}

bool parryHandler::inParryState(RE::Actor* a_parrier)
{
	if (!a_parrier) {
		return false;
	}
	std::shared_lock<std::shared_mutex> lock(mtx_parryTimer);
	auto it = _parryTimer.find(a_parrier->GetFormID());
	if (it != _parryTimer.end()) {
		return it->second.timer >= settings::fParryWindow_Start;
	}
	return false;
}

bool parryHandler::canParry(RE::Actor* a_parrier, RE::TESObjectREFR* a_obj) {
    return inParryState(a_parrier) && Utils::Actor::isInBlockAngle(a_parrier, a_obj);
}

bool parryHandler::processMeleeParry(RE::Actor* a_aggressor, RE::Actor* a_victim)
{
    if (!a_aggressor || !a_victim) {
        return false;
    }

    if (Utils::Actor::isPowerBashing(a_aggressor) || Utils::Actor::isBashing(a_aggressor)) {
        return false;
    }

    bool bIsUnblockable = false;
	if (settings::bUnblockableCompatibility && a_aggressor->GetGraphVariableBool("isUnblockableHit", bIsUnblockable) && bIsUnblockable) {
        return false;
    }

    if (canParry(a_victim, a_aggressor)) {

		// Mark parry as successful
		{
			std::unique_lock<std::shared_mutex> lock(mtx_parryTimer);
			auto it = _parryTimer.find(a_victim->GetFormID());
			if (it != _parryTimer.end()) {
				it->second.successful = true;
			}
		}

        // Stagger the aggressor (a_aggressor), caused by the parrier (a_victim)
        reactionHandler::applyStagger(a_victim, a_aggressor, reactionHandler::reactionType::kLargest);
        
        stunHandler::GetSingleton()->processStunDamage(stunHandler::STUNSOURCE::parry, nullptr, a_victim, a_aggressor, 0);
        
        playParryEffects(a_victim, a_aggressor);

        ValhallaUtils::SendModEvent("VC_MeleeParryEvent", a_victim);

        if (settings::bEnableParryExp && a_victim->IsPlayerRef()) {
            if (auto pc = RE::PlayerCharacter::GetSingleton()) {
                pc->AddSkillExperience(RE::ActorValue::kBlock, settings::fParryMeleeExp, nullptr, 0);
            }
        }
        if (settings::bApplySpellOnSuccess) {
            Utils::Actor::safeApplySpell(data::parrySpell, a_victim);
            if (settings::bAdvancedLogging) {
                logger::info("Parry Spell applied successfully");
            }
        }
        if (settings::bAdvancedLogging) {
            logger::info("Successful Parry");
        }
        return true;
    }
    return false;
}

bool parryHandler::processProjectileParry(RE::Actor* a_parrier, RE::Projectile* a_projectile, RE::hkpCollidable* a_projectile_collidable)
{
    if (!a_parrier || !a_projectile) {
        return false;
    }

    if (canParry(a_parrier, a_projectile)) {
        bool isSpell = a_projectile->GetProjectileRuntimeData().spell != nullptr;
        bool hasShield = Utils::Actor::isEquippedShield(a_parrier);
        bool parryAllowed = false;

        if (!hasShield) { // Weapon parry
            if (isSpell) {
                parryAllowed = settings::bWeaponParrySpell && perks::canWeaponParrySpell(a_parrier);
            }
            else {
                parryAllowed = settings::bWeaponParryArrow && perks::canWeaponParryArrow(a_parrier);
            }
        }
        else { // Shield parry
            if (isSpell) {
                parryAllowed = settings::bShieldParrySpell && perks::canShieldParrySpell(a_parrier);
            }
            else {
                parryAllowed = settings::bShieldParryArrow && perks::canShieldParryArrow(a_parrier);
            }
        }

        if (!parryAllowed) {
            return false;
        }

		// Mark parry as successful
		{
			std::unique_lock<std::shared_mutex> lock(mtx_parryTimer);
			auto it = _parryTimer.find(a_parrier->GetFormID());
			if (it != _parryTimer.end()) {
				it->second.successful = true;
			}
		}

        // Execute successful parry
        Utils::Projectile::deflect(a_parrier, a_projectile, a_projectile_collidable);

        playParryEffects(a_parrier, nullptr);

        ValhallaUtils::SendModEvent("VC_ProjectileParryEvent", a_parrier);

        if (settings::bEnableParryExp && a_parrier->IsPlayerRef()) {
            if (auto pc = RE::PlayerCharacter::GetSingleton()) {
                pc->AddSkillExperience(RE::ActorValue::kBlock, settings::fParryProjectileExp, nullptr, 0);
            }
        }
        if (settings::bApplySpellOnSuccess) {
            Utils::Actor::safeApplySpell(data::parryProjectileSpell, a_parrier);
            if (settings::bAdvancedLogging) {
                logger::info("Projectile Parry Spell applied successfully");
            }
        }
        if (settings::bAdvancedLogging) {
            logger::info("Successful Projectile Parry");
        }
        return true;
    }
    return false;
}

void parryHandler::onActorDeath(RE::Actor* a_actor) {
    if (!a_actor) {
        return;
    }
    std::unique_lock<std::shared_mutex> lock(mtx_parryTimer);
    _parryTimer.erase(a_actor->GetFormID());
}

void parryHandler::reset() {
    std::unique_lock<std::shared_mutex> lock(mtx_parryTimer);
    _parryTimer.clear();
}

void parryHandler::playParrySFX(RE::Actor* parrier, bool parriedWithWeapon) {
    if (parriedWithWeapon) {
        ValhallaUtils::playSound(parrier, data::soundParryWeapon_perfect, settings::fParrySFXVolume);
    }
    else {
        ValhallaUtils::playSound(parrier, data::soundParryShield_perfect, settings::fParrySFXVolume);
    }
}

void parryHandler::playParryVFX(RE::Actor* parrier, bool parriedWithWeapon) {
    auto blockFXNode = RE::Offset::PlaceAtMe(parrier, data::BlockFX, 1, false, false);
    std::string node = parriedWithWeapon ? "WEAPON" : "SHIELD";

    blockFXNode->MoveToNode(parrier, node);

    RE::Offset::PlaceAtMe(blockFXNode, data::BlockSpark, 1, false, false);
    RE::Offset::PlaceAtMe(blockFXNode, data::BlockSparkFlare, 1, false, false);
    RE::Offset::PlaceAtMe(blockFXNode, data::BlockSparkRing, 1, false, false);

    blockFXNode->SetDelete(true);
}

void parryHandler::playParryScreenShake() {
    RE::Offset::shakeCamera(1.5f, RE::PlayerCharacter::GetSingleton()->GetPosition(), 0.4f);
}

void parryHandler::playParrySlowTime() {

    float slowDuration = settings::fParrySlowTimeDuration;
    float slowPercentage = settings::fParrySlowTimePercentage;
    if (slowDuration > 0.0f) {
        inlineUtils::slowTime(slowDuration, slowPercentage);
    }
}

void parryHandler::playParryEffects(RE::Actor* parrier, RE::Actor* attacker) {
    bool shieldEquipped = RE::Offset::getEquippedShield(parrier);
    bool parriedWithWeapon = !shieldEquipped;

    if (settings::bParrySFX) {
        playParrySFX(parrier, parriedWithWeapon);
    }

    if (settings::bParryScreenShake &&
        ((parrier && parrier->IsPlayerRef()) || (attacker && attacker->IsPlayerRef()))) {
        playParryScreenShake();
    }

    if (settings::bParrySlowTime && parrier && parrier->IsPlayerRef()) {
        playParrySlowTime();
    }

    switch (settings::uParrySparkType) {
    case 0:
    default:
        break;
    case 1:
        playParryVFX(parrier, parriedWithWeapon);
        break;
    case 2:
        MaxsuBlockSpark::blockSpark::playPerfectBlockSpark(parrier);
        break;
    }
}
