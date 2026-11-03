#pragma once
#include "lib/PrecisionAPI.h"
#include "lib/robin_hood.h"
#include <shared_mutex>

class blockHandler {
public:
	static blockHandler* GetSingleton()
	{
		static blockHandler singleton;
		return std::addressof(singleton);
	}

	void update();
	void reset();

	enum blockType {
		regular = 1,
		timed,
		perfect,
		guardBreaking,
		tackle,
	};

	enum blockWindowPenaltyLevel {
		none = 0,
		light,  // 70% of regular timing
		medium, // 50% of regular timing
		heavy   // 20% of regular timing
	};

	struct ActorBlockState {
		blockWindowPenaltyLevel blockWindowPenalty = blockWindowPenaltyLevel::none;

		float timedBlockTimer = 0.0f;
		float timedBlockCooldownTimer = 0.0f;

		bool isTimedBlocking = false;
		bool isBlockingCooldown = false;
		bool isTimedBlockSuccess = false;

		bool isTackling = false;
		bool isTackleCooldown = false;
		float tackleTimer = 0.0f;
		float tackleCooldownTimer = 0.0f;
	};

	bool getIsTimedBlocking(RE::Actor* a_actor);
	bool getIsPerfectBlocking(RE::Actor* a_actor);
	bool getIsTackling(RE::Actor* a_actor);

	void onBlockStart(RE::Actor* a_actor);
	void onBlockStop(RE::Actor* a_actor);
	void onSuccessfulTimedBlock(RE::Actor* a_actor);
	void onActorDeath(RE::Actor* a_actor);

private:

	void onTimedBlockEnd(ActorBlockState& state);
	void pcBlockWindowPenalty_update(blockWindowPenaltyLevel& a_penaltyLevel, bool increase);
	bool isTimedBlockElapsedTimeLessThan(const ActorBlockState& state, float a_in) const;
	void restoreStamina(RE::Actor* a_blocker);

	inline void playBlockVFX(RE::Actor* blocker, blockType blockType, bool blockedWithWeapon);
	inline void playBlockSFX(RE::Actor* blocker, blockType blockType, bool blockedWithWeapon);
	inline void playBlockScreenShake(RE::Actor* blocker, blockType blockType);
	inline void playBlockSlowTime(blockType blockType);

	void playBlockEffects(RE::Actor* blocker, RE::Actor* aggressor, blockType blockType);

	robin_hood::unordered_map<RE::FormID, ActorBlockState> _blockStates;
	std::shared_mutex mtx_blockStates;

public:
	void processPhysicalBlock(RE::Actor* blocker, RE::Actor* aggressor, REX::EnumSet<RE::HitData::Flag, std::uint32_t> hitFlags, RE::HitData& hitData);
	bool processProjectileBlock(RE::Actor* a_blocker, RE::Projectile* a_projectile, RE::hkpCollidable* a_projectile_collidable);
	bool processMeleeTimedBlock(RE::Actor* a_blocker, RE::Actor* a_attacker);
	bool processMeleeTackle(RE::Actor* a_tackler, RE::Actor* a_attacker);

	static PRECISION_API::PreHitCallbackReturn precisionPrehitCallbackFunc(const PRECISION_API::PrecisionHitData& a_precisionHitData);

private:
	void processStaminaBlock(RE::Actor* a_blocker, RE::Actor* a_aggressor, REX::EnumSet<RE::HitData::Flag, std::uint32_t> a_hitFlag, RE::HitData& a_hitData);
	float getBlockStaminaCostMult(RE::Actor* blocker, RE::Actor* aggressor, REX::EnumSet<RE::HitData::Flag, std::uint32_t> a_hitFlag);
	bool tryBlockProjectile_Arrow(RE::Actor* a_blocker, RE::Projectile* a_projectile);
	bool tryBlockProjectile_Spell(RE::Actor* a_blocker, RE::Projectile* a_projectile);
	bool tryParryProjectile_Spell(RE::Actor* a_blocker, RE::Projectile* a_projectile, RE::hkpCollidable* a_projectile_collidable);
	bool tryParryProjectile_Arrow(RE::Actor* a_blocker, RE::Projectile* a_projectile, RE::hkpCollidable* a_projectile_collidable);

	inline void deflectProjectile(RE::Actor* a_deflector, RE::Projectile* a_projectile, RE::hkpCollidable* a_projectile_collidable);
	inline void destroyProjectile(RE::Projectile* a_projectile);

public:
	class EldenCounterCompatibility
	{
	public:
		static void attemptInit();
		static void triggerCounter(RE::Actor* a_actor);
		static inline bool on = false;

	private:
		static inline RE::SpellItem* ec_triggerSpell = nullptr;
		static inline float ec_Time = 1;
		static inline void async_removeECTriggerSpell(RE::Actor* a_actor);
		static void readSettings();
	};
};

class BlockCommitment
{
public:
	static BlockCommitment* GetSingleton()
	{
		static BlockCommitment singleton;
		return std::addressof(singleton);
	}

	struct CommitmentState {
		float timer = 0.0f;
		bool stopPending = false;
	};

	void StartTimer(RE::Actor* a_actor);
	void update();
	void reset();
	bool ShouldSinkEvent(RE::Actor* a_actor, const RE::BSFixedString& a_eventName);
	void onActorDeath(RE::Actor* a_actor);

private:
	robin_hood::unordered_map<RE::FormID, CommitmentState> _commitmentStates;
	std::shared_mutex mtx_commitmentStates;
	inline static thread_local bool bypassHook = false;
};