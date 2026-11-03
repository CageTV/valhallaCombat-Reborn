#include "include/blockHandler.h"
#include "include/data.h"
#include "include/settings.h"
#include "include/Utils.h"
#include "include/stunHandler.h"
#include "include/parryHandler.h"
#include "include/reactionHandler.h"
#include "include/AI.h"
#include "include/lib/BlockSpark.h"
#include "include/offsets.h"
#include "include/staminaHandler.h"
#include "include/Hooks.h"
#include "ValhallaCombat.hpp"
#include "include/perks.h"
#include "include/specialAttackHandler.h"

using HITFLAG = RE::HitData::Flag;

/*Called every frame. Decrement timers for active blocking actors.*/
void blockHandler::update() {
	float deltaTime = *RE::Offset::g_deltaTime;

	// Exclusive write lock required because timers and erase map entries are mutated
	std::unique_lock<std::shared_mutex> lock(mtx_blockStates);

	for (auto it = _blockStates.begin(); it != _blockStates.end(); ) {
		auto& state = it->second;

		// Timed Block Timers
		if (state.isTimedBlocking) {
			state.timedBlockTimer -= deltaTime;
			if (state.timedBlockTimer <= 0.0f) {
				onTimedBlockEnd(state);
			}
		}

		if (state.isBlockingCooldown) {
			state.timedBlockCooldownTimer -= deltaTime;
			if (state.timedBlockCooldownTimer <= 0.0f) {
				state.isBlockingCooldown = false;
			}
		}

		// Tackle Timers
		if (state.isTackling) {
			state.tackleTimer -= deltaTime;
			if (state.tackleTimer <= 0.0f) {
				state.isTackling = false;
				state.isTackleCooldown = true;
				state.tackleCooldownTimer = settings::fTackleCooldownTime;
			}
		}

		if (state.isTackleCooldown) {
			state.tackleCooldownTimer -= deltaTime;
			if (state.tackleCooldownTimer <= 0.0f) {
				state.isTackleCooldown = false;
			}
		}

		// Cleanup inactive entries
		if (!state.isTimedBlocking &&
			!state.isBlockingCooldown &&
			!state.isTackling &&
			!state.isTackleCooldown) {
			it = _blockStates.erase(it);
		}
		else {
			++it;
		}
	}

	if (_blockStates.empty()) {
		ValhallaCombat::GetSingleton()->deactivateUpdate(ValhallaCombat::HANDLER::blockHandler);
	}
}

void blockHandler::onBlockStart(RE::Actor* a_actor) {
	if (!a_actor|| !Utils::Actor::canBlock(a_actor)) {
		return;
	}

	// Block Commitment Initiation
	bool blockCommitmentAllowed = false;
	if (a_actor->IsPlayerRef()) {
		blockCommitmentAllowed = settings::bBlockCommitmentToggle;
	}
	else {
		blockCommitmentAllowed = settings::bBlockCommitmentToggle && settings::bBlockCommitmentToggle_NPC;
	}

	if (blockCommitmentAllowed) {
		BlockCommitment::GetSingleton()->StartTimer(a_actor);
		//logger::info("Block Commitment: Start Timer");
	}

	// Exclusive write lock to add or modify entry
	std::unique_lock<std::shared_mutex> lock(mtx_blockStates);
	auto& state = _blockStates[a_actor->GetFormID()];

	// Timed Block Initiation
	bool timedBlockMeleeAllowed = false;
	if (a_actor->IsPlayerRef()) {
		timedBlockMeleeAllowed = settings::bTimedBlockToggle && perks::canTimedBlock(a_actor);
	}
	else {
		timedBlockMeleeAllowed = settings::bTimedBlockToggle && settings::bTimedBlockToggle_NPC
			&& perks::canTimedBlock(a_actor);
	}

	bool timedBlockProjectileAllowed = false;
	if (a_actor->IsPlayerRef()) {
		timedBlockProjectileAllowed = settings::bTimedBlockProjectileToggle && perks::canTimedProjectileBlock(a_actor);
	}
	else {
		timedBlockProjectileAllowed = settings::bTimedBlockProjectileToggle && settings::bTimedBlockProjectileToggle_NPC
			&& perks::canTimedProjectileBlock(a_actor);
	}

	if (timedBlockMeleeAllowed || timedBlockProjectileAllowed) {
		if (state.isTimedBlocking) {
			onTimedBlockEnd(state);
		}
		//logger::info("Timed Blocking: Start Timed Block");
		state.isTimedBlocking = true;
		state.isTimedBlockSuccess = false;

		float blockWindow = settings::fTimedBlockWindow;
		switch (state.blockWindowPenalty) {
		case blockWindowPenaltyLevel::none: break;
		case blockWindowPenaltyLevel::light: blockWindow *= 0.8f; break;
		case blockWindowPenaltyLevel::medium: blockWindow *= 0.5f; break;
		case blockWindowPenaltyLevel::heavy: blockWindow *= 0.3f; break;
		}
		state.timedBlockTimer = blockWindow;
	}

	/*Tackle Initiation
	bool tackleAllowed = false;
	if (a_actor->IsPlayerRef()) {
		tackleAllowed = settings::bTackleToggle && perks::canTackle(a_actor);
	}
	else {
		tackleAllowed = settings::bTackleToggle && settings::bTackleToggle_NPC && perks::canTackle(a_actor);
	}

	if (tackleAllowed && !state.isTackleCooldown && !state.isTackling) {
		state.isTackling = true;
		state.tackleTimer = settings::fTackleWindow;
		//logger::info("Tackle: Start Tackle");
	}*/

	ValhallaCombat::GetSingleton()->activateUpdate(ValhallaCombat::HANDLER::blockHandler);
}

void blockHandler::onBlockStop(RE::Actor* a_actor) {
	if (!a_actor) {
		return;
	}
	// Exclusive write lock to add or modify entry
	std::unique_lock<std::shared_mutex> lock(mtx_blockStates);
	auto it = _blockStates.find(a_actor->GetFormID());
	if (it != _blockStates.end()) {
		onTimedBlockEnd(it->second);
	}
}

void blockHandler::onTimedBlockEnd(ActorBlockState& state) {
	if (!state.isTimedBlockSuccess) {
		state.isBlockingCooldown = true;
		state.timedBlockCooldownTimer = settings::fTimedBlockCooldownTime;
		pcBlockWindowPenalty_update(state.blockWindowPenalty, true);
		ValhallaCombat::GetSingleton()->activateUpdate(ValhallaCombat::HANDLER::blockHandler);
	}
	else {
		state.isTimedBlockSuccess = false;
	}
	state.isTimedBlocking = false;
}

void blockHandler::onSuccessfulTimedBlock(RE::Actor* a_actor) {
	if (!a_actor) {
		return;
	}
	// Exclusive write lock to add or modify entry
	std::unique_lock<std::shared_mutex> lock(mtx_blockStates);
	auto it = _blockStates.find(a_actor->GetFormID());
	if (it != _blockStates.end()) {
		it->second.isTimedBlockSuccess = true;
		it->second.blockWindowPenalty = blockWindowPenaltyLevel::none;
	}
}

bool blockHandler::getIsTimedBlocking(RE::Actor* a_actor){
	if (!a_actor) {
		return false;
	}
	/*if (settings::bIsBlockingTimedBlock) {
		bool isBlocking = false;
		if (!a_actor->GetGraphVariableBool("IsBlocking", isBlocking) || !isBlocking) {
			return false;
		}
	}*/
	// Shared read lock allows multiple threads to read concurrently
	std::shared_lock<std::shared_mutex> lock(mtx_blockStates);
	auto it = _blockStates.find(a_actor->GetFormID());
	return it != _blockStates.end() && it->second.isTimedBlocking;
}

bool blockHandler::getIsPerfectBlocking(RE::Actor* a_actor){
	if (!a_actor) {
		return false;
	}
	/*if (settings::bIsBlockingTimedBlock) {
		bool isBlocking = false;
		if (!a_actor->GetGraphVariableBool("IsBlocking", isBlocking) || !isBlocking) {
			return false;
		}
	}*/
	// Shared read lock allows multiple threads to read concurrently
	std::shared_lock<std::shared_mutex> lock(mtx_blockStates);
	auto it = _blockStates.find(a_actor->GetFormID());
	return it != _blockStates.end() && isTimedBlockElapsedTimeLessThan(it->second, settings::fPerfectBlockWindow);
}

bool blockHandler::getIsTackling(RE::Actor* a_actor){
	if (!a_actor) {
		return false;
	}
	// Shared read lock allows multiple threads to read concurrently
	std::shared_lock<std::shared_mutex> lock(mtx_blockStates);
	auto it = _blockStates.find(a_actor->GetFormID());
	return it != _blockStates.end() && it->second.isTackling;
}

inline void blockHandler::pcBlockWindowPenalty_update(blockWindowPenaltyLevel& a_penaltyLevel, bool increase) {
	if (increase) {
		switch (a_penaltyLevel) {
		case blockWindowPenaltyLevel::none: a_penaltyLevel = light; break;
		case blockWindowPenaltyLevel::light: a_penaltyLevel = medium; break;
		case blockWindowPenaltyLevel::medium: a_penaltyLevel = heavy; break;
		}
	}
	else {
		switch (a_penaltyLevel) {
		case blockWindowPenaltyLevel::light: a_penaltyLevel = none; break;
		case blockWindowPenaltyLevel::medium: a_penaltyLevel = light; break;
		case blockWindowPenaltyLevel::heavy: a_penaltyLevel = medium; break;
		}
	}
}

inline bool blockHandler::isTimedBlockElapsedTimeLessThan(const ActorBlockState& state, float a_in) const {
	return settings::fTimedBlockWindow - state.timedBlockTimer <= a_in;
}

void blockHandler::onActorDeath(RE::Actor* a_actor) {
	if (!a_actor) {
		return;
	}
	std::unique_lock<std::shared_mutex> lock(mtx_blockStates);
	_blockStates.erase(a_actor->GetFormID());
}

void blockHandler::reset() {
	std::unique_lock<std::shared_mutex> lock(mtx_blockStates);
	_blockStates.clear();
}

// Block Commitment

void BlockCommitment::StartTimer(RE::Actor* a_actor) {
	if (!a_actor) {
		return;
	}
	std::unique_lock<std::shared_mutex> lock(mtx_commitmentStates);
	auto& state = _commitmentStates[a_actor->GetFormID()];
	state.timer = settings::fBlockCommitmentTime;
	state.stopPending = false;

	// Activate separate block commitment handler loop
	ValhallaCombat::GetSingleton()->activateUpdate(ValhallaCombat::HANDLER::blockCommitment);
}

void BlockCommitment::update() {
	if (!settings::bBlockCommitmentToggle) {
		ValhallaCombat::GetSingleton()->deactivateUpdate(ValhallaCombat::HANDLER::blockCommitment);
		return;
	}

	float a_delta = *RE::Offset::g_deltaTime;
	std::vector<RE::FormID> actorsToStop;
	bool isEmpty = false;

	// Phase 1: Lock and modify map
	{
		std::unique_lock<std::shared_mutex> lock(mtx_commitmentStates);
		for (auto it = _commitmentStates.begin(); it != _commitmentStates.end(); ) {
			auto& state = it->second;
			state.timer -= a_delta;

			if (state.timer <= 0.0f) {
				if (state.stopPending) {
					actorsToStop.push_back(it->first);
				}
				it = _commitmentStates.erase(it);
			}
			else {
				++it;
			}
		}
		isEmpty = _commitmentStates.empty();
	}
	// Lock released

	// Phase 2: Engine Calls Outside Lock (Deadlock Free)
	for (RE::FormID formID : actorsToStop) {
		if (auto actor = RE::TESForm::LookupByID<RE::Actor>(formID)) {
			if (Utils::Actor::isBlocking(actor)) {
				bypassHook = true;
				actor->NotifyAnimationGraph("blockStop");
				bypassHook = false;
			}
		}
	}

	// Deactivate block commitment handler loop when idle
	if (isEmpty) {
		ValhallaCombat::GetSingleton()->deactivateUpdate(ValhallaCombat::HANDLER::blockCommitment);
	}
}

bool BlockCommitment::ShouldSinkEvent(RE::Actor* a_actor, const RE::BSFixedString& a_eventName)
{
	if (bypassHook || !a_actor) {
		return false;
	}

	if (a_eventName == "blockStop") {
		std::unique_lock<std::shared_mutex> lock(mtx_commitmentStates);
		auto it = _commitmentStates.find(a_actor->GetFormID());
		if (it != _commitmentStates.end() && it->second.timer > 0.0f) {
			it->second.stopPending = true;
			return true;
		}
	}

	return false;
}

void BlockCommitment::onActorDeath(RE::Actor* a_actor) {
	if (!a_actor) {
		return;
	}
	std::unique_lock<std::shared_mutex> lock(mtx_commitmentStates);
	_commitmentStates.erase(a_actor->GetFormID());
}

void BlockCommitment::reset() {
	std::unique_lock<std::shared_mutex> lock(mtx_commitmentStates);
	_commitmentStates.clear();
}

// Projectile & Physical Blocking Functions

bool blockHandler::tryParryProjectile_Spell(RE::Actor* a_blocker, RE::Projectile* a_projectile, RE::hkpCollidable* a_projectile_collidable) {
	onSuccessfulTimedBlock(a_blocker);
	if (settings::bEnableBlockExp && a_blocker->IsPlayerRef()) {
		if (auto pc = RE::PlayerCharacter::GetSingleton()) {
			pc->AddSkillExperience(RE::ActorValue::kBlock, settings::fTimedBlockProjectileExp, nullptr, 0);
		}
	}
	float cost = a_projectile->GetProjectileRuntimeData().spell->CalculateMagickaCost(a_blocker);
	if (Utils::Actor::tryDamageAv(a_blocker, RE::ActorValue::kMagicka, cost)) {
		deflectProjectile(a_blocker, a_projectile, a_projectile_collidable);
	}
	else {
		destroyProjectile(a_projectile);
	}
	if (Utils::Actor::isBlocking(a_blocker)) {
		a_blocker->NotifyAnimationGraph("BlockHitStart");
	}
	playBlockEffects(a_blocker, nullptr, blockHandler::blockType::timed);

	ValhallaUtils::SendModEvent("VC_SpellTimedBlockEvent", a_blocker);

	if (settings::bApplySpellOnSuccess) {
		Utils::Actor::safeApplySpell(data::timedProjectileBlockSpell, a_blocker);
		if (settings::bAdvancedLogging) {
			logger::info("Timed Projectile Block Spell applied successfully");
		}
	}
	if (settings::bAdvancedLogging) {
		logger::info("Successful Spell Timed Block");
	}
	return true;
}

bool blockHandler::tryParryProjectile_Arrow(RE::Actor* a_blocker, RE::Projectile* a_projectile, RE::hkpCollidable* a_projectile_collidable) {
	onSuccessfulTimedBlock(a_blocker);
	if (settings::bEnableBlockExp && a_blocker->IsPlayerRef()) {
		if (auto pc = RE::PlayerCharacter::GetSingleton()) {
			pc->AddSkillExperience(RE::ActorValue::kBlock, settings::fTimedBlockProjectileExp, nullptr, 0);
		}
	}
	auto launcher = a_projectile->GetProjectileRuntimeData().weaponSource;
	auto ammo = a_projectile->GetProjectileRuntimeData().ammoSource;
	float cost = 0;
	if (launcher) {
		cost += launcher->GetAttackDamage();
	}
	if (ammo) {
		cost += ammo->data.damage;
	}

	inlineUtils::offsetRealDamage(cost, nullptr, a_blocker);
	if (Utils::Actor::tryDamageAv(a_blocker, RE::ActorValue::kMagicka, cost)) {
		deflectProjectile(a_blocker, a_projectile, a_projectile_collidable);
	}
	else {
		destroyProjectile(a_projectile);
	}
	if (Utils::Actor::isBlocking(a_blocker)) {
		a_blocker->NotifyAnimationGraph("BlockHitStart");
	}
	playBlockEffects(a_blocker, nullptr, blockHandler::blockType::timed);

	ValhallaUtils::SendModEvent("VC_ArrowTimedBlockEvent", a_blocker);

	if (settings::bApplySpellOnSuccess) {
		Utils::Actor::safeApplySpell(data::timedProjectileBlockSpell, a_blocker);
		if (settings::bAdvancedLogging) {
			logger::info("Timed Projectile Block Spell applied successfully");
		}
	}
	if (settings::bAdvancedLogging) {
		logger::info("Successful Arrow Timed Block");
	}
	return true;
}

bool blockHandler::tryBlockProjectile_Spell(RE::Actor* a_blocker, RE::Projectile* a_projectile) {
	auto cost = a_projectile->GetProjectileRuntimeData().spell->CalculateMagickaCost(a_blocker);
	if (Utils::Actor::tryDamageAv(a_blocker, RE::ActorValue::kMagicka, cost)) {
		if (settings::bEnableBlockExp && a_blocker->IsPlayerRef()) {
			if (auto pc = RE::PlayerCharacter::GetSingleton()) {
				pc->AddSkillExperience(RE::ActorValue::kBlock, settings::fBlockProjectileExp, nullptr, 0);
			}
		}
		if (Utils::Actor::isBlocking(a_blocker)) {
			a_blocker->NotifyAnimationGraph("BlockHitStart");
		}
		destroyProjectile(a_projectile);

		playBlockEffects(a_blocker, nullptr, blockHandler::blockType::timed);

		ValhallaUtils::SendModEvent("VC_SpellBlockEvent", a_blocker);
		return true;
	}
	return false;
}

bool blockHandler::tryBlockProjectile_Arrow(RE::Actor* a_blocker, RE::Projectile* a_projectile) {
	auto launcher = a_projectile->GetProjectileRuntimeData().weaponSource;
	auto ammo = a_projectile->GetProjectileRuntimeData().ammoSource;
	float cost = 0;
	if (launcher) {
		cost += launcher->GetAttackDamage();
	}
	if (ammo) {
		cost += ammo->data.damage;
	}
	inlineUtils::offsetRealDamage(cost, nullptr, a_blocker);
	if (Utils::Actor::tryDamageAv(a_blocker, RE::ActorValue::kMagicka, cost)) {
		if (settings::bEnableBlockExp && a_blocker->IsPlayerRef()) {
			if (auto pc = RE::PlayerCharacter::GetSingleton()) {
				pc->AddSkillExperience(RE::ActorValue::kBlock, settings::fBlockProjectileExp, nullptr, 0);
			}
		}
		if (Utils::Actor::isBlocking(a_blocker)) {
			a_blocker->NotifyAnimationGraph("BlockHitStart");
		}
		destroyProjectile(a_projectile);

		playBlockEffects(a_blocker, nullptr, blockHandler::blockType::timed);

		ValhallaUtils::SendModEvent("VC_ArrowBlockEvent", a_blocker);
		return true;
	}
	return false;
}

bool blockHandler::processProjectileBlock(RE::Actor* a_blocker, RE::Projectile* a_projectile, RE::hkpCollidable* a_projectile_collidable) {
	if (!a_blocker || !a_projectile) {
		return false;
	}

	if (Utils::Actor::isInBlockAngle(a_blocker, a_projectile) && Utils::Actor::isBlocking(a_blocker)) {
		bool isSpell = a_projectile->GetProjectileRuntimeData().spell != nullptr;
		bool hasShield = Utils::Actor::isEquippedShield(a_blocker);
		bool blockAllowed = false;

		if (!hasShield) { // Weapon block
			if (isSpell) {
				blockAllowed = settings::bWeaponBlockSpell && perks::canWeaponBlockSpell(a_blocker);
			}
			else {
				blockAllowed = settings::bWeaponBlockArrow && perks::canWeaponBlockArrow(a_blocker);
			}
		}
		else { // Shield block
			if (isSpell) {
				blockAllowed = settings::bShieldBlockSpell && perks::canShieldBlockSpell(a_blocker);
			}
			else {
				blockAllowed = settings::bShieldBlockArrow && perks::canShieldBlockArrow(a_blocker);
			}
		}

		if (!blockAllowed) {
			return false;
		}

		bool timedBlockProjectileAllowed = false;
		if (a_blocker->IsPlayerRef()) {
			timedBlockProjectileAllowed = settings::bTimedBlockProjectileToggle && perks::canTimedProjectileBlock(a_blocker);
		}
		else {
			timedBlockProjectileAllowed = settings::bTimedBlockProjectileToggle && settings::bTimedBlockProjectileToggle_NPC
				&& perks::canTimedProjectileBlock(a_blocker);
		}

		if (timedBlockProjectileAllowed && getIsTimedBlocking(a_blocker)) {
			if (isSpell) {
				return tryParryProjectile_Spell(a_blocker, a_projectile, a_projectile_collidable);
			}
			else {
				return tryParryProjectile_Arrow(a_blocker, a_projectile, a_projectile_collidable);
			}
		}

		bool blockProjectileAllowed = false;
		if (a_blocker->IsPlayerRef()) {
			blockProjectileAllowed = settings::bBlockProjectileToggle && perks::canProjectileBlock(a_blocker);
		}
		else {
			blockProjectileAllowed = settings::bBlockProjectileToggle && settings::bBlockProjectileToggle_NPC
				&& perks::canProjectileBlock(a_blocker);
		}

		if (blockProjectileAllowed) {
			if (isSpell) {
				return tryBlockProjectile_Spell(a_blocker, a_projectile);
			}
			else {
				return tryBlockProjectile_Arrow(a_blocker, a_projectile);
			}
		}
	}
	return false;
}

bool blockHandler::processMeleeTimedBlock(RE::Actor* a_blocker, RE::Actor* a_attacker) {
	if (!a_blocker || !a_attacker) {
		return false;
	}

	if (Utils::Actor::isPowerBashing(a_attacker) || Utils::Actor::isBashing(a_attacker)) {
		return false;
	}

	bool bIsUnblockable = false;
	if (settings::bUnblockableCompatibility && a_attacker->GetGraphVariableBool("isUnblockableHit", bIsUnblockable) && bIsUnblockable) {
		return false;
	}

	if (Utils::Actor::isUnblockable(a_attacker)) {
		return false;
	}

	if (!Utils::Actor::isBlocking(a_blocker) || !Utils::Actor::isInBlockAngle(a_blocker, a_attacker)) {
		return false;
	}

	if (!getIsTimedBlocking(a_blocker)){
		return false;
	}

	bool isPerfectblock = getIsPerfectBlocking(a_blocker) && perks::canPerfectBlock(a_blocker);

	onSuccessfulTimedBlock(a_blocker);

	if (settings::bEnableBlockExp && a_blocker->IsPlayerRef()) {
		if (auto pc = RE::PlayerCharacter::GetSingleton()) {
			pc->AddSkillExperience(RE::ActorValue::kBlock, settings::fTimedBlockMeleeExp, nullptr, 0);
		}
	}

	float reflectedDamage = 0;
	auto blockerWeapon = Utils::Actor::getWieldingWeapon(a_blocker);
	reflectedDamage = blockerWeapon ? blockerWeapon->GetAttackDamage() : a_blocker->CalcUnarmedDamage();

	inlineUtils::offsetRealDamage(reflectedDamage, a_blocker, a_attacker);
	float stunDmg = reflectedDamage;
	stunHandler::GetSingleton()->processStunDamage(stunHandler::STUNSOURCE::timedBlock, nullptr, a_blocker, a_attacker, stunDmg);
	playBlockEffects(a_blocker, a_attacker, isPerfectblock ? blockType::perfect : blockType::timed);

	if (isPerfectblock) {

		// Tackle Initiation on successful Perfect Block
		bool tackleAllowed = false;
		if (a_blocker->IsPlayerRef()) {
			tackleAllowed = settings::bTackleToggle && perks::canTackle(a_blocker);
		}
		else {
			tackleAllowed = settings::bTackleToggle && settings::bTackleToggle_NPC && perks::canTackle(a_blocker);
		}

		if (tackleAllowed) {
			std::unique_lock<std::shared_mutex> lock(mtx_blockStates);
			auto it = _blockStates.find(a_blocker->GetFormID());
			if (it != _blockStates.end()) {
				auto& state = it->second;
				if (!state.isTackleCooldown && !state.isTackling) {
					state.isTackling = true;
					state.tackleTimer = settings::fTackleWindow;
					ValhallaCombat::GetSingleton()->activateUpdate(ValhallaCombat::HANDLER::blockHandler);
					//logger::info("Tackle: Start Tackle");
				}
			}
		}

		reactionHandler::triggerRecoil(a_attacker);

		ValhallaUtils::SendModEvent("VC_PerfectBlockEvent", a_blocker, reflectedDamage);

		if (settings::bPerfectBlockStaminaRegen && perks::canPerfectBlockStamina(a_blocker)) {
			if (!debuffHandler::GetSingleton()->isInDebuff(a_blocker)) {
				restoreStamina(a_blocker);
			}
			else if (settings::bPerfectBlockStaminaRegenExhaustion && perks::canPerfectBlockStamina(a_blocker)) {
				debuffHandler::GetSingleton()->stopDebuff(a_blocker);
				restoreStamina(a_blocker);
			}
		}
		if (settings::bApplySpellOnSuccess) {
			Utils::Actor::safeApplySpell(data::perfectBlockSpell, a_blocker);
			if (settings::bAdvancedLogging) {
				logger::info("Perfect Block Spell applied successfully");
			}
		}
		if (settings::bAdvancedLogging) {
			logger::info("Successful Perfect Block");
		}
	}
	else {
		RE::HitData hitData;
		RE::InventoryEntryData* attackerWeapon = a_attacker->GetAttackingWeapon();
		hitData.Populate(a_attacker, a_blocker, attackerWeapon);
		float staminaDamageMult = getBlockStaminaCostMult(a_blocker, a_attacker, hitData.flags);
		Utils::Actor::damageav(a_blocker, RE::ActorValue::kStamina,
			hitData.totalDamage * staminaDamageMult * settings::fTimedBlockStaminaCostMult);

		ValhallaUtils::SendModEvent("VC_TimedBlockEvent", a_blocker, reflectedDamage);

		if (settings::bApplySpellOnSuccess) {
			Utils::Actor::safeApplySpell(data::timedBlockSpell, a_blocker);
			if (settings::bAdvancedLogging) {
				logger::info("Timed Block Spell applied successfully");
			}
		}
		if (settings::bAdvancedLogging) {
			logger::info("Successful Timed Block");
		}
	}

	if (EldenCounterCompatibility::on) {
		EldenCounterCompatibility::triggerCounter(a_blocker);
	}

	a_blocker->NotifyAnimationGraph("BlockHitStart");
	return true;
}

bool blockHandler::processMeleeTackle(RE::Actor* a_tackler, RE::Actor* a_attacker) {
	if (!a_tackler || !a_attacker) {
		return false;
	}

	if (Utils::Actor::isPowerBashing(a_attacker) || Utils::Actor::isBashing(a_attacker)) {
		return false;
	}

	bool bIsUnblockable = false;
	if (settings::bUnblockableCompatibility && a_attacker->GetGraphVariableBool("isUnblockableHit", bIsUnblockable) && bIsUnblockable) {
		return false;
	}

	if (Utils::Actor::isUnblockable(a_attacker)) {
		return false;
	}

	if (!getIsTackling(a_tackler)) {
		return false;
	}

	reactionHandler::triggerRecoil(a_attacker);

	playBlockEffects(a_tackler, a_attacker, blockHandler::tackle);

	ValhallaUtils::SendModEvent("VC_TackleEvent", a_tackler);

	if (settings::bApplySpellOnSuccess) {
		Utils::Actor::safeApplySpell(data::tackleSpell, a_tackler);
	}
	return true;
}

// Block Helper Functions

void blockHandler::deflectProjectile(RE::Actor* a_deflector, RE::Projectile* a_projectile, RE::hkpCollidable* a_projectile_collidable) {
	Utils::Projectile::deflect(a_deflector, a_projectile, a_projectile_collidable);
}

void blockHandler::destroyProjectile(RE::Projectile* a_projectile) {
	RE::Offset::destroyProjectile(a_projectile);
}

void blockHandler::processPhysicalBlock(RE::Actor* blocker, RE::Actor* aggressor, REX::EnumSet<RE::HitData::Flag, std::uint32_t> a_hitFlag, RE::HitData& hitData) {
	if (settings::bBlockStaminaToggle) {
		processStaminaBlock(blocker, aggressor, a_hitFlag, hitData);
	}
}

float blockHandler::getBlockStaminaCostMult(RE::Actor* blocker, RE::Actor* aggressor, REX::EnumSet<RE::HitData::Flag, std::uint32_t> a_hitFlag) {
	if (a_hitFlag.any(HITFLAG::kBlockWithWeapon)) {
		if (blocker->IsPlayerRef()) {
			return settings::fBckWpnStaminaMult_PC_Block_NPC;
		}
		else {
			if (aggressor->IsPlayerRef()) {
				return settings::fBckWpnStaminaMult_NPC_Block_PC;
			}
			else {
				return settings::fBckWpnStaminaMult_NPC_Block_NPC;
			}
		}
	}
	else {
		if (blocker->IsPlayerRef()) {
			return settings::fBckShdStaminaMult_PC_Block_NPC;
		}
		else {
			if (aggressor->IsPlayerRef()) {
				return settings::fBckShdStaminaMult_NPC_Block_PC;
			}
			else {
				return settings::fBckShdStaminaMult_NPC_Block_NPC;
			}
		}
	}
}

void blockHandler::processStaminaBlock(RE::Actor* a_blocker, RE::Actor* a_aggressor, REX::EnumSet<RE::HitData::Flag, std::uint32_t> a_hitFlag, RE::HitData& a_hitData) {
	float staminaDamage = a_hitData.totalDamage;
	inlineUtils::offsetRealDamage(staminaDamage, a_aggressor, a_blocker);
	float staminaDamageMult = getBlockStaminaCostMult(a_blocker, a_aggressor, a_hitFlag);
	staminaDamage *= staminaDamageMult;
	
	if (staminaDamage <= 0.0f) {
		return;
	}

	float targetStamina = a_blocker->AsActorValueOwner()->GetActorValue(RE::ActorValue::kStamina);

	if (targetStamina < staminaDamage) {
		if (settings::bGuardBreak) {
			reactionHandler::applyStagger(a_aggressor, a_blocker, Utils::Actor::isPowerAttacking(a_aggressor) ? reactionHandler::kLargest : reactionHandler::kMedium);
		}
		a_hitData.totalDamage = (staminaDamage - (targetStamina / staminaDamageMult)) * (a_hitData.totalDamage) / staminaDamage;
		Utils::Actor::damageav(a_blocker, RE::ActorValue::kStamina, targetStamina);
		debuffHandler::GetSingleton()->initStaminaDebuff(a_blocker);
	}
	else {
		a_hitData.totalDamage = 0;
		Utils::Actor::damageav(a_blocker, RE::ActorValue::kStamina, staminaDamage);
	}
}

void blockHandler::restoreStamina(RE::Actor* a_blocker) {
	if (!a_blocker) {
		return;
	}
	if (a_blocker->IsPlayerRef() && data::incrementalFatigueCap) {
		auto* avOwner = a_blocker->AsActorValueOwner();
		float current = avOwner->GetActorValue(RE::ActorValue::kStamina);
		float effectiveMax = avOwner->GetPermanentActorValue(RE::ActorValue::kStamina);
		float cap = std::clamp(data::incrementalFatigueCap->value, 0.05f, 1.0f);
		float target = effectiveMax * cap;

		if (current < target) {
			Utils::Actor::restoreav(a_blocker, RE::ActorValue::kStamina, target - current);
		}
	}
	else {
		Utils::Actor::refillActorValue(a_blocker, RE::ActorValue::kStamina);
	}
}

PRECISION_API::PreHitCallbackReturn blockHandler::precisionPrehitCallbackFunc(const PRECISION_API::PrecisionHitData& a_precisionHitData) {
	PRECISION_API::PreHitCallbackReturn ret;

	auto attacker = a_precisionHitData.attacker;
	auto target = a_precisionHitData.target;
	if (!attacker || !target || target->GetFormType() != RE::FormType::ActorCharacter) {
		return ret;
	}
	RE::Actor* targetActor = target->As<RE::Actor>();

	if (SpecialAttackHandler::GetSingleton()->isInvulnerable(targetActor)) {
		ret.bIgnoreHit = true;
		return ret;
	}

	if (SpecialAttackHandler::GetSingleton()->processIncomingHit(targetActor, attacker)) {
		ret.bIgnoreHit = true;
		return ret;
	}

	const bool parryEnabledForAttacker = settings::bParryToggle && (attacker->IsPlayerRef() || settings::bParryToggle_NPC);
	const bool parryEnabledForDefender = settings::bParryToggle && (targetActor->IsPlayerRef() || settings::bParryToggle_NPC);

	if (parryEnabledForAttacker && Utils::Actor::isBashing(attacker)) {
		ret.bIgnoreHit = true;
		return ret;
	}

	if (parryEnabledForDefender && Utils::Actor::isBashing(targetActor)) {
		if (parryHandler::GetSingleton()->processMeleeParry(attacker, targetActor)) {
			ret.bIgnoreHit = true;
			return ret;
		}
	}

	if (settings::bTimedBlockToggle && blockHandler::GetSingleton()->processMeleeTimedBlock(targetActor, attacker)) {
		ret.bIgnoreHit = true;
		return ret;
	}

	if (settings::bTackleToggle && blockHandler::GetSingleton()->processMeleeTackle(targetActor, attacker)) {
		ret.bIgnoreHit = true;
		return ret;
	}

	return ret;
}

// Block Effects

void blockHandler::playBlockSFX(RE::Actor* blocker, blockType blockType, bool blockedWithWeapon) {
	if (blockedWithWeapon) {
		switch (blockType) {
		case blockType::guardBreaking: ValhallaUtils::playSound(blocker, data::soundParryWeapon_gb, settings::fTimedBlockSFXVolume); break;
		case blockType::perfect: ValhallaUtils::playSound(blocker, data::soundParryWeapon_perfect, settings::fTimedBlockSFXVolume); break;
		case blockType::timed: ValhallaUtils::playSound(blocker, data::soundParryWeapon, settings::fTimedBlockSFXVolume); break;
		case blockType::tackle: ValhallaUtils::playSound(blocker, data::soundParryWeapon, settings::fTimedBlockSFXVolume); break;
		}
	}
	else {
		switch (blockType) {
		case blockType::guardBreaking: ValhallaUtils::playSound(blocker, data::soundParryShield_gb, settings::fTimedBlockSFXVolume); break;
		case blockType::perfect: ValhallaUtils::playSound(blocker, data::soundParryShield_perfect, settings::fTimedBlockSFXVolume); break;
		case blockType::timed: ValhallaUtils::playSound(blocker, data::soundParryShield, settings::fTimedBlockSFXVolume); break;
		case blockType::tackle: ValhallaUtils::playSound(blocker, data::soundParryWeapon, settings::fTimedBlockSFXVolume); break;
		}
	}
}

void blockHandler::playBlockVFX(RE::Actor* blocker, blockType blockType, bool blockedWithWeapon) {
	auto blockFXNode = RE::Offset::PlaceAtMe(blocker, data::BlockFX, 1, false, false);
	std::string node = blockedWithWeapon ? "WEAPON" : "SHIELD";
	blockFXNode->MoveToNode(blocker, node);
	RE::Offset::PlaceAtMe(blockFXNode, data::BlockSpark, 1, false, false);
	RE::Offset::PlaceAtMe(blockFXNode, data::BlockSparkFlare, 1, false, false);
	if (blockType == blockType::guardBreaking || blockType == blockType::perfect) {
		RE::Offset::PlaceAtMe(blockFXNode, data::BlockSparkRing, 1, false, false);
	}
	blockFXNode->SetDelete(true);
}

void blockHandler::playBlockScreenShake(RE::Actor* blocker, blockType blockType) {
	(void)blocker;
	switch (blockType) {
	case blockType::guardBreaking: RE::Offset::shakeCamera(1.7, RE::PlayerCharacter::GetSingleton()->GetPosition(), 0.8f); break;
	case blockType::timed: RE::Offset::shakeCamera(1.5, RE::PlayerCharacter::GetSingleton()->GetPosition(), 0.3f); break;
	case blockType::perfect: RE::Offset::shakeCamera(1.5, RE::PlayerCharacter::GetSingleton()->GetPosition(), 0.4f); break;
	}
}

void blockHandler::playBlockSlowTime(blockType blockType) {
	float slowDuration = 0.0f;
	float slowPercentage = settings::fTimedBlockSlowTimePercentage;
	switch (blockType) {
	case blockType::timed:
	case blockType::tackle: slowDuration = settings::fTimedBlockSlowTime_Timed; break;
	case blockType::guardBreaking:
	case blockType::perfect: slowDuration = settings::fTimedBlockSlowTime_Perfect; break;
	}
	if (slowDuration > 0.0f) {
		inlineUtils::slowTime(slowDuration, slowPercentage);
	}
}

void blockHandler::playBlockEffects(RE::Actor* blocker, RE::Actor* attacker, blockType blockType) {
	bool shieldEquipped = RE::Offset::getEquippedShield(blocker);
	if (settings::bTimeBlockSFX) {
		playBlockSFX(blocker, blockType, !shieldEquipped);
	}
	if (settings::bTimedBlockScreenShake && ((blocker && blocker->IsPlayerRef()) || (attacker && attacker->IsPlayerRef()))) {
		playBlockScreenShake(blocker, blockType);
	}
	if (settings::bTimedBlockSlowTime && blocker && blocker->IsPlayerRef()) {
		playBlockSlowTime(blockType);
	}
	switch (settings::uTimedBlockSparkType) {
	case 0:
		default:
		break;
	case 1:
		playBlockVFX(blockType == blockType::tackle ? attacker : blocker, blockType, blockType == blockType::tackle || !shieldEquipped);
		break;
	case 2:
		MaxsuBlockSpark::blockSpark::playPerfectBlockSpark(blockType == blockType::tackle ? attacker : blocker);
		break;
	}
}

// Elden Counter Compatibility

void blockHandler::EldenCounterCompatibility::attemptInit() {
	logger::info("Initializing Elden Counter compatibility");
	ec_triggerSpell = RE::TESDataHandler::GetSingleton()->LookupForm<RE::SpellItem>(0x801, "EldenCounter.esp");
	if (!ec_triggerSpell) {
		logger::info("EldenCounter.esp not found");
		on = false;
	}
	else {
		logger::info("found Elden Counter plugin; compatibility enabled.");
		EldenCounterCompatibility::readSettings();
		on = true;
	}
}

inline void blockHandler::EldenCounterCompatibility::triggerCounter(RE::Actor* a_actor)
{
	if (!a_actor) {
		return;
	}
	if (a_actor->HasSpell(ec_triggerSpell)) {
		return;
	}
	a_actor->AddSpell(ec_triggerSpell);
	std::jthread removeSpellThread(async_removeECTriggerSpell, a_actor);
	removeSpellThread.detach();
}

void blockHandler::EldenCounterCompatibility::async_removeECTriggerSpell(RE::Actor* a_actor)
{
	if (!a_actor) {
		return;
	}
	RE::FormID actorID = a_actor->GetFormID();

	std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(ec_Time * 1000)));

	SKSE::GetTaskInterface()->AddTask([actorID]() {
		if (auto actor = RE::TESForm::LookupByID<RE::Actor>(actorID)) {
			Utils::Actor::safeRemoveSpell(ec_triggerSpell, actor);
		}
	});
}

void blockHandler::EldenCounterCompatibility::readSettings() {
	logger::info("EldenCounter compatibility: reading settings...");
	CSimpleIniA ini;
#define SETTINGFILE_PATH "Data\\SKSE\\Plugins\\EldenCounter.ini"
	ini.LoadFile(SETTINGFILE_PATH);
	ec_Time = std::stof(ini.GetValue("General", "Time"));
	logger::info("EldenCounter compatibilty: settings loaded.");
}
