#pragma once
/*Convenience clas to deal with stamina operations.*/
class staminaHandler
{
public:
	static void checkStamina(RE::Actor* a_actor);
	static float getLightAttackCost(RE::Actor* a_actor);
	static float getPowerAttackCost(RE::Actor* a_actor);
	static float getLightBashCost(RE::Actor* a_actor);
	static float getPowerBashCost(RE::Actor* a_actor);
	static float getPerkStaminaCostMult(RE::Actor* a_actor);
	static float getGraphStaminaMult(RE::Actor* a_actor);
	static float getSpecialAttackStaminaMult(RE::Actor* a_actor);
	static void staminaLightAttackMiss(RE::Actor* actor);
	static void staminaLightAttackReward(RE::Actor* a_actor, uint32_t hitIndex);
	static void staminaLightAttackHitFrameCost(RE::Actor* a_actor, uint32_t hitIndex);
	static void staminaPowerAttackHitFrameCost(RE::Actor* a_actor, uint32_t hitIndex);
	static void staminaLightBashHitFrameCost(RE::Actor* a_actor, uint32_t hitIndex);
	static void staminaPowerBashHitFrameCost(RE::Actor* a_actor, uint32_t hitIndex);

private:
	static float calculateFinalCost(RE::Actor* a_actor, float a_baseCost, bool a_usePerkMult, uint32_t a_hitIndex);
};
