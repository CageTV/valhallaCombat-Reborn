#include "include/data.h"
#include "include/settings.h"
#include "include/attackHandler.h"
#include "include/staminaHandler.h"
#include "include/debuffHandler.h"
#include "include/specialAttackHandler.h"
#include "include/Utils.h"

float staminaHandler::getLightAttackCost(RE::Actor* a_actor)
{
	if (!a_actor) {
		return settings::fMeleeCostLight_Base;
	}

	float weaponWeight = 0.0f;

	auto* object = a_actor->GetEquippedObject(false);

	if (auto* weapon = object ? object->As<RE::TESObjectWEAP>() : nullptr) {
		weaponWeight = weapon->weight;
	}

	return settings::fMeleeCostLight_Base + weaponWeight * settings::fMeleeCostLight_WeightMult;
}

float staminaHandler::getPowerAttackCost(RE::Actor* a_actor)
{
	if (!a_actor) {
		return settings::fMeleeCostPower_Base;
	}

	float weaponWeight = 0.0f;

	auto* object = a_actor->GetEquippedObject(false);

	if (auto* weapon = object ? object->As<RE::TESObjectWEAP>() : nullptr) {
		weaponWeight = weapon->weight;
	}

	return settings::fMeleeCostPower_Base + weaponWeight * settings::fMeleeCostPower_WeightMult;
}

float staminaHandler::getLightBashCost(RE::Actor* a_actor)
{
	if (!a_actor) {
		return settings::fMeleeCostLightBash_Base;
	}

	float weaponWeight = 0.0f;
	auto* object = a_actor->GetEquippedObject(false);

	if (auto* weapon = object ? object->As<RE::TESObjectWEAP>() : nullptr) {
		weaponWeight = weapon->weight;
	}

	return settings::fMeleeCostLightBash_Base + (weaponWeight * settings::fMeleeCostLightBash_WeightMult);
}

float staminaHandler::getPowerBashCost(RE::Actor* a_actor)
{
	if (!a_actor) {
		return settings::fMeleeCostPowerBash_Base;
	}

	float weaponWeight = 0.0f;
	auto* object = a_actor->GetEquippedObject(false);

	if (auto* weapon = object ? object->As<RE::TESObjectWEAP>() : nullptr) {
		weaponWeight = weapon->weight;
	}

	return settings::fMeleeCostPowerBash_Base + (weaponWeight * settings::fMeleeCostPowerBash_WeightMult);
}

float staminaHandler::getPerkStaminaCostMult(RE::Actor* a_actor)
{
	if (!a_actor) {
		return 1.0f;
	}

	float mult = 1.0f;

	RE::TESForm* weapon = nullptr;
	if (auto equippedForm = a_actor->GetEquippedObject(false)) {
		weapon = equippedForm;
	}

	RE::BGSEntryPoint::HandleEntryPoint<RE::TESForm*, float*>(
		RE::BGSEntryPoint::ENTRY_POINTS::kModPowerAttackStamina,
		a_actor,
		weapon,
		&mult
	);

	//logger::info("PowerAttackCostMult for {}: final multiplier is {}", a_actor->GetName(), mult);

	return mult;
}

float staminaHandler::getGraphStaminaMult(RE::Actor* a_actor)
{
	if (!a_actor) {
		return 1.0f;
	}

	float graphMult = 1.0f;
	a_actor->GetGraphVariableFloat("Val_StaminaMult", graphMult);
	
	return graphMult;
}

float staminaHandler::getSpecialAttackStaminaMult(RE::Actor* a_actor)
{
	if (!a_actor) {
		return 1.0f;
	}
	float specialAtkMult = SpecialAttackHandler::GetSingleton()->getStaminaMultiplier(a_actor);
	return specialAtkMult;
}

float staminaHandler::calculateFinalCost(RE::Actor* a_actor, float a_baseCost, bool a_usePerkMult, uint32_t a_hitIndex)
{
	float cost = a_baseCost;

	auto fixedStam = SpecialAttackHandler::GetSingleton()->getFixedStamina(a_actor);
	if (fixedStam.has_value()) {
		cost = fixedStam.value();
	}

	float perkMult = a_usePerkMult ? getPerkStaminaCostMult(a_actor) : 1.0f;
	float graphMult = getGraphStaminaMult(a_actor);
	float specialAtkMult = getSpecialAttackStaminaMult(a_actor);

	cost *= perkMult * graphMult * specialAtkMult;

	if (a_hitIndex > 1) {
		cost *= settings::fAdditionalHitFrameCostMult;
	}

	return cost;
}

void staminaHandler::checkStamina(RE::Actor* a_actor)
{
	if (!a_actor) {
		return;
	}

	if (!settings::bStaminaDebuffToggle) {
		return;
	}

	auto* avOwner = a_actor->AsActorValueOwner();
	if (!avOwner) {
		return;
	}

	if (avOwner->GetActorValue(RE::ActorValue::kStamina) <= 0) {
		debuffHandler::GetSingleton()->initStaminaDebuff(a_actor);
	}
}

void staminaHandler::staminaLightAttackReward(RE::Actor* a_actor, uint32_t hitIndex)
{
	if (!a_actor) {
		return;
	}

	if (!a_actor->IsInCombat() && !settings::bNonCombatStaminaCost) {
		return;
	}

	auto* avOwner = a_actor->AsActorValueOwner();
	if (!avOwner) {
		return;
	}

	float weaponWeight = 0.0f;
	auto* object = a_actor->GetEquippedObject(false);
	if (auto* weapon = object ? object->As<RE::TESObjectWEAP>() : nullptr) {
		weaponWeight = weapon->weight;
	}

	float reward = 0.0f;

	// Percent-based reward (existing system)
	reward += avOwner->GetPermanentActorValue(RE::ActorValue::kStamina) * settings::fMeleeRewardLightHit_Percent;

	// Flat reward
	reward += settings::fMeleeRewardLightHit_Point;

	// Weight-based reward
	reward += weaponWeight * settings::fMeleeRewardLightHit_WeightMult;

	// Scaling for later hitframes
	if (hitIndex > 1) {
		reward *= settings::fAdditionalHitFrameRewardMult;
	}

	Utils::Actor::restoreav(a_actor, RE::ActorValue::kStamina, reward);

	checkStamina(a_actor);
}

void staminaHandler::staminaLightAttackMiss(RE::Actor* a_actor)
{
	if (!a_actor || (!a_actor->IsInCombat() && !settings::bNonCombatStaminaCost)) {
		return;
	}

	float cost = calculateFinalCost(a_actor, getLightAttackCost(a_actor), settings::bLightAttackPerkMult, 1);

	Utils::Actor::damageav(a_actor, RE::ActorValue::kStamina, cost);
	checkStamina(a_actor);
}

void staminaHandler::staminaLightAttackHitFrameCost(RE::Actor* a_actor, uint32_t hitIndex)
{
	if (!a_actor || (!a_actor->IsInCombat() && !settings::bNonCombatStaminaCost)) {
		return;
	}

	float cost = calculateFinalCost(a_actor, getLightAttackCost(a_actor), settings::bLightAttackPerkMult, hitIndex);

	Utils::Actor::damageav(a_actor, RE::ActorValue::kStamina, cost);
	checkStamina(a_actor);
}

void staminaHandler::staminaPowerAttackHitFrameCost(RE::Actor* a_actor, uint32_t hitIndex)
{
	if (!a_actor || (!a_actor->IsInCombat() && !settings::bNonCombatStaminaCost)) {
		return;
	}

	float cost = calculateFinalCost(a_actor, getPowerAttackCost(a_actor), settings::bPowerAttackPerkMult, hitIndex);

	Utils::Actor::damageav(a_actor, RE::ActorValue::kStamina, cost);
	checkStamina(a_actor);
}

void staminaHandler::staminaLightBashHitFrameCost(RE::Actor* a_actor, uint32_t hitIndex)
{
	if (!a_actor || (!a_actor->IsInCombat() && !settings::bNonCombatStaminaCost)) {
		return;
	}

	float cost = calculateFinalCost(a_actor, getLightBashCost(a_actor), settings::bLightBashPerkMult, hitIndex);

	Utils::Actor::damageav(a_actor, RE::ActorValue::kStamina, cost);
	checkStamina(a_actor);
}

void staminaHandler::staminaPowerBashHitFrameCost(RE::Actor* a_actor, uint32_t hitIndex)
{
	if (!a_actor || (!a_actor->IsInCombat() && !settings::bNonCombatStaminaCost)) {
		return;
	}

	float cost = calculateFinalCost(a_actor, getPowerBashCost(a_actor), settings::bPowerBashPerkMult, hitIndex);

	Utils::Actor::damageav(a_actor, RE::ActorValue::kStamina, cost);
	checkStamina(a_actor);
}