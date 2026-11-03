#pragma once
#include "lib/robin_hood.h"
#include <shared_mutex>

/*Class handling actor debuffs and corresponding UI alerts.*/
class debuffHandler
{

public:
	/*Set of all actors currently in debuff*/
	robin_hood::unordered_set<RE::FormID> actorInDebuff;
	mutable std::shared_mutex mtx_actorInDebuff;

	static debuffHandler* GetSingleton()
	{
		static debuffHandler singleton;
		return  std::addressof(singleton);
	}

	void applyMovementRestrictions(RE::Actor* a_actor);
	void clearMovementRestrictions(RE::Actor* a_actor);
	bool processExhaustionAnimEvent(RE::Actor* a_actor, const RE::BSFixedString& a_eventName, bool& a_outShouldProcess);
	bool processExhaustionAnimEventNPC(RE::Actor* a_actor, const RE::BSFixedString& a_eventName, bool& a_outShouldProcess);
	bool suppressExhaustionAnimEvent(RE::Actor* a_actor, const RE::BSFixedString& a_eventName, bool& a_outShouldProcess);
	void startGuardBreakLockout(RE::Actor* a_actor);
	bool ShouldSinkEvent(RE::Actor* a_actor, const RE::BSFixedString& a_eventName);

	/*called along with the main update hook.*/
	void update();

	/*@return whether this actor is in stamina debuff.*/
	bool isInDebuff(RE::FormID a_formID);
	bool isInDebuff(RE::Actor* a_actor);

	bool isGuardBreakLockedOut(RE::FormID a_formID);
	bool isGuardBreakLockedOut(RE::Actor* a_actor);

	void initStaminaDebuff(RE::Actor* a_actor);
	void revertDebuffUI(RE::Actor* a_actor);
	void initDebuffUI(RE::Actor* a_actor);

	/*Quickly stop stamina debuff without using the iterator.
	a convenience method for outside class.*/
	void stopDebuff(RE::FormID a_formID);
	void stopDebuff(RE::Actor* a_actor);

	void onBlockStart(RE::Actor* a_actor);
	void onBlockStop(RE::Actor* a_actor);

	/*Clean up debuff states when an actor dies.*/
	void onActorDeath(RE::Actor* a_actor);

	/*Clears all tracked debuffs and reverts UI/Spells. Called on game load.*/
	void reset();

private:

	static inline bool pcStaminaMeterFlash_b = false;

	robin_hood::unordered_map<RE::FormID, float> guardBreakLockouts;
	mutable std::shared_mutex mtx_guardBreakLockouts;

	robin_hood::unordered_map<RE::FormID, float> exhaustionBlockTimers;
	mutable std::shared_mutex mtx_exhaustionBlockTimers;

};