#pragma once
#include "lib/robin_hood.h"
#include <shared_mutex>

class parryHandler
{
public:
	static parryHandler* GetSingleton()
	{
		static parryHandler singleton;
		return  std::addressof(singleton);
	}

	void update();
	void reset();

	bool processMeleeParry(RE::Actor* a_aggressor, RE::Actor* a_victim);
	bool processProjectileParry(RE::Actor* a_parrier, RE::Projectile* a_projectile, RE::hkpCollidable* a_collidable);
	void startTimingParry(RE::Actor* a_parrier);
	void finishTimingParry(RE::Actor* a_parrier);
	bool inParryState(RE::Actor* a_parrier);
	void onActorDeath(RE::Actor* a_actor);

private:
	parryHandler() = default;
	~parryHandler() = default;
	parryHandler(const parryHandler&) = delete;
	parryHandler& operator=(const parryHandler&) = delete;

	void playParrySFX(RE::Actor* parrier, bool parriedWithWeapon);
	void playParryVFX(RE::Actor* parrier, bool parriedWithWeapon);
	void playParryScreenShake();
	void playParrySlowTime();
	void playParryEffects(RE::Actor* parrier, RE::Actor* attacker);
	
	//bool inParryState(RE::Actor* a_parrier);
	bool canParry(RE::Actor* a_parrier, RE::TESObjectREFR* a_obj);

	struct ParryContext
	{
		float timer = 0.0f;
		bool successful = false;
	};

	robin_hood::unordered_map<RE::FormID, ParryContext> _parryTimer;
	std::shared_mutex mtx_parryTimer;
};
