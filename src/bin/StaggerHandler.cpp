#include "include/StaggerHandler.h"
#include "include/poiseHandler.h"
#include "include/reactionHandler.h"
#include "include/Utils.h"
#include "include/settings.h"
#include "include/data.h"

void StaggerHandler::ProcessWeaponStagger(RE::Actor* a_aggressor, RE::Actor* a_target, const RE::HitData* a_hitData)
{
	thread_local bool isProcessing = false;

	if (isProcessing) {
		logger::info("ProcessWeaponStagger: Blocked by isProcessing flag");
		return;
	}

	if (!a_aggressor || !a_target || !a_hitData) {
		logger::info("ProcessWeaponStagger: HitData, Aggressor or Target is null");
		return;
	}

	//bool isTargetStaggering = Utils::Actor::isStaggering(target);
	auto staggerProtectTime = poiseHandler::GetSingleton()->getStaggerProtect(a_target);

	if (staggerProtectTime > 0.0f /* && isTargetStaggering*/) {
		if (settings::bAdvancedLogging) {
			logger::info("ProcessWeaponStagger: Exiting due to protection, StaggerProtectTime: {}", staggerProtectTime);
		}
		return;
	}

	isProcessing = true;  // Lock the function

	auto totalPoiseHealth = poiseHandler::GetSingleton()->getTotalPoise(a_target);
	auto currentPoiseHealth = poiseHandler::GetSingleton()->getCurrentPoise(a_target);
	auto poiseDamage = poiseHandler::GetSingleton()->getWeaponPoiseDamage(a_aggressor, a_target, a_hitData);
	currentPoiseHealth -= poiseDamage;

	auto staggerLevel = GetStaggerLevel(poiseDamage / totalPoiseHealth);
	auto immuneLevel = ImmuneLevelCalculator::GetTotalImmuneLevel(a_target);

	if (settings::bAdvancedLogging) {
		logger::info("ProcessWeaponStagger: TotalPoise: {} | CurrentPoise: {} | Damage: {} | StaggerLevel: {} | ImmuneLevel: {}",
			totalPoiseHealth, currentPoiseHealth, poiseDamage, (uint32_t)staggerLevel, (uint32_t)immuneLevel);
	}
	// Case 1: Poise is broken entirely -> Trigger maximum break stagger
	if (currentPoiseHealth <= 0.0f) {
		reactionHandler::applyStagger(a_aggressor, a_target, reactionHandler::reactionType::kLargest);
		staggerProtectTime = settings::fPoise_StaggerProtectTime;
		poiseHandler::GetSingleton()->setStaggerProtect(a_target, staggerProtectTime);
		currentPoiseHealth = totalPoiseHealth;

		if (settings::facts::TrueHudAPI_PoiseControlsSpecialBar) {
			TrueHUDUtils::greyOutSpecialMeter(a_target);
		}
		if (settings::bAdvancedLogging) {
			logger::info("ProcessWeaponStagger: Poise Broken");
		}
	}
	// Case 2: Poise is not broken, target is not protected -> Evaluate thresholds
	else if (staggerProtectTime <= 0.0f) {
		if (staggerLevel != StaggerLevel::kNone && staggerLevel > immuneLevel) {
			auto rType = static_cast<reactionHandler::reactionType>(staggerLevel);
			reactionHandler::applyStagger(a_aggressor, a_target, rType);

			if (settings::bAdvancedLogging) {
				logger::info("ProcessWeaponStagger: Stagger level {} applied", (uint32_t)staggerLevel);
			}
		} else {
			if (settings::bAdvancedLogging) {
				logger::info("ProcessWeaponStagger: Thresholds not met");
			}
		}
	}

	poiseHandler::GetSingleton()->setCurrentPoise(a_target, currentPoiseHealth);
	poiseHandler::GetSingleton()->setRegenDelay(a_target, settings::fPoise_RegenDelayTime);

	isProcessing = false;  // Unlock the function

	if (settings::bAdvancedLogging) {
		logger::info("ProcessWeaponStagger: Stagger processed successfully");
	}

	if (settings::bPoise_DebugConsoleLog) {
		if (auto selectedRef = RE::Console::GetSelectedRef()) {
			if (selectedRef->formID == a_target->GetFormID()) {
				std::ostringstream logs;
				logs << "-------Weapon Stagger Result-------\n";

				if (staggerProtectTime > 0.f) {
					logs << "Largest Stagger! TotalHealth: " << totalPoiseHealth << "\n";
				} else {
					logs << "TotalHealth: " << totalPoiseHealth << "\n"
						 << "Damage: " << poiseDamage << "\n"
						 << "CurrentHealth: " << currentPoiseHealth << "\n"
						 << "StaggerLevel: " << static_cast<uint32_t>(staggerLevel) << "\n"
						 << "ImmuneLevel: " << static_cast<uint32_t>(immuneLevel) << "\n";
				}

				logs << "---------------------------------------";

				if (auto console = RE::ConsoleLog::GetSingleton()) {
					console->Print("%s", logs.str().c_str());
				}
			}
		}
	}
}

void StaggerHandler::ProcessMagicStagger(RE::Actor* a_target, float a_staggerMult, RE::Actor* a_aggressor)
{
	thread_local bool isProcessing = false;
	if (isProcessing) {
		logger::info("ProcessMagicStagger: Blocked by isProcessing flag");
		return;
	}

	if (!a_target) {
		logger::info("ProcessMagicStagger: Target is null");
		return;
	}

	if (a_staggerMult <= 0.0f) {
		logger::info("ProcessMagicStagger: StaggerMult is null");
		return;
	}

	if (a_target == a_aggressor) {
		a_aggressor = nullptr;
		if (settings::bAdvancedLogging) {
			logger::info("ProcessMagicStagger: Target = Aggressor");
		}
	}

	//bool isTargetStaggering = Utils::Actor::isStaggering(a_target);
	auto staggerProtectTime = poiseHandler::GetSingleton()->getStaggerProtect(a_target);

	if (staggerProtectTime > 0.0f /*&& isTargetStaggering*/) {
		if (settings::bAdvancedLogging) {
			logger::info("ProcessMagicStagger: Exiting due to protection, StaggerProtectTime: {}", staggerProtectTime);
		}
		return;
	}

	isProcessing = true; // Lock the function

	auto totalPoiseHealth = poiseHandler::GetSingleton()->getTotalPoise(a_target);
	auto currentPoiseHealth = poiseHandler::GetSingleton()->getCurrentPoise(a_target);
	auto poiseDamage = poiseHandler::GetSingleton()->getMagicPoiseDamage(a_target, a_staggerMult, a_aggressor);
	currentPoiseHealth -= poiseDamage;

	auto staggerLevel = GetStaggerLevel(poiseDamage / totalPoiseHealth);
	auto immuneLevel = ImmuneLevelCalculator::GetTotalImmuneLevel(a_target);
	
	if (settings::bAdvancedLogging) {
		logger::info("ProcessMagicStagger: TotalPoise: {} | CurrentPoise: {} | Damage: {} | StaggerLevel: {} | ImmuneLevel: {}",
			totalPoiseHealth, currentPoiseHealth, poiseDamage, (uint32_t)staggerLevel, (uint32_t)immuneLevel);
	}
	// Case 1: Poise is broken entirely -> Trigger maximum break stagger
	if (currentPoiseHealth <= 0.f) {

		reactionHandler::applyStagger(a_aggressor, a_target, reactionHandler::reactionType::kLargest);
		staggerProtectTime = settings::fPoise_StaggerProtectTime;
		poiseHandler::GetSingleton()->setStaggerProtect(a_target, staggerProtectTime);
		currentPoiseHealth = totalPoiseHealth;
		
		if (settings::facts::TrueHudAPI_PoiseControlsSpecialBar) {
			TrueHUDUtils::greyOutSpecialMeter(a_target);
		}
		if (settings::bAdvancedLogging) {
			logger::info("ProcessMagicStagger: Poise Broken");
		}

	}
	// Case 2: Poise is not broken, target is not protected -> Evaluate thresholds
	else if (staggerProtectTime <= 0.f) {
		
		if (staggerLevel != StaggerLevel::kNone && staggerLevel > immuneLevel) {

			auto rType = static_cast<reactionHandler::reactionType>(staggerLevel);
			reactionHandler::applyStagger(a_aggressor, a_target, rType);

			if (settings::bAdvancedLogging) {
				logger::info("ProcessMagicStagger: Stagger level {} applied", (uint32_t)staggerLevel);
			}
		}
	}

	poiseHandler::GetSingleton()->setCurrentPoise(a_target, currentPoiseHealth);
	poiseHandler::GetSingleton()->setRegenDelay(a_target, settings::fPoise_RegenDelayTime);

	isProcessing = false; // Unlock the function

	if (settings::bAdvancedLogging) {
		logger::info("ProcessMagicStagger: Stagger processed successfully");
	}

	if (settings::bPoise_DebugConsoleLog) {
		if (auto selectedRef = RE::Console::GetSelectedRef()) {
			if (selectedRef->formID == a_target->GetFormID()) {
				std::ostringstream logs;
				logs << "-------Magic Stagger Result-------\n";

				if (staggerProtectTime > 0.f) {
					logs << "Largest Stagger! TotalHealth: " << totalPoiseHealth << "\n";
				} else {
					logs << "TotalHealth: " << totalPoiseHealth << "\n"
						 << "Damage: " << poiseDamage << "\n"
						 << "CurrentHealth: " << currentPoiseHealth << "\n"
						 << "StaggerLevel: " << static_cast<uint32_t>(staggerLevel) << "\n"
						 << "ImmuneLevel: " << static_cast<uint32_t>(immuneLevel) << "\n";
				}

				logs << "---------------------------------------";

				if (auto console = RE::ConsoleLog::GetSingleton()) {
					console->Print("%s", logs.str().c_str());
				}
			}
		}
	}
}

StaggerLevel StaggerHandler::GetStaggerLevel(const float& a_DamagePercent)
{
	// kNone = 0, kSmall = 1, kMedium = 2, kLarge = 3, kLargest = 4
	if (a_DamagePercent >= 1.0f)                           return StaggerLevel::kLargest;
	if (a_DamagePercent >= settings::fPoise_LargeStaggerTHLD)  return StaggerLevel::kLarge;
	if (a_DamagePercent >= settings::fPoise_MediumStaggerTHLD) return StaggerLevel::kMedium;
	if (a_DamagePercent >= settings::fPoise_SmallStaggerTHLD)  return StaggerLevel::kSmall;

	return StaggerLevel::kNone; // 0 damage percent means no stagger
}

StaggerLevel ImmuneLevelCalculator::GetTotalImmuneLevel(RE::Actor* a_target)
{
	if (!a_target)
		return StaggerLevel::kNone;

	auto magicTarget = a_target->AsMagicTarget();

	// Check Largest Immunity
	if (data::Poise_ImmuneLargest &&
		(a_target->HasKeyword(data::Poise_ImmuneLargest) || HasActiveEffectWithKeyword(magicTarget, data::Poise_ImmuneLargest))) {
		return StaggerLevel::kLargest;
	}

	// Check Large Immunity
	if (data::Poise_ImmuneLarge &&
		(a_target->HasKeyword(data::Poise_ImmuneLarge) || HasActiveEffectWithKeyword(magicTarget, data::Poise_ImmuneLarge))) {
		return StaggerLevel::kLarge;
	}

	// Check Medium Immunity
	if (data::Poise_ImmuneMedium &&
		(a_target->HasKeyword(data::Poise_ImmuneMedium) || HasActiveEffectWithKeyword(magicTarget, data::Poise_ImmuneMedium))) {
		return StaggerLevel::kMedium;
	}

	// Check Small Immunity
	if (data::Poise_ImmuneSmall &&
		(a_target->HasKeyword(data::Poise_ImmuneSmall) || HasActiveEffectWithKeyword(magicTarget, data::Poise_ImmuneSmall))) {
		return StaggerLevel::kSmall;
	}

	return StaggerLevel::kNone;
}

bool ImmuneLevelCalculator::HasActiveEffectWithKeyword(RE::MagicTarget* a_target, RE::BGSKeyword* a_keyword)
{
	auto effectList = a_target ? a_target->GetActiveEffectList() : nullptr;
	if (!effectList)
		return false;

	for (const auto effect : *effectList) {
		if (effect && effect->GetBaseObject() && effect->GetBaseObject()->HasKeyword(a_keyword) &&
			(effect->conditionStatus.get() == RE::ActiveEffect::ConditionStatus::kTrue))
			return true;
	}

	return false;
}
