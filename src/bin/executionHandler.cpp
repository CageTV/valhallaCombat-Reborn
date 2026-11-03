#include "include/executionHandler.h"
#include "include/data.h"
#include "include/settings.h"
#include "include/stunHandler.h"
#include "include/Utils.h"

void executionHandler::tryPcExecution()
{
	auto pc = RE::PlayerCharacter::GetSingleton();
	if (!pc) {
		return;
	}

	RE::Actor* optimalVictim = nullptr;
	float curr_min_dist = HUGE_VALF;

	for (auto formID : stunHandler::GetSingleton()->getStunBrokenActors()) {
		auto actor = RE::TESForm::LookupByID<RE::Actor>(formID);

		if (!actor || !actor->Is3DLoaded() || actor->IsDead() || actor->IsInKillMove()) {
			continue;
		}

		// Prevent the player from executing targets unless they meet the condition
		if (!stunHandler::GetSingleton()->isEligibleForExecution(actor)) {
			continue;
		}

		auto& data = actor->GetActorRuntimeData();
		auto* process = data.currentProcess;

		if (!process || !process->InHighProcess()) {
			continue;
		}

		float dist = pc->GetPosition().GetDistance(actor->GetPosition());

		if (dist >= 250.0f) {
			continue;
		}

		if (dist < curr_min_dist) {
			curr_min_dist = dist;
			optimalVictim = actor;
		}
	}

	if (optimalVictim) {
		attemptExecute(pc, optimalVictim);
	}
}

bool executionHandler::attemptExecute(RE::Actor* a_executor, RE::Actor* a_victim)
{
	if (!a_executor || !a_victim) {
		return false;
	}
	if (!settings::bStunToggle ||
		!Utils::Actor::isHumanoid(a_executor) ||
		a_executor->IsDead() || a_victim->IsDead() ||
		!a_executor->Is3DLoaded() || !a_victim->Is3DLoaded() ||
		a_executor->IsInKillMove() || a_victim->IsInKillMove() ||
		a_executor->IsOnMount() || a_victim->IsOnMount() ||
		a_victim->IsEssential() || !Utils::Actor::canExecute(a_executor) ||
		!stunHandler::GetSingleton()->isEligibleForExecution(a_victim)) {
		return false;
	}

	// SAFE paralysis check
	auto magicTarget = a_victim->AsMagicTarget();
	if (magicTarget && magicTarget->HasEffectWithArchetype(RE::MagicTarget::Archetype::kParalysis)) {
		return false;
	}

	data::raceCatagory victimRaceType{};
	;
	bool foundCategory = false;

	// Tracking variables for logging
	std::string matchSource = "";
	std::string matchDetail = "";

	// Check Race FormID FIRST (Overrides Graph Project Name)
	auto victimRace = a_victim->GetRace();
	if (victimRace) {
		auto itRace = data::raceMapping.find(victimRace);
		if (itRace != data::raceMapping.end()) {
			victimRaceType = itRace->second;
			foundCategory = true;

			matchSource = "Race FormID";

			const char* rawName = victimRace->GetName();
			std::string raceName = (rawName && strlen(rawName) > 0) ? rawName : "Unnamed Race";

			matchDetail = fmt::format("{} [0x{:08X}]", raceName, victimRace->GetFormID());
		}
	}

	// Behavior Graph Project Name if no specific Race FormID matched
	if (!foundCategory) {
		RE::BSTSmartPointer<RE::BSAnimationGraphManager> animGraphMgr;
		if (a_victim->GetAnimationGraphManager(animGraphMgr) && animGraphMgr) {
			for (const auto& graph : animGraphMgr->graphs) {
				if (graph && !graph->projectName.empty()) {
					std::string projName = inlineUtils::to_lower_string(graph->projectName.c_str());

					auto itGraph = data::graphRaceMapping.find(projName);
					if (itGraph != data::graphRaceMapping.end()) {
						victimRaceType = itGraph->second;
						foundCategory = true;

						matchSource = "Behavior Graph";
						matchDetail = projName;
						break;
					}
				}
			}
		}
	}

	const char* rawVictimName = a_victim->GetName();
	std::string victimName = (rawVictimName && strlen(rawVictimName) > 0) ? rawVictimName : "Unnamed Actor";

	// Log resolution failure or success
	if (!foundCategory) {
		if (settings::bAdvancedLogging) {
			logger::info("Execution failed for victim '{}' [0x{:08X}]: No matching Race FormID or Behavior Graph found.",
				victimName, a_victim->GetFormID());
		}
		return false;
	}
	if (settings::bAdvancedLogging) {
		logger::info("Execution attempted on target '{}' [0x{:08X}] -> Resolved via {}: {}",
			victimName, a_victim->GetFormID(), matchSource, matchDetail);
	}

	RE::WEAPON_TYPE weaponType;
	auto weapon = Utils::Actor::getWieldingWeapon(a_executor);
	if (!weapon) {
		weaponType = RE::WEAPON_TYPE::kHandToHandMelee;
	} else {
		weaponType = weapon->GetWeaponType();
	}

	if (weaponType == RE::WEAPON_TYPE::kBow || weaponType == RE::WEAPON_TYPE::kCrossbow) {
		return false;
	}

#define RACE data::raceCatagory
	switch (victimRaceType) {
	case RACE::Humanoid:
		executeHumanoid(a_executor, a_victim, weaponType);
		break;
	case RACE::Undead:
		executeDraugr(a_executor, a_victim, weaponType);
		break;
	case RACE::Falmer:
		executeFalmer(a_executor, a_victim, weaponType);
		break;
	case RACE::Spider:
		executeSpider(a_executor, a_victim, weaponType);
		break;
	case RACE::Gargoyle:
		executeGargoyle(a_executor, a_victim, weaponType);
		break;
	case RACE::Giant:
		executeGiant(a_executor, a_victim, weaponType);
		break;
	case RACE::Bear:
		executeBear(a_executor, a_victim, weaponType);
		break;
	case RACE::SabreCat:
		executeSabreCat(a_executor, a_victim, weaponType);
		break;
	case RACE::Wolf:
		executeWolf(a_executor, a_victim, weaponType);
		break;
	case RACE::Troll:
		executeTroll(a_executor, a_victim, weaponType);
		break;
	case RACE::Boar:
		executeBoar(a_executor, a_victim, weaponType);
		break;
	case RACE::Hagraven:
		executeHagraven(a_executor, a_victim, weaponType);
		break;
	case RACE::Spriggan:
		executeSpriggan(a_executor, a_victim, weaponType);
		break;
	case RACE::Riekling:
		executeRiekling(a_executor, a_victim, weaponType);
		break;
	case RACE::AshHopper:
		executeAshHopper(a_executor, a_victim, weaponType);
		break;
	case RACE::SteamCenturion:
		executeSteamCenturion(a_executor, a_victim, weaponType);
		break;
	case RACE::DwarvenBallista:
		executeDwarvenBallista(a_executor, a_victim, weaponType);
		break;
	case RACE::ChaurusFlyer:
		executeChaurusFlyer(a_executor, a_victim, weaponType);
		break;
	case RACE::Lurker:
		executeLurker(a_executor, a_victim, weaponType);
		break;
	case RACE::Dragon:
		executeDragon(a_executor, a_victim, weaponType);
		break;
	default:
		return false;
	}
	return true;
}

inline bool playExecutionIdle(RE::Actor* a_executor, RE::Actor* a_victim, RE::TESIdleForm* a_executionIdle)
{
	if (!a_executor || !a_victim || !a_executionIdle) {
		return false;
	}

	if (!a_executor->Is3DLoaded() || !a_victim->Is3DLoaded()) {
		return false;
	}

	auto& executor_data = a_executor->GetActorRuntimeData();
	auto* executor_process = executor_data.currentProcess;

	if (!executor_process) {
		return false;
	}

	bool result = RE::Offset::playPairedIdle(
		executor_process,
		a_executor,
		RE::DEFAULT_OBJECT::kActionIdle,
		a_executionIdle,
		true,
		false,
		a_victim);

	return result;
}

void executionHandler::queueExecutionIdle(RE::Actor* a_executor, RE::Actor* a_victim, const std::vector<RE::TESIdleForm*>& a_executionIdleV)
{
	if (!a_executor || !a_victim || a_executionIdleV.empty()) {
		return;
	}

	// Pick random idle
	auto idle = *inlineUtils::select_randomly(a_executionIdleV.begin(), a_executionIdleV.end());
	queueExecutionIdle(a_executor, a_victim, idle);
}

void executionHandler::queueExecutionIdle(RE::Actor* a_executor, RE::Actor* a_victim, RE::TESIdleForm* a_executionIdle)
{
	if (!a_executor || !a_victim || !a_executionIdle)
		return;

	auto executorHandle = a_executor->GetHandle();
	auto victimHandle = a_victim->GetHandle();
	auto idleFormID = a_executionIdle->GetFormID();

	if (auto task = SKSE::GetTaskInterface()) {
		task->AddTask([executorHandle, victimHandle, idleFormID]() {
			auto executor = executorHandle.get().get();
			auto victim = victimHandle.get().get();
			auto* idle = RE::TESForm::LookupByID<RE::TESIdleForm>(idleFormID);

			if (!executor || !victim || !idle) {
				return;
			}

			bool success = playExecutionIdle(executor, victim, idle);
			if (settings::bAdvancedLogging) {
				if (success) {
					logger::warn("Succeeded to play execution idle [{:X}]", idleFormID);
				} 
				else {
					logger::warn("Failed to play execution idle [{:X}]", idleFormID);
				}
			}
		});
	}
}

void executionHandler::executeHumanoid(RE::Actor* a_executor, RE::Actor* a_victim, RE::WEAPON_TYPE a_weaponType) {
	//logger::info("executing humanoid!");
	if (Utils::Actor::isDualWielding(a_executor)) {
		//logger::info("dual wielding!");
		queueExecutionIdle(a_executor, a_victim, data::KM_Humanoid_DW);
	}
	else if (Utils::Actor::isBackFacing(a_victim, a_executor)) {
		//logger::info("backstab!");
		switch (a_weaponType) {
		case RE::WEAPON_TYPE::kTwoHandAxe: queueExecutionIdle(a_executor, a_victim, data::KM_Humanoid_2hw_Back); break;
		case RE::WEAPON_TYPE::kTwoHandSword: queueExecutionIdle(a_executor, a_victim, data::KM_Humanoid_2hm_Back); break;
		case RE::WEAPON_TYPE::kHandToHandMelee: queueExecutionIdle(a_executor, a_victim, data::KM_Humanoid_H2H_Back); break;
		default: queueExecutionIdle(a_executor, a_victim, data::KM_Humanoid_1hm_Back); break;
		}
	}
	else {
		switch (a_weaponType) {
		case RE::WEAPON_TYPE::kOneHandSword: queueExecutionIdle(a_executor, a_victim, data::KM_Humanoid_Sword); break;
		case RE::WEAPON_TYPE::kOneHandDagger: queueExecutionIdle(a_executor, a_victim, data::KM_Humanoid_Dagger); break;
		case RE::WEAPON_TYPE::kOneHandAxe:  queueExecutionIdle(a_executor, a_victim, data::KM_Humanoid_Axe); break;
		case RE::WEAPON_TYPE::kOneHandMace:  queueExecutionIdle(a_executor, a_victim, data::KM_Humanoid_Mace); break;
		case RE::WEAPON_TYPE::kTwoHandAxe:  queueExecutionIdle(a_executor, a_victim, data::KM_Humanoid_2hw); break;
		case RE::WEAPON_TYPE::kTwoHandSword:  queueExecutionIdle(a_executor, a_victim, data::KM_Humanoid_GreatSword); break;
		case RE::WEAPON_TYPE::kHandToHandMelee:  queueExecutionIdle(a_executor, a_victim, data::KM_Humanoid_H2H); break;
		}
	}
};
void executionHandler::executeDraugr(RE::Actor* a_executor, RE::Actor* a_victim, RE::WEAPON_TYPE weaponType) {
	switch (weaponType) {
	case RE::WEAPON_TYPE::kTwoHandAxe: queueExecutionIdle(a_executor, a_victim, data::KM_Undead_2hw); break;
	case RE::WEAPON_TYPE::kTwoHandSword: queueExecutionIdle(a_executor, a_victim, data::KM_Undead_2hm); break;
	case RE::WEAPON_TYPE::kHandToHandMelee: break;
	default: queueExecutionIdle(a_executor, a_victim, data::KM_Undead_1hm); break;
	}
};
void executionHandler::executeFalmer(RE::Actor* a_executor, RE::Actor* a_victim, RE::WEAPON_TYPE weaponType) {
	switch (weaponType) {
	case RE::WEAPON_TYPE::kTwoHandAxe: queueExecutionIdle(a_executor, a_victim, data::KM_Falmer_2hw); break;
	case RE::WEAPON_TYPE::kTwoHandSword: queueExecutionIdle(a_executor, a_victim, data::KM_Falmer_2hm); break;
	case RE::WEAPON_TYPE::kHandToHandMelee: break;
	default: queueExecutionIdle(a_executor, a_victim, data::KM_Falmer_1hm);
	}
};
void executionHandler::executeSpider(RE::Actor* a_executor, RE::Actor* a_victim, RE::WEAPON_TYPE weaponType) {
	switch (weaponType) {
	case RE::WEAPON_TYPE::kTwoHandAxe: queueExecutionIdle(a_executor, a_victim, data::KM_Spider_2hw); break;
	case RE::WEAPON_TYPE::kTwoHandSword: queueExecutionIdle(a_executor, a_victim, data::KM_Spider_2hm); break;
	case RE::WEAPON_TYPE::kHandToHandMelee: break;
	default: queueExecutionIdle(a_executor, a_victim, data::KM_Spider_1hm);
	}
}
void executionHandler::executeGargoyle(RE::Actor* a_executor, RE::Actor* a_victim, RE::WEAPON_TYPE weaponType) {
	switch (weaponType) {
	case RE::WEAPON_TYPE::kHandToHandMelee: break;
	case RE::WEAPON_TYPE::kTwoHandAxe: queueExecutionIdle(a_executor, a_victim, data::KM_Gargoyle_2hw); break;
	case RE::WEAPON_TYPE::kTwoHandSword: queueExecutionIdle(a_executor, a_victim, data::KM_Gargoyle_2hm); break;
	default: queueExecutionIdle(a_executor, a_victim, data::KM_Gargoyle_1hm); break;
	}
}
void executionHandler::executeGiant(RE::Actor* a_executor, RE::Actor* a_victim, RE::WEAPON_TYPE weaponType) {
	switch (weaponType) {
	case RE::WEAPON_TYPE::kHandToHandMelee: break;
	case RE::WEAPON_TYPE::kTwoHandAxe: queueExecutionIdle(a_executor, a_victim, data::KM_Giant_2hw); break;
	case RE::WEAPON_TYPE::kTwoHandSword: queueExecutionIdle(a_executor, a_victim, data::KM_Giant_2hm); break;
	default: queueExecutionIdle(a_executor, a_victim, data::KM_Giant_1hm); break;
	}
}
void executionHandler::executeBear(RE::Actor* a_executor, RE::Actor* a_victim, RE::WEAPON_TYPE weaponType) {
	switch (weaponType) {
	case RE::WEAPON_TYPE::kHandToHandMelee: break;
	case RE::WEAPON_TYPE::kTwoHandAxe: queueExecutionIdle(a_executor, a_victim, data::KM_Bear_2hw); break;
	case RE::WEAPON_TYPE::kTwoHandSword: queueExecutionIdle(a_executor, a_victim, data::KM_Bear_2hm); break;
	default: queueExecutionIdle(a_executor, a_victim, data::KM_Bear_1hm); break;
	}
}
void executionHandler::executeSabreCat(RE::Actor* a_executor, RE::Actor* a_victim, RE::WEAPON_TYPE weaponType) {
	switch (weaponType) {
	case RE::WEAPON_TYPE::kHandToHandMelee: break;
	case RE::WEAPON_TYPE::kTwoHandAxe: queueExecutionIdle(a_executor, a_victim, data::KM_SabreCat_2hw); break;
	case RE::WEAPON_TYPE::kTwoHandSword: queueExecutionIdle(a_executor, a_victim, data::KM_SabreCat_2hm); break;
	default: queueExecutionIdle(a_executor, a_victim, data::KM_SabreCat_1hm); break;
	}
}
void executionHandler::executeWolf(RE::Actor* a_executor, RE::Actor* a_victim, RE::WEAPON_TYPE weaponType) {
	switch (weaponType) {
	case RE::WEAPON_TYPE::kHandToHandMelee: break;
	case RE::WEAPON_TYPE::kTwoHandAxe: queueExecutionIdle(a_executor, a_victim, data::KM_Wolf_2hw); break;
	case RE::WEAPON_TYPE::kTwoHandSword: queueExecutionIdle(a_executor, a_victim, data::KM_Wolf_2hm); break;
	default: queueExecutionIdle(a_executor, a_victim, data::KM_Wolf_1hm); break;
	}
}
void executionHandler::executeTroll(RE::Actor* a_executor, RE::Actor* a_victim, RE::WEAPON_TYPE weaponType) {
	switch (weaponType) {
	case RE::WEAPON_TYPE::kHandToHandMelee: break;
	case RE::WEAPON_TYPE::kTwoHandAxe: queueExecutionIdle(a_executor, a_victim, data::KM_Troll_2hw); break;
	case RE::WEAPON_TYPE::kTwoHandSword: queueExecutionIdle(a_executor, a_victim, data::KM_Troll_2hm); break;
	default: queueExecutionIdle(a_executor, a_victim, data::KM_Troll_1hm); break;
	}
}
void executionHandler::executeHagraven(RE::Actor* a_executor, RE::Actor* a_victim, RE::WEAPON_TYPE weaponType) {
	switch (weaponType) {
	case RE::WEAPON_TYPE::kHandToHandMelee: break;
	case RE::WEAPON_TYPE::kTwoHandAxe: queueExecutionIdle(a_executor, a_victim, data::KM_Hagraven_2hw); break;
	case RE::WEAPON_TYPE::kTwoHandSword: queueExecutionIdle(a_executor, a_victim, data::KM_Hagraven_2hm);  break;
	default: queueExecutionIdle(a_executor, a_victim, data::KM_Hagraven_1hm); break;
	}
}
void executionHandler::executeSpriggan(RE::Actor* a_executor, RE::Actor* a_victim, RE::WEAPON_TYPE weaponType) {
	switch (weaponType) {
	case RE::WEAPON_TYPE::kHandToHandMelee: break;
	case RE::WEAPON_TYPE::kTwoHandAxe: queueExecutionIdle(a_executor, a_victim, data::KM_Spriggan_2hw); break;
	case RE::WEAPON_TYPE::kTwoHandSword: queueExecutionIdle(a_executor, a_victim, data::KM_Spriggan_2hm); break;
	default: queueExecutionIdle(a_executor, a_victim, data::KM_Spriggan_1hm); break;
	}
}
void executionHandler::executeBoar(RE::Actor* a_executor, RE::Actor* a_victim, RE::WEAPON_TYPE weaponType) {
	switch (weaponType) {
	case RE::WEAPON_TYPE::kHandToHandMelee: break;
	case RE::WEAPON_TYPE::kTwoHandAxe: queueExecutionIdle(a_executor, a_victim, data::KM_Boar_2hw); break;
	case RE::WEAPON_TYPE::kTwoHandSword: queueExecutionIdle(a_executor, a_victim, data::KM_Boar_2hm); break;
	default: queueExecutionIdle(a_executor, a_victim, data::KM_Boar_1hm); break;
	}
}
void executionHandler::executeRiekling(RE::Actor* a_executor, RE::Actor* a_victim, RE::WEAPON_TYPE weaponType) {
	switch (weaponType) {
	case RE::WEAPON_TYPE::kHandToHandMelee: break;
	case RE::WEAPON_TYPE::kTwoHandAxe: queueExecutionIdle(a_executor, a_victim, data::KM_Riekling_2hw); break;
	case RE::WEAPON_TYPE::kTwoHandSword: queueExecutionIdle(a_executor, a_victim, data::KM_Riekling_2hm); break;
	default: queueExecutionIdle(a_executor, a_victim, data::KM_Riekling_1hm); break;
	}
}
void executionHandler::executeAshHopper(RE::Actor* a_executor, RE::Actor* a_victim, RE::WEAPON_TYPE weaponType) {
	switch (weaponType) {
	case RE::WEAPON_TYPE::kHandToHandMelee: break;
	case RE::WEAPON_TYPE::kTwoHandAxe: queueExecutionIdle(a_executor, a_victim, data::KM_AshHopper_2hw); break;
	case RE::WEAPON_TYPE::kTwoHandSword: queueExecutionIdle(a_executor, a_victim, data::KM_AshHopper_2hm); break;
	default: queueExecutionIdle(a_executor, a_victim, data::KM_AshHopper_1hm); break;
	}
}
void executionHandler::executeSteamCenturion(RE::Actor* a_executor, RE::Actor* a_victim, RE::WEAPON_TYPE weaponType) {
	switch (weaponType) {
	case RE::WEAPON_TYPE::kHandToHandMelee: break;
	case RE::WEAPON_TYPE::kTwoHandAxe: queueExecutionIdle(a_executor, a_victim, data::KM_Centurion_2hw); break;
	case RE::WEAPON_TYPE::kTwoHandSword: queueExecutionIdle(a_executor, a_victim, data::KM_Centurion_2hm); break;
	default: queueExecutionIdle(a_executor, a_victim, data::KM_Centurion_1hm); break;
	}
}
void executionHandler::executeDwarvenBallista(RE::Actor* a_executor, RE::Actor* a_victim, RE::WEAPON_TYPE weaponType) {
	switch (weaponType) {
	case RE::WEAPON_TYPE::kHandToHandMelee: break;
	case RE::WEAPON_TYPE::kTwoHandAxe: queueExecutionIdle(a_executor, a_victim, data::KM_Ballista_2hw); break;
	case RE::WEAPON_TYPE::kTwoHandSword: queueExecutionIdle(a_executor, a_victim, data::KM_Ballista_2hm); break;
	default: queueExecutionIdle(a_executor, a_victim, data::KM_Ballista_1hm); break;
	}
}
void executionHandler::executeChaurusFlyer(RE::Actor* a_executor, RE::Actor* a_victim, RE::WEAPON_TYPE weaponType) {
	switch (weaponType) {
	case RE::WEAPON_TYPE::kHandToHandMelee: break;
	case RE::WEAPON_TYPE::kTwoHandAxe: queueExecutionIdle(a_executor, a_victim, data::KM_ChaurusFlyer_2hw); break;
	case RE::WEAPON_TYPE::kTwoHandSword: queueExecutionIdle(a_executor, a_victim, data::KM_ChaurusFlyer_2hm); break;
	default: queueExecutionIdle(a_executor, a_victim, data::KM_ChaurusFlyer_1hm); break;
	}
}
void executionHandler::executeLurker(RE::Actor* a_executor, RE::Actor* a_victim, RE::WEAPON_TYPE weaponType) {
	switch (weaponType) {
	case RE::WEAPON_TYPE::kHandToHandMelee: break;
	case RE::WEAPON_TYPE::kTwoHandAxe: queueExecutionIdle(a_executor, a_victim, data::KM_Lurker_2hw); break;
	case RE::WEAPON_TYPE::kTwoHandSword: queueExecutionIdle(a_executor, a_victim, data::KM_Lurker_2hm); break;
	default: queueExecutionIdle(a_executor, a_victim, data::KM_Lurker_1hm); break;
	}
}
void executionHandler::executeDragon(RE::Actor* a_executor, RE::Actor* a_victim, RE::WEAPON_TYPE weaponType) {
	switch (weaponType) {
	case RE::WEAPON_TYPE::kHandToHandMelee: break;
	case RE::WEAPON_TYPE::kTwoHandAxe: queueExecutionIdle(a_executor, a_victim, data::KM_Dragon_2hw); break;
	case RE::WEAPON_TYPE::kTwoHandSword: queueExecutionIdle(a_executor, a_victim, data::KM_Dragon_2hm); break;
	default: queueExecutionIdle(a_executor, a_victim, data::KM_Dragon_1hm); break;
	}
}

