#pragma once

class PoiseDamageCalculator
{
public:
	static float GetWeaponPoiseDamage(RE::Actor* a_aggressor, RE::Actor* a_target, const RE::HitData* a_hitData);
	static float GetMagicPoiseDamage(RE::Actor* a_target, float a_staggerMult, RE::Actor* a_aggressor);

private:
	static float GetBasePoiseDamage(RE::Actor* a_aggressor, bool a_isRange);
	static float GetWeaponDamageMult(RE::TESObjectWEAP* a_weapon);
	static float GetRaceDamageMult(RE::Actor* a_aggressor);
	static float GetActorDamageMult(RE::Actor* a_aggressor);
	static float GetGraphDamageMult(RE::Actor* a_aggressor);
	static float GetAttackDataDamageMult(RE::Actor* a_aggressor);
	static float GetTargetStateDamageMult(RE::Actor* a_target);
	static float GetStrengthMult(RE::Actor* a_aggressor, RE::Actor* a_target);
	static float GetSpecialAttackMult(RE::Actor* a_aggressor, RE::Actor* a_target);
	static float GetDifficultyDamageMult(RE::Actor* a_aggressor, RE::Actor* a_target);

	static float GetPerkModTargetStagger(RE::Actor* a_aggressor, RE::Actor* a_target);
	static float GetPerkModIncomingStagger(RE::Actor* a_aggressor, RE::Actor* a_target);

	PoiseDamageCalculator() = delete;
	~PoiseDamageCalculator() = delete;
};
