#include "include/Utils.h"
#include "ValhallaCombat.hpp"

bool Utils::Actor::isHumanoid(RE::Actor* a_actor)
{
	if (!a_actor) {
		return false;
	}
	auto bodyPartData = a_actor->GetRace() ? a_actor->GetRace()->bodyPartData : nullptr;
	return bodyPartData && bodyPartData->GetFormID() == 0x1d;
}

RE::TESObjectWEAP* Utils::Actor::getWieldingWeapon(RE::Actor* a_actor)
{
	if (!a_actor) {
		return nullptr;
	}
	auto weapon = a_actor->GetAttackingWeapon();
	if (weapon && weapon->object) {
		return weapon->object->As<RE::TESObjectWEAP>();
	}
	auto rhs = a_actor->GetEquippedObject(false);
	if (rhs && rhs->IsWeapon()) {
		return rhs->As<RE::TESObjectWEAP>();
	}
	auto lhs = a_actor->GetEquippedObject(true);
	if (lhs && lhs->IsWeapon()) {
		return lhs->As<RE::TESObjectWEAP>();
	}
	return nullptr;
}

bool Utils::Actor::isDualWielding(RE::Actor* a_actor)
{
	if (!a_actor) {
		return false;
	}
	auto lhs = a_actor->GetEquippedObject(true);
	auto rhs = a_actor->GetEquippedObject(false);
	if (lhs && rhs && lhs->IsWeapon() && rhs->IsWeapon()) {
		auto weaponType = rhs->As<RE::TESObjectWEAP>()->GetWeaponType();
		return weaponType != RE::WEAPON_TYPE::kTwoHandAxe && weaponType != RE::WEAPON_TYPE::kTwoHandSword;  //can't be two hand sword.
	}
	return false;
}

bool Utils::Actor::isEquippedShield(RE::Actor* a_actor)
{
	if (!a_actor) {
		return false;
	}
	return RE::Offset::getEquippedShield(a_actor);
}

void Utils::Actor::damageav(RE::Actor* a, RE::ActorValue av, float val)
{
	if (val == 0 || !a) {
		return;
	}
	if (a) {
		a->AsActorValueOwner()->DamageActorValue(av, val);
	}
}

/*Try to damage this actor's actorvalue. If the actor does not have enough value, do not damage and return false;*/
bool Utils::Actor::tryDamageAv(RE::Actor* a_actor, RE::ActorValue a_actorValue, float a_damage)
{
	if (!a_actor) {
		return false;
	}

	auto avOwner = a_actor->AsActorValueOwner();
	if (!avOwner) {
		return false;
	}

	auto currentAv = avOwner->GetActorValue(a_actorValue);
	if (currentAv - a_damage <= 0.0f) {
		return false;
	}
	damageav(a_actor, a_actorValue, a_damage);
	return true;
}

void Utils::Actor::restoreav(RE::Actor* a_actor, RE::ActorValue a_actorValue, float a_value)
{
	if (a_value == 0 || !a_actor) {
		return;
	}
	if (a_actor) {
		a_actor->AsActorValueOwner()->RestoreActorValue(a_actorValue, a_value);
	}
}

/*Complete refills this actor's actor value.
		@param a_actor actor whose actorValue will be refilled.
		@param actorValue type of actor value to refill.*/
void Utils::Actor::refillActorValue(RE::Actor* a_actor, RE::ActorValue a_actorValue)
{
	if (!a_actor) {
		return;
	}

	auto avOwner = a_actor->AsActorValueOwner();
	if (!avOwner) {
		return;
	}

	float av = avOwner->GetActorValue(a_actorValue);
	float pav = avOwner->GetPermanentActorValue(a_actorValue);
	if (av >= pav) {
		return;
	}
	float avToRestore = pav - av;
	restoreav(a_actor, a_actorValue, avToRestore);
}

void Utils::Actor::safeApplySpell(RE::SpellItem* a_spell, RE::Actor* a_actor)
{
	if (a_actor && a_spell && !a_actor->HasSpell(a_spell)) {
		a_actor->AddSpell(a_spell);
	}
}

void Utils::Actor::safeRemoveSpell(RE::SpellItem* a_spell, RE::Actor* a_actor)
{
	if (a_actor && a_spell && a_actor->HasSpell(a_spell)) {
		a_actor->RemoveSpell(a_spell);
	}
}

void Utils::Actor::safeApplyPerk(RE::BGSPerk* a_perk, RE::Actor* a_actor)
{
	if (a_actor && a_perk && !a_actor->HasPerk(a_perk)) {
		a_actor->AddPerk(a_perk);
	}
}

void Utils::Actor::safeRemovePerk(RE::BGSPerk* a_perk, RE::Actor* a_actor)
{
	if (a_actor && a_perk && a_actor->HasPerk(a_perk)) {
		a_actor->RemovePerk(a_perk);
	}
}

bool Utils::Actor::isBackFacing(RE::Actor* actor1, RE::Actor* actor2)
{
	if (!actor1||!actor2) {
		return false;
	}
	auto angle = actor1->GetHeadingAngle(actor2->GetPosition(), false);
	if (90 < angle || angle < -90) {
		return true;
	}
	else {
		return false;
	}
}

bool Utils::Actor::isJumping(RE::Actor* a_actor)
{
	if (!a_actor) {
		return false;
	}

	bool result = false;
	return a_actor->GetGraphVariableBool("bInJumpState", result) && result;
}

bool Utils::Actor::isStaggering(RE::Actor* a_actor)
{
	if (!a_actor) {
		return false;
	}

	bool result = false;
	return a_actor->GetGraphVariableBool("IsStaggering", result) && result;
}

bool Utils::Actor::isUnblockable(RE::Actor* a_actor)
{
	if (!a_actor) {
		return false;
	}

	bool result = false;
	return a_actor->GetGraphVariableBool("Val_Unblockable", result) && result;
}

bool Utils::Actor::isDodging(RE::Actor* a_actor)
{
	if (!a_actor) {
		return false;
	}

	bool result = false;
	return a_actor->GetGraphVariableBool("bIsDodging", result) && result;
}

bool Utils::Actor::isBlocking(RE::Actor* a_actor)
{
	if (!a_actor) {
		return false;
	}
	/*bool result = false;
	return a_actor->GetGraphVariableBool("IsBlocking", result) && result;*/
	return a_actor->IsBlocking();
}

bool Utils::Actor::isAttacking(RE::Actor* a_actor)
{
	if (!a_actor) {
		return false;
	}
	/*RE::ActorState* state = a_actor->AsActorState();
	if (!state) {
		return false;
	}
	return state->GetAttackState() != RE::ATTACK_STATE_ENUM::kNone;*/
	return a_actor->IsAttacking();
}

/* Whether the actor is blocking or is in a state at which a block input transfer the actor into blocking state.*/
bool Utils::Actor::canBlock(RE::Actor* a_actor)
{
	if (!a_actor) {
		return false;
	}
	if (Utils::Actor::isBlocking(a_actor)){
		return true;
	}
	RE::ActorState* state = a_actor->AsActorState();
	if (!state) {
		return false;
	}
	return !isJumping(a_actor) &&
		state->GetKnockState() == RE::KNOCK_STATE_ENUM::kNormal &&
		state->GetAttackState() == RE::ATTACK_STATE_ENUM::kNone &&
		!state->IsSwimming() && state->IsWeaponDrawn() &&
		!state->IsSprinting() && !isStaggering(a_actor) && !isDodging(a_actor);
}

bool Utils::Actor::canReact(RE::Actor* a_actor)
{
	if (!a_actor) {
		return false;
	}
	RE::ActorState* state = a_actor->AsActorState();
	if (!state) {
		return false;
	}
	return !isJumping(a_actor) &&
		state->GetKnockState() == RE::KNOCK_STATE_ENUM::kNormal &&
		state->GetAttackState() == RE::ATTACK_STATE_ENUM::kNone &&
		!state->IsSwimming() && state->IsWeaponDrawn() &&
		!state->IsSprinting() && !isStaggering(a_actor) && !isDodging(a_actor);
}

bool Utils::Actor::canExecute(RE::Actor* a_actor)
{
	if (!a_actor) {
		return false;
	}
	RE::ActorState* state = a_actor->AsActorState();
	if (!state) {
		return false;
	}
	return !isJumping(a_actor) && state->GetKnockState() == RE::KNOCK_STATE_ENUM::kNormal &&
	       !state->IsSwimming() && state->IsWeaponDrawn() &&
	       !isStaggering(a_actor) && !isDodging(a_actor);
}

bool Utils::Actor::isInBlockAngle(RE::Actor* a_blocker, RE::TESObjectREFR* a_obj)
{
	if (!a_blocker || !a_obj) {
		return false;
	}
	auto angle = a_blocker->GetHeadingAngle(a_obj->GetPosition(), false);
	return (angle <= data::fCombatHitConeAngle && angle >= -data::fCombatHitConeAngle);
}

bool Utils::Actor::isPowerBashing(RE::Actor* a_actor)
{
	if (!a_actor) {
		return false;
	}

	auto currentProcess = a_actor->GetActorRuntimeData().currentProcess;
	if (currentProcess && currentProcess->high && currentProcess->high->attackData) {
		auto flags = currentProcess->high->attackData->data.flags;

		// Power Bashing requires BOTH flags to be active
		return flags.any(RE::AttackData::AttackFlag::kPowerAttack) &&
			flags.any(RE::AttackData::AttackFlag::kBashAttack);
	}
	return false;
}

bool Utils::Actor::isPowerAttacking(RE::Actor* a_actor)
{
	if (!a_actor) {
		return false;
	}

	auto currentProcess = a_actor->GetActorRuntimeData().currentProcess;
	if (currentProcess && currentProcess->high && currentProcess->high->attackData) {
		auto flags = currentProcess->high->attackData->data.flags;

		// Regular Power Attack: Has kPowerAttack, but MUST NOT have kBashAttack
		return flags.any(RE::AttackData::AttackFlag::kPowerAttack) &&
			!flags.any(RE::AttackData::AttackFlag::kBashAttack);
	}
	return false;
}

bool Utils::Actor::isBashing(RE::Actor* a_actor)
{
	if (!a_actor) {
		return false;
	}

	// Check if we are in a power bash first to maintain exclusivity
	if (isPowerBashing(a_actor)) {
		return false;
	}

	// Native enum state check (this usually denotes a regular light bash)
	if (a_actor->AsActorState()->GetAttackState() == RE::ATTACK_STATE_ENUM::kBash) {
		return true;
	}

	auto currentProcess = a_actor->GetActorRuntimeData().currentProcess;
	if (currentProcess && currentProcess->high && currentProcess->high->attackData) {
		auto flags = currentProcess->high->attackData->data.flags;

		// Regular Light Bash: Has kBashAttack, but MUST NOT have kPowerAttack
		return flags.any(RE::AttackData::AttackFlag::kBashAttack) &&
			!flags.any(RE::AttackData::AttackFlag::kPowerAttack);
	}
	return false;
}

bool Utils::Actor::isLightAttacking(RE::Actor* a_actor)
{
	if (!a_actor) {
		return false;
	}

	// Maintain exclusivity against power attacks and bashes
	/*if (isPowerAttacking(a_actor) || isBashing(a_actor) || isPowerBashing(a_actor)) {
		return false;
	}*/

	auto currentProcess = a_actor->GetActorRuntimeData().currentProcess;
	if (currentProcess && currentProcess->high && currentProcess->high->attackData) {
		auto flags = currentProcess->high->attackData->data.flags;

		// Regular Light Attack: Active attackData, but MUST NOT have kPowerAttack or kBashAttack
		return !flags.any(RE::AttackData::AttackFlag::kPowerAttack) &&
			!flags.any(RE::AttackData::AttackFlag::kBashAttack);
	}
	return false;
}

// Checks if attacker is facing towards the target within a given cone angle (default +-60 degrees)
bool Utils::Actor::isFacingTarget(RE::Actor * a_attacker, RE::Actor * a_target, float a_maxConeDegrees)
{
	if (!a_attacker || !a_target) {
		return false;
	}

	float headingAngle = a_attacker->GetHeadingAngle(a_target->GetPosition(), false);
	return (headingAngle >= -a_maxConeDegrees && headingAngle <= a_maxConeDegrees);
}

// Calculates weapon reach based on equipped weapon, race unarmed reach, or fallback
float Utils::Actor::GetWeaponReach(RE::Actor* a_attacker)
{
	if (!a_attacker || !a_attacker->Is3DLoaded()) {
		return 150.0f;
	}

	// 1. Try Precision API via your ValhallaCombat singleton
	auto val = ValhallaCombat::GetSingleton();
	if (val && val->ersh_Precision) {
		RE::ActorHandle handle = a_attacker->GetHandle();
		float precisionReach = val->ersh_Precision->GetAttackCollisionCapsuleLength(
			handle,
			PRECISION_API::RequestedAttackCollisionType::Default
		);

		if (precisionReach > 0.0f) {
			return precisionReach;
		}
	}

	// 2. Fallback: Vanilla Weapon Reach Multiplier
	RE::TESForm* weapon = a_attacker->GetEquippedObject(false); // Right hand
	if (!weapon) {
		weapon = a_attacker->GetEquippedObject(true); // Left hand
	}

	if (weapon) {
		if (auto weap = weapon->As<RE::TESObjectWEAP>()) {
			float reachMultiplier = weap->GetReach();
			if (reachMultiplier > 0.0f) {
				return reachMultiplier * 100.0f; // Convert ~1.0-1.3 multiplier to game units
			}
		}
	}

	// 3. Fallback: Unarmed Race Reach
	if (auto race = a_attacker->GetRace()) {
		float unarmedReach = race->data.unarmedReach;
		if (unarmedReach > 0.0f) {
			return unarmedReach;
		}
	}

	// 4. Hardcoded Fallback
	return 150.0f;
}

// Determines if target is within attacker's physical reach cone
bool Utils::Actor::isInAttackRange(RE::Actor * a_attacker, RE::Actor * a_target, float a_bufferDistance)
{
	if (!a_attacker || !a_target) {
		return false;
	}

	float currentDistance = a_attacker->GetPosition().GetDistance(a_target->GetPosition());
	float maxReach = GetWeaponReach(a_attacker) + a_bufferDistance; // Buffer accounts for body radiuses

	return currentDistance <= maxReach;
}

//AI Utils

RE::Actor* Utils::Actor::GetCombatTarget(RE::Actor* a_actor)
{
	if (!a_actor) {
		return nullptr;
	}
	auto combatController = a_actor->GetActorRuntimeData().combatController;

	if (combatController) {
		if (auto targetHandle = combatController->targetHandle) {
			return targetHandle.get().get();
		}
	}

	return nullptr;
}

float Utils::Actor::GetConfidence(RE::Actor* a_defender, RE::Actor* a_attacker, float a_mult, float a_min, float a_max)
{
	if (!a_defender || !a_attacker) {
		return 1.0f;
	}

	float defenderLevel = static_cast<float>(a_defender->GetLevel());
	float attackerLevel = static_cast<float>(a_attacker->GetLevel());

	if (defenderLevel <= 0.0f) {
		defenderLevel = 1.0f;
	}
	if (attackerLevel <= 0.0f) {
		attackerLevel = 1.0f;
	}

	float levelRatio = defenderLevel / attackerLevel;
	float confidence = (std::max)(a_min, 1.0f + (levelRatio - 1.0f) * a_mult);

	return (std::min)(a_max, confidence);
}

float Utils::Actor::GetSkillLevel(RE::Actor* a_actor, CombatSkill a_skill)
{
	if (!a_actor) {
		return 0.0f;
	}

	auto avOwner = a_actor->AsActorValueOwner();
	if (!avOwner) {
		return 0.0f;
	}

	RE::ActorValue av = RE::ActorValue::kNone;

	switch (a_skill) {
	case CombatSkill::Block:
		av = RE::ActorValue::kBlock;
		break;
	case CombatSkill::OneHanded:
		av = RE::ActorValue::kOneHanded;
		break;
	case CombatSkill::TwoHanded:
		av = RE::ActorValue::kTwoHanded;
		break;
	case CombatSkill::LightArmor:
		av = RE::ActorValue::kLightArmor;
		break;
	case CombatSkill::HeavyArmor:
		av = RE::ActorValue::kHeavyArmor;
		break;
	case CombatSkill::Archery:
		av = RE::ActorValue::kArchery;
		break;
	}

	if (av == RE::ActorValue::kNone) {
		return 0.0f;
	}

	// Retrieves the effective current skill level (including armor/potion fortify modifiers)
	return avOwner->GetActorValue(av);
}

RE::TESCombatStyle* Utils::Actor::GetCombatStyle(RE::Actor* a_actor)
{
	if (!a_actor) {
		return nullptr;
	}

	// 1. Check runtime CombatController (if actor is actively in combat)
	auto combatController = a_actor->GetActorRuntimeData().combatController;
	if (combatController) {
		if (combatController->combatStyle) {
			return combatController->combatStyle;
		}
	}

	// 2. Fallback to base NPC record
	if (auto actorBase = a_actor->GetActorBase()) {
		return actorBase->combatStyle;
	}

	return nullptr;
}

float Utils::Actor::GetCombatStyleMult(RE::Actor* a_actor, CombatStyleMult a_multType)
{
	if (!a_actor) {
		return 0.0f;
	}

	auto style = GetCombatStyle(a_actor);
	if (!style) {
		// Engine defaults when no combat style is found
		switch (a_multType) {
		case CombatStyleMult::Offensive:
		case CombatStyleMult::Defensive:
			return 1.0f;
		default:
			return 0.0f;
		}
	}

	switch (a_multType) {
	case CombatStyleMult::Bash:
		return style->meleeData.bashMult;
	case CombatStyleMult::BashAttack:
		return style->meleeData.bashAttackMult;
	case CombatStyleMult::BashPowerAttack:
		return style->meleeData.bashPowerAttackMult;
	case CombatStyleMult::Offensive:
		return style->generalData.offensiveMult;
	case CombatStyleMult::Defensive:
		return style->generalData.defensiveMult;
	}

	return 0.0f;
}

void Utils::Actor::applyPerkEntryPoint(std::int32_t a_entryPoint, RE::Actor* a_perkOwner, RE::Actor* a_target, float* a_value)
{
	using EntryPoint = RE::BGSEntryPoint::ENTRY_POINTS::ENTRY_POINT;
	if (a_perkOwner && a_value) {
		RE::BGSEntryPoint::HandleEntryPoint(static_cast<EntryPoint>(a_entryPoint), a_perkOwner, a_target, a_value);
	}
}

bool Utils::Actor::performAction(RE::BGSAction* a_action, RE::Actor* a_actor)
{
	if (!a_action || !a_actor) {
		return false;
	}

	auto data = RE::TESActionData::Create();
	if (!data) {
		return false;
	}

	data->source = RE::NiPointer<RE::TESObjectREFR>(a_actor);
	data->action = a_action;

	bool result = data->Process();

	data->~TESActionData();
	RE::free(data);
	data = nullptr;

	return result;
}

// Behavior Graph Utils

namespace Utils::Actor
{
	static const robin_hood::unordered_map<std::string, BehaviorGraph> kBehaviorGraphMap = {
		{ "defaultmale",                    BehaviorGraph::DefaultMale },
		{ "defaultfemale",                  BehaviorGraph::DefaultFemale },
		{ "dragonproject",                  BehaviorGraph::Dragon },
		{ "giantproject",                   BehaviorGraph::Giant },
		{ "falmerproject",                  BehaviorGraph::Falmer },
		{ "atronachflame",                  BehaviorGraph::AtronachFlame },
		{ "atronachfrostproject",           BehaviorGraph::AtronachFrost },
		{ "atronachstormproject",           BehaviorGraph::AtronachStorm },
		{ "bearproject",                    BehaviorGraph::Bear },
		{ "chaurusproject",                 BehaviorGraph::Chaurus },
		{ "chaurusflyer",                   BehaviorGraph::ChaurusFlyer },
		{ "dragon_priest",                  BehaviorGraph::DragonPriest },
		{ "draugrproject",                  BehaviorGraph::Draugr },
		{ "steamproject",                   BehaviorGraph::SteamCenturion },
		{ "spherecenturion",                BehaviorGraph::SphereCenturion },
		{ "dwarvenspidercenturionproject",  BehaviorGraph::SpiderCenturion },
		{ "ballistacenturion",              BehaviorGraph::BallistaCenturion },
		{ "frostbitespiderproject",         BehaviorGraph::FrostbiteSpider },
		{ "hagravenproject",                BehaviorGraph::Hagraven },
		{ "horkerproject",                  BehaviorGraph::Horker },
		{ "horseproject",                   BehaviorGraph::Horse },
		{ "icewraithproject",               BehaviorGraph::IceWraith },
		{ "mammothproject",                 BehaviorGraph::Mammoth },
		{ "sabrecatproject",                BehaviorGraph::SabreCat },
		{ "skeeverproject",                 BehaviorGraph::Skeever },
		{ "draugrskeletonproject",          BehaviorGraph::DraugrSkeleton },
		{ "spriggan",                       BehaviorGraph::Spriggan },
		{ "trollproject",                   BehaviorGraph::Troll },
		{ "werewolfbeastproject",           BehaviorGraph::WerewolfBeast },
		{ "wolfproject",                    BehaviorGraph::Wolf },
		{ "dogproject",                     BehaviorGraph::Dog },
		{ "vampirebruteproject",            BehaviorGraph::VampireBrute },
		{ "vampirelord",                    BehaviorGraph::VampireLord },
		{ "benthiclurkerproject",           BehaviorGraph::BenthicLurker },
		{ "boarproject",                    BehaviorGraph::Boar },
		{ "rieklingproject",                BehaviorGraph::Riekling },
		{ "mudcrabproject",                 BehaviorGraph::Mudcrab },
		{ "hmdaedra",                       BehaviorGraph::HMDaedra },
		{ "netchproject",                   BehaviorGraph::Netch }
	};

	BehaviorGraph ParseBehaviorGraph(std::string_view a_projectName)
	{
		if (a_projectName.empty()) {
			return BehaviorGraph::None;
		}

		std::string lowerName = inlineUtils::to_lower_string(a_projectName);

		auto it = kBehaviorGraphMap.find(lowerName);
		if (it != kBehaviorGraphMap.end()) {
			return it->second;
		}

		return BehaviorGraph::None;
	}

	BehaviorGraph GetBehaviorGraph(RE::Actor* a_actor)
	{
		if (!a_actor) {
			return BehaviorGraph::None;
		}

		RE::BSTSmartPointer<RE::BSAnimationGraphManager> animGraphMgr;
		if (a_actor->GetAnimationGraphManager(animGraphMgr) && animGraphMgr) {
			for (const auto& graph : animGraphMgr->graphs) {
				if (graph && !graph->projectName.empty()) {
					BehaviorGraph type = ParseBehaviorGraph(graph->projectName.c_str());
					if (type != BehaviorGraph::None) {
						return type;
					}
				}
			}
		}

		return BehaviorGraph::None;
	}

	bool isAISupportedNPC(RE::Actor* a_actor)
	{
		if (!a_actor || a_actor->IsPlayerRef() || a_actor->IsDead()) {
			return false;
		}

		BehaviorGraph graph = GetBehaviorGraph(a_actor);
		switch (graph) {
		case BehaviorGraph::DefaultMale:
		case BehaviorGraph::DefaultFemale:
		case BehaviorGraph::Draugr:
		case BehaviorGraph::Falmer:
		case BehaviorGraph::DraugrSkeleton:
			return true;
		default:
			return false;
		}
	}
}

// Projectile Utils

void Utils::Projectile::deflect(RE::Actor* a_deflector, RE::Projectile* a_projectile, RE::hkpCollidable* a_projectile_collidable)
{
	if (!a_deflector || !a_projectile) {
		return;
	}

	RE::TESObjectREFR* shooter = nullptr;
	auto& runtimeData = a_projectile->GetProjectileRuntimeData();
	if (runtimeData.shooter && runtimeData.shooter.get()) {
		shooter = runtimeData.shooter.get().get();
	}

	ValhallaUtils::resetProjectileOwner(a_projectile, a_deflector, a_projectile_collidable);

	if (shooter && shooter->Is3DLoaded()) {
		ValhallaUtils::RetargetProjectile(a_projectile, shooter);
	}
	else {
		ValhallaUtils::ReflectProjectile(a_projectile);
	}
}

// Randomizer Utils

namespace Utils::Random
{
	static thread_local std::mt19937 generator([]() {
		std::random_device rd;
		return rd();
		}());

	float GetFloat(float a_min, float a_max)
	{
		if (std::isnan(a_min) || std::isnan(a_max) || std::isinf(a_min) || std::isinf(a_max)) {
			return 0.0f;
		}

		if (a_min >= a_max) {
			return a_min;
		}

		std::uniform_real_distribution<float> dist(a_min, a_max);
		return dist(generator);
	}

	int GetInt(int a_min, int a_max)
	{
		if (a_min >= a_max) {
			return a_min;
		}
		std::uniform_int_distribution<int> dist(a_min, a_max);
		return dist(generator);
	}

	bool RollChance(float a_chancePercent)
	{
		if (std::isnan(a_chancePercent) || std::isinf(a_chancePercent) || a_chancePercent <= 0.0f) {
			return false;
		}
		if (a_chancePercent >= 100.0f) {
			return true;
		}
		return GetFloat(0.0f, 100.0f) <= a_chancePercent;
	}

	bool RollNormalized(float a_chanceNormalized)
	{
		if (std::isnan(a_chanceNormalized) || std::isinf(a_chanceNormalized) || a_chanceNormalized <= 0.0f) {
			return false;
		}
		if (a_chanceNormalized >= 1.0f) {
			return true;
		}
		return GetFloat(0.0f, 1.0f) <= a_chanceNormalized;
	}
}
