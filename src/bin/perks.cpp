#include "include/perks.h"

bool perks::evaluatePerkRequirement(RE::Actor* a_actor, const FeatureSetting& a_setting)
{
	if (!a_actor) {
		return false;
	}

	// 1. Return true if perk locking for this feature is globally disabled
	if (!a_setting.enabled) {
		return true;
	}

	// 2. Return true if the actor is an NPC and perk locking is disabled for NPCs
	const bool isPlayer = a_actor->IsPlayerRef();
	if (!isPlayer && !a_setting.enabledNPC) {
		return true;
	}

	// 3. If perk locking is enabled, return true only if the actor possesses the loaded perk
	if (a_setting.perk && a_actor->HasPerk(a_setting.perk)) {
		return true;
	}
	return false;
}

bool perks::canTimedBlock(RE::Actor* a_actor) {
	return evaluatePerkRequirement(a_actor, timedBlocking);
}
bool perks::canProjectileBlock(RE::Actor* a_actor) {
	return evaluatePerkRequirement(a_actor, projectileBlocking);
}
bool perks::canTimedProjectileBlock(RE::Actor* a_actor) {
	return evaluatePerkRequirement(a_actor, timedProjectileBlocking);
}
bool perks::canProjectileParry(RE::Actor* a_actor) {
	return evaluatePerkRequirement(a_actor, projectileParrying);
}
bool perks::canExecute(RE::Actor* a_actor) {
	return evaluatePerkRequirement(a_actor, execution);
}
bool perks::canTackle(RE::Actor* a_actor) {
	return evaluatePerkRequirement(a_actor, tackle);
}
bool perks::canPerfectBlock(RE::Actor* a_actor) {
	return evaluatePerkRequirement(a_actor, perfectBlocking);
}
bool perks::canPerfectBlockStamina(RE::Actor* a_actor) {
	return evaluatePerkRequirement(a_actor, perfectBlockingStamina);
}
bool perks::canGuardBash(RE::Actor* a_actor) {
	return evaluatePerkRequirement(a_actor, guardBash);
}
bool perks::canWeaponBlockSpell(RE::Actor* a_actor) {
	return evaluatePerkRequirement(a_actor, weaponBlockSpell);
}
bool perks::canWeaponBlockArrow(RE::Actor* a_actor) {
	return evaluatePerkRequirement(a_actor, weaponBlockArrow);
}
bool perks::canShieldBlockSpell(RE::Actor* a_actor) {
	return evaluatePerkRequirement(a_actor, shieldBlockSpell);
}
bool perks::canShieldBlockArrow(RE::Actor* a_actor) {
	return evaluatePerkRequirement(a_actor, shieldBlockArrow);
}
bool perks::canWeaponParrySpell(RE::Actor* a_actor) {
	return evaluatePerkRequirement(a_actor, weaponParrySpell);
}
bool perks::canWeaponParryArrow(RE::Actor* a_actor) {
	return evaluatePerkRequirement(a_actor, weaponParryArrow);
}
bool perks::canShieldParrySpell(RE::Actor* a_actor) {
	return evaluatePerkRequirement(a_actor, shieldParrySpell);
}
bool perks::canShieldParryArrow(RE::Actor* a_actor) {
	return evaluatePerkRequirement(a_actor, shieldParryArrow);
}