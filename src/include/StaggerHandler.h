#pragma once
#include <map>
#include <string>

enum StaggerLevel : std::uint32_t
{
	kNone = 0,
	kSmall = 1,
	kMedium = 2,
	kLarge = 3,
	kLargest = 4,
};

class StaggerHandler
{
public:
	static void ProcessWeaponStagger(RE::Actor* a_aggressor, RE::Actor* a_target, const RE::HitData* a_hitData);
	static void ProcessMagicStagger(RE::Actor* a_target, float a_staggerMult, RE::Actor* a_aggressor);

private:
	static StaggerLevel GetStaggerLevel(const float& a_DamagePercent);

	StaggerHandler() = delete;
	~StaggerHandler() = delete;
};

class ImmuneLevelCalculator
{
public:
	static StaggerLevel GetTotalImmuneLevel(RE::Actor* a_target);
	static bool HasActiveEffectWithKeyword(RE::MagicTarget* a_target, RE::BGSKeyword* a_keyword);

private:
	ImmuneLevelCalculator() = delete;
	~ImmuneLevelCalculator() = delete;
};