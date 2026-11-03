#pragma once
#include "lib/PrecisionAPI.h"
using HITFLAG = RE::HitData::Flag;
class hitProcessor {
public:
	static hitProcessor* GetSingleton()
	{
		static hitProcessor singleton;
		return  std::addressof(singleton);
	}

	/*Process a valid hit passed from hook. Offset the raw damage based on game setting.*/
	void processHit(RE::Actor* aggressor, RE::Actor* victim, RE::HitData& hitData);

	/*Precision's post-hit callback: hits it handles go through the same processing.*/
	static void precisionPosthitCallbackFunc(const PRECISION_API::PrecisionHitData& a_precisionHitData, const RE::HitData& a_hitData);

	/*Hits can reach us from Precision and from the vanilla hit function. True the first time a pair is seen within 50 ms, so one blow is never counted twice.*/
	static bool claimHit(RE::Actor* a_aggressor, RE::Actor* a_victim);


};
