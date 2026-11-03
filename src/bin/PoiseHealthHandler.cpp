#include "include/PoiseHealthHandler.h"
#include "include/settings.h"
#include "include/Utils.h"
#include "include/data.h"

float PoiseHealthHandler::GetTotalPoiseHealth(RE::Actor* a_target)
{
	if (!a_target) {
		return 0.0f;
	}

	auto actorBase = a_target->GetActorBase();

	// Scale scaling
	float baseScale = a_target->GetScale();
	if (baseScale <= 0.0f) {
		baseScale = 1.0f;
	}

	// Formula: 1.0 + (Scale - 1.0) * ScaleMult Setting
	// Clamped to a minimum of 0.1f to prevent 0 or negative health multipliers
	float actorScale = (std::max)(0.1f, 1.0f + (baseScale - 1.0f) * settings::fPoise_ScaleHealthMult);

	// Weight scaling
	float actorWeight = 1.0f;

	// Ensure this only applies to Human NPCs
	if (data::npcKeyword && a_target->HasKeyword(data::npcKeyword)) {
		if (actorBase) {
			float weight = actorBase->GetWeight(); // 0.0 to 100.0 range
			float baseWeightMult = 1.0f;

			// Piece-wise linear scale to match specific threshold requirements: 
			// Weight 0 = 0.5x | Weight 50 = 1.0x | Weight 100 = 2.0x
			if (weight <= 50.0f) {
				baseWeightMult = 0.5f + (weight / 50.0f) * 0.5f;
			}
			else {
				baseWeightMult = 1.0f + ((weight - 50.0f) / 50.0f) * 1.0f;
			}

			// Formula: 1.0 + (WeightMult - 1.0) * WeightMult Setting
			// Clamped to a minimum of 0.1f to prevent 0 or negative health multipliers
			actorWeight = (std::max)(0.1f, 1.0f + (baseWeightMult - 1.0f) * settings::fPoise_WeightHealthMult);
		}
	}

	float basePoiseHealth = settings::fPoise_BasePoiseHealth; // Default fallback

	// 1. Behavior Graph / Project Name Override Check
	RE::BSTSmartPointer<RE::BSAnimationGraphManager> animGraphMgr;
	if (a_target->GetAnimationGraphManager(animGraphMgr) && animGraphMgr) {
		for (const auto& graph : animGraphMgr->graphs) {
			if (graph && !graph->projectName.empty()) {
				std::string projName = inlineUtils::to_lower_string(graph->projectName.c_str());

				auto itGraph = settings::poiseHealthGraphOverrideMap.find(projName);
				if (itGraph != settings::poiseHealthGraphOverrideMap.end()) {
					basePoiseHealth = itGraph->second;
					break; // Match found
				}
			}
		}
	}
	
	// 2. Race Override Check
	if (auto race = a_target->GetRace()) {
		auto itRace = settings::poiseHealthOverrideMap.find(race->GetFormID());
		if (itRace != settings::poiseHealthOverrideMap.end()) {
			basePoiseHealth = itRace->second; // Set to race default (e.g., 60.0)
		}
	}

	// 3. Base Actor Override Check
	if (actorBase) {
		auto itActor = settings::poiseHealthOverrideMap.find(actorBase->GetFormID());
		if (itActor != settings::poiseHealthOverrideMap.end()) {
			basePoiseHealth = itActor->second;
		}
	}

	// Calculate final health pools using the resolved base health value
	auto actorPoiseHealth = basePoiseHealth * actorWeight * actorScale;
	auto armorPoiseHealth = GetTotalArmorPoiseHealth(a_target);
	return actorPoiseHealth + armorPoiseHealth;
}

float PoiseHealthHandler::GetTotalArmorPoiseHealth(RE::Actor* a_target)
{
	float result = 0.0f;
	
	if (!a_target) {
		return result;
	}

	for (const auto& [slot, multiplier] : settings::armorSlotMultMap) {

		auto slotEnum = static_cast<RE::BGSBipedObjectForm::BipedObjectSlot>(slot);
		auto armor = a_target->GetWornArmor(slotEnum);

		auto armorHealth = settings::fPoise_BaseArmorPoiseHealth * multiplier;

		if (armor && (armor->IsLightArmor() || armor->IsHeavyArmor())) {
			result += armor->IsLightArmor() ? armorHealth : armorHealth * (1.0f + settings::fPoise_HeavyArmorPoiseBonus);
		}
		else {
			armor = a_target->GetSkin(slotEnum);
			if (armor && (armor->IsLightArmor() || armor->IsHeavyArmor())) {
				result += armor->IsLightArmor() ? armorHealth : armorHealth * (1.0f + settings::fPoise_HeavyArmorPoiseBonus);
			}
		}
	}
	return result;
}