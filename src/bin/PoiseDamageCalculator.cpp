#include "include/PoiseDamageCalculator.h"
#include "include/specialAttackHandler.h"
#include "include/settings.h"
#include "include/Utils.h"

float PoiseDamageCalculator::GetWeaponPoiseDamage(RE::Actor* a_aggressor, RE::Actor* a_target, const RE::HitData* a_hitData)
{
	float result = 0.0f;
	if (!a_aggressor || !a_target || !a_hitData) {
		logger::info("GetWeaponPoiseDamage: HitData, Aggressor or Target is null");
		return result;
	}

	RE::Projectile* sourceProjectile = nullptr;
	if (a_hitData->sourceRef) {
		if (auto ref = a_hitData->sourceRef.get().get()) {
			sourceProjectile = ref->As<RE::Projectile>();
		}
	}

	// Use your inline configuration structure for base weapon definitions
	float baseWeapDamage = GetBasePoiseDamage(a_aggressor, sourceProjectile != nullptr);
	float weapDamageMult = sourceProjectile ? GetWeaponDamageMult(sourceProjectile->GetProjectileRuntimeData().weaponSource) : GetWeaponDamageMult(a_hitData->weapon);
	float raceDamageMult = GetRaceDamageMult(a_aggressor);
	float actorDamageMult = GetActorDamageMult(a_aggressor);
	float graphDamageMult = GetGraphDamageMult(a_aggressor);
	float attackDataMult = GetAttackDataDamageMult(a_aggressor);
	float targetStateMult = GetTargetStateDamageMult(a_target);
	float ModTargetStagger = GetPerkModTargetStagger(a_aggressor, a_target);
	float ModIncomingStagger = GetPerkModIncomingStagger(a_aggressor, a_target);
	float StrengthMult = GetStrengthMult(a_aggressor, a_target);
	float specialAtkMult = GetSpecialAttackMult(a_aggressor, a_target);
	float difficultyMult = GetDifficultyDamageMult(a_aggressor, a_target);

	result = baseWeapDamage 
		* weapDamageMult 
		* raceDamageMult 
		* actorDamageMult 
		* attackDataMult 
		* graphDamageMult 
		* targetStateMult 
		* ModTargetStagger 
		* ModIncomingStagger 
		* StrengthMult 
		* specialAtkMult 
		* difficultyMult;
	
	if (settings::bAdvancedLogging) {
		logger::info("PoiseDmg [{:.2f}] - BaseDmg:{:.2f}, WeaponMult:{:.2f}, RaceMult:{:.2f}, ActorMult:{:.2f}, AtkDataMult:{:.2f}, GraphMult:{:.2f}, TargetStateMult:{:.2f}, PerkTargetMult:{:.2f}, PerkIncMult:{:.2f}, StrengthMult:{:.2f}, SpecialAtkMult:{:.2f}, DiffMult:{:.2f}",
			result, baseWeapDamage, weapDamageMult, raceDamageMult, actorDamageMult, attackDataMult, graphDamageMult, targetStateMult, ModTargetStagger, ModIncomingStagger, StrengthMult, specialAtkMult, difficultyMult);
	}
	return result;
}

float PoiseDamageCalculator::GetBasePoiseDamage(RE::Actor* a_aggressor, bool a_isRange)
{
	// Range/Projectiles skip the overrides and use default range base damage
	if (a_isRange) {
		return settings::fPoise_BaseRangePoiseDamage;
	}

	float baseDamage = settings::fPoise_BaseMeleePoiseDamage;

	if (!a_aggressor) {
		return baseDamage;
	}

	// 1. Behavior Graph / Project Name Override Check
	RE::BSTSmartPointer<RE::BSAnimationGraphManager> animGraphMgr;
	if (a_aggressor->GetAnimationGraphManager(animGraphMgr) && animGraphMgr) {
		for (const auto& graph : animGraphMgr->graphs) {
			if (graph && !graph->projectName.empty()) {
				std::string projName = inlineUtils::to_lower_string(graph->projectName.c_str());

				auto itGraph = settings::poiseBaseDamageGraphOverrideMap.find(projName);
				if (itGraph != settings::poiseBaseDamageGraphOverrideMap.end()) {
					baseDamage = itGraph->second;
					break; // Found matching behavior graph project
				}
			}
		}
	}

	// 2. Race Base Damage Override Check
	if (auto race = a_aggressor->GetRace()) {
		auto itRace = settings::poiseBaseDamageOverrideMap.find(race->GetFormID());
		if (itRace != settings::poiseBaseDamageOverrideMap.end()) {
			baseDamage = itRace->second;
		}
	}

	// 3. Base Actor Override Check (takes precedence over race, Melee only)
	if (auto actorBase = a_aggressor->GetActorBase()) {
		auto itActor = settings::poiseBaseDamageOverrideMap.find(actorBase->GetFormID());
		if (itActor != settings::poiseBaseDamageOverrideMap.end()) {
			baseDamage = itActor->second;
		}
	}

	return baseDamage;
}

float PoiseDamageCalculator::GetWeaponDamageMult(RE::TESObjectWEAP* a_weapon)
{
	if (!a_weapon) {
		return 1.0f;
	}

	// Check for a specific item FormID override first
	auto it = settings::poiseDamageMultOverrideMap.find(a_weapon->GetFormID());
	if (it != settings::poiseDamageMultOverrideMap.end()) {
		return it->second; // Return the custom override multiplier/value
	}

	switch (a_weapon->GetWeaponType()) {
	case RE::WEAPON_TYPE::kHandToHandMelee:
		return settings::fHandToHandMeleeMult;

	case RE::WEAPON_TYPE::kOneHandDagger:
		return settings::fOneHandDaggerMult;

	case RE::WEAPON_TYPE::kOneHandSword:
		return settings::fOneHandSwordMult;

	case RE::WEAPON_TYPE::kOneHandAxe:
		return settings::fOneHandAxeMult;

	case RE::WEAPON_TYPE::kOneHandMace:
		return settings::fOneHandMaceMult;

	case RE::WEAPON_TYPE::kTwoHandSword:
		return settings::fTwoHandSwordMult;

	case RE::WEAPON_TYPE::kTwoHandAxe:
		return settings::fTwoHandAxeMult;

	case RE::WEAPON_TYPE::kBow:
		return settings::fBowMult;

	case RE::WEAPON_TYPE::kCrossbow:
		return settings::fCrossbowMult;

	case RE::WEAPON_TYPE::kStaff:
		return settings::fStaffMult;

	default:
		return 1.0f;
	}
}

float PoiseDamageCalculator::GetRaceDamageMult(RE::Actor* a_aggressor)
{
	if (!a_aggressor) {
		return 1.0f;
	}

	if (auto race = a_aggressor->GetRace()) {
		auto itRace = settings::poiseDamageMultOverrideMap.find(race->GetFormID());
		if (itRace != settings::poiseDamageMultOverrideMap.end()) {
			return itRace->second;
		}
	}

	return 1.0f;
}

float PoiseDamageCalculator::GetActorDamageMult(RE::Actor* a_aggressor)
{
	if (!a_aggressor) {
		return 1.0f;
	}

	if (auto actorBase = a_aggressor->GetActorBase()) {
		auto itActor = settings::poiseDamageMultOverrideMap.find(actorBase->GetFormID());
		if (itActor != settings::poiseDamageMultOverrideMap.end()) {
			return itActor->second;
		}
	}

	return 1.0f;
}

float PoiseDamageCalculator::GetGraphDamageMult(RE::Actor* a_aggressor)
{
	if (!a_aggressor) {
		return 1.0f;
	}

	float graphMult = 1.0f;
	// BDI ensures this variable always exists, so a direct call is reliable
	a_aggressor->GetGraphVariableFloat("Val_PoiseMult", graphMult);

	return graphMult;
}

float PoiseDamageCalculator::GetAttackDataDamageMult(RE::Actor* a_aggressor)
{
	if (!a_aggressor) {
		return 1.0f;
	}

	// 1. Power Bash
	if (Utils::Actor::isPowerBashing(a_aggressor)) {
		return settings::fPoise_PowerBashMult;
	}

	// 2. Regular Bash
	if (Utils::Actor::isBashing(a_aggressor)) {
		return settings::fPoise_BashMult;
	}

	// 3. Power Attack
	if (Utils::Actor::isPowerAttacking(a_aggressor)) {
		return settings::fPoise_PowerAttackMult;
	}

	return 1.0f;
}

float PoiseDamageCalculator::GetTargetStateDamageMult(RE::Actor* a_target)
{
	if (!a_target) {
		return 1.0f;
	}

	// 1. Power Bash
	if (Utils::Actor::isPowerBashing(a_target)) {
		return settings::fPoise_TargetPowerBashMult;
	}

	// 2. Regular Bash
	if (Utils::Actor::isBashing(a_target)) {
		return settings::fPoise_TargetBashMult;
	}

	// 3. Power Attack
	if (Utils::Actor::isPowerAttacking(a_target)) {
		return settings::fPoise_TargetPowerAttackMult;
	}

	// 4. Light Attack
	if (Utils::Actor::isLightAttacking(a_target)) {
		return settings::fPoise_TargetLightAttackMult;
	}

	auto attackState = a_target->AsActorState()->GetAttackState();

	// 5. Bow or Crossbow Drawing / Holding Draw
	if (attackState == RE::ATTACK_STATE_ENUM::kBowDraw ||
		attackState == RE::ATTACK_STATE_ENUM::kBowDrawn) {
		return settings::fPoise_TargetBowDrawMult;
	}

	// 6. Casting / Charging Spells
	if (a_target->IsCasting(nullptr)) {
		return settings::fPoise_TargetCastingMult;
	}

	return 1.0f;
}

float PoiseDamageCalculator::GetStrengthMult(RE::Actor* a_aggressor, RE::Actor* a_target)
{
	if (!a_aggressor || !a_target) {
		return 1.0f;
	}

	// 1. Relative Scale Scaling
	float aggressorScale = a_aggressor->GetScale();
	float targetScale = a_target->GetScale();

	if (aggressorScale <= 0.0f) {
		aggressorScale = 1.0f;
	}
	if (targetScale <= 0.0f) {
		targetScale = 1.0f;
	}

	float scaleRatio = aggressorScale / targetScale;
	float scaleMult = (std::max)(0.1f, 1.0f + (scaleRatio - 1.0f) * settings::fPoise_ScaleDamageMult);

	// 2. Relative Weight Scaling (Human NPCs only)
	float weightMult = 1.0f;

	if (data::npcKeyword && a_aggressor->HasKeyword(data::npcKeyword) && a_target->HasKeyword(data::npcKeyword)) {
		auto aggressorBase = a_aggressor->GetActorBase();
		auto targetBase = a_target->GetActorBase();

		if (aggressorBase && targetBase) {
			float aggressorWeightFactor = 0.5f + (aggressorBase->GetWeight() / 100.0f) * 0.5f;
			float targetWeightFactor = 0.5f + (targetBase->GetWeight() / 100.0f) * 0.5f;

			float weightRatio = aggressorWeightFactor / targetWeightFactor;
			weightMult = (std::max)(0.1f, 1.0f + (weightRatio - 1.0f) * settings::fPoise_WeightDamageMult);
		}
	}

	return scaleMult * weightMult;
}

float PoiseDamageCalculator::GetSpecialAttackMult(RE::Actor* a_aggressor, RE::Actor* a_target) {
	if (!a_aggressor) {
		return 1.0f;
	}
	float specialAtkMult = SpecialAttackHandler::GetSingleton()->getStaggerMultiplier(a_aggressor);
	float hyperArmorMult = SpecialAttackHandler::GetSingleton()->getHyperArmorStaggerMultiplier(a_target);
	float mult = specialAtkMult * hyperArmorMult;
	return mult;
}

float PoiseDamageCalculator::GetDifficultyDamageMult(RE::Actor* a_aggressor, RE::Actor* a_target)
{
	if (settings::fPoise_DifficultyMultWeight == 0.0f) {
		return 1.0f;
	}

	float diffValue = 1.0f;
	inlineUtils::offsetRealDamage(diffValue, a_aggressor, a_target);

	// Normalize and scale based on weight (0.0 disables, 1.0 is full vanilla difficulty impact)
	float scaledMult = 1.0f + (diffValue - 1.0f) * settings::fPoise_DifficultyMultWeight;
	return (std::max)(0.01f, scaledMult);
}

float PoiseDamageCalculator::GetMagicPoiseDamage(RE::Actor* a_target, float a_staggerMult, RE::Actor* a_aggressor)
{
	if (!a_target) {
		return 0.0f;
	}

	float baseMagicDamage = settings::fPoise_BaseMagicPoiseDamage;
	float targetStateMult = GetTargetStateDamageMult(a_target);
	float hyperArmorMult = SpecialAttackHandler::GetSingleton()->getHyperArmorStaggerMultiplier(a_target);

	if (!a_aggressor) {
		return baseMagicDamage * targetStateMult * a_staggerMult;
	}

	float ModTargetStagger = GetPerkModTargetStagger(a_aggressor, a_target);
	float ModIncomingStagger = GetPerkModIncomingStagger(a_aggressor, a_target);
	float difficultyMult = GetDifficultyDamageMult(a_aggressor, a_target);

	return baseMagicDamage 
		* targetStateMult 
		* a_staggerMult 
		* ModTargetStagger 
		* ModIncomingStagger 
		* hyperArmorMult 
		* difficultyMult;
}

float PoiseDamageCalculator::GetPerkModTargetStagger(RE::Actor* a_aggressor, RE::Actor* a_target)
{
	using EntryPoint = RE::BGSEntryPointPerkEntry::EntryPoint;
	float result = 1.0f;

	if (!a_aggressor || !a_target) {
		return result;
	}

	// Linked explicitly to applyPerkEntryPoint tracker wrapper
	Utils::Actor::applyPerkEntryPoint(static_cast<std::int32_t>(EntryPoint::kModTargetStagger), a_aggressor, a_target, &result);
	return result;
}

float PoiseDamageCalculator::GetPerkModIncomingStagger(RE::Actor* a_aggressor, RE::Actor* a_target)
{
	using EntryPoint = RE::BGSEntryPointPerkEntry::EntryPoint;
	float result = 1.0f;

	if (!a_aggressor || !a_target) {
		return result;
	}

	// Linked explicitly to applyPerkEntryPoint tracker wrapper
	Utils::Actor::applyPerkEntryPoint(static_cast<std::int32_t>(EntryPoint::kModIncomingStagger), a_target, a_aggressor, &result);
	return result;
}
