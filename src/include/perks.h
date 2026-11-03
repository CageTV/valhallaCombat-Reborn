#pragma once

// Stores the settings loaded from Perks.ini
struct FeatureSetting
{
	bool enabled{ false };
	bool enabledNPC{ false };
	RE::BGSPerk* perk{ nullptr };
};

class perks
{
public:
	// Instances populated by data::loadINIPerks()
	static inline FeatureSetting timedBlocking;
	static inline FeatureSetting projectileBlocking;
	static inline FeatureSetting timedProjectileBlocking;
	static inline FeatureSetting projectileParrying;
	static inline FeatureSetting execution;
	static inline FeatureSetting tackle;
	static inline FeatureSetting perfectBlocking;
	static inline FeatureSetting perfectBlockingStamina;
	static inline FeatureSetting guardBash;
	static inline FeatureSetting weaponBlockSpell;
	static inline FeatureSetting weaponBlockArrow;
	static inline FeatureSetting shieldBlockSpell;
	static inline FeatureSetting shieldBlockArrow;
	static inline FeatureSetting weaponParrySpell;
	static inline FeatureSetting weaponParryArrow;
	static inline FeatureSetting shieldParrySpell;
	static inline FeatureSetting shieldParryArrow;

	// Feature Check Functions
	static bool canTimedBlock(RE::Actor* a_actor);
	static bool canProjectileBlock(RE::Actor* a_actor);
	static bool canTimedProjectileBlock(RE::Actor* a_actor);
	static bool canProjectileParry(RE::Actor* a_actor);
	static bool canExecute(RE::Actor* a_actor);
	static bool canTackle(RE::Actor* a_actor);
	static bool canPerfectBlock(RE::Actor* a_actor);
	static bool canPerfectBlockStamina(RE::Actor* a_actor);
	static bool canGuardBash(RE::Actor* a_actor);
	static bool canWeaponBlockSpell(RE::Actor* a_actor);
	static bool canWeaponBlockArrow(RE::Actor* a_actor);
	static bool canShieldBlockSpell(RE::Actor* a_actor);
	static bool canShieldBlockArrow(RE::Actor* a_actor);
	static bool canWeaponParrySpell(RE::Actor* a_actor);
	static bool canWeaponParryArrow(RE::Actor* a_actor);
	static bool canShieldParrySpell(RE::Actor* a_actor);
	static bool canShieldParryArrow(RE::Actor* a_actor);

private:
	// Central logic for evaluating if an actor meets the perk conditions
	static bool evaluatePerkRequirement(RE::Actor* a_actor, const FeatureSetting& a_setting);
};