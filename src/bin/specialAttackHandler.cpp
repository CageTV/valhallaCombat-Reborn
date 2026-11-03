#include "include/specialAttackHandler.h"
#include "include/settings.h"
#include "include/reactionHandler.h"
#include "include/Utils.h"
#include "include/data.h"
#include "include/offsets.h"
#include "ValhallaCombat.hpp"
#include "include/lib/BlockSpark.h"
#include <optional>

void SpecialAttackHandler::update() {

	std::vector<RE::FormID> expiredHyperArmors;

	{
		std::unique_lock<std::shared_mutex> lock(mtx_Attacks);
		float deltaTime = *RE::Offset::g_deltaTime;
		bool needsUpdate = false;

		// 1. Process Parry Timing States
		auto attackIt = _activeAttacks.begin();
		while (attackIt != _activeAttacks.end()) {
			if (attackIt->second.state == ParryState::kTiming) {
				needsUpdate = true;
				attackIt->second.timer += deltaTime;

				if (attackIt->second.timer > attackIt->second.window) {
					attackIt->second.state = ParryState::kFailure;
					if (settings::bAdvancedLogging) {
						logger::info("Parry Window Expired | Actor: {:X} | Window: {:.2f}s", attackIt->first, attackIt->second.window);
					}
				}
			}
			++attackIt;
		}

		// 2. Process Hyper Armor Countdown
		auto haIt = _activeHyperArmors.begin();
		while (haIt != _activeHyperArmors.end()) {
			if (haIt->second.duration.has_value()) {
				needsUpdate = true;

				*haIt->second.duration -= deltaTime;

				if (*haIt->second.duration <= 0.0f) {
					if (settings::bAdvancedLogging) {
						logger::info("HyperArmor Expired | Actor: {:X} | Reason: Duration Elapsed", haIt->first);
					}
					expiredHyperArmors.push_back(haIt->first);
					haIt = _activeHyperArmors.erase(haIt);
					continue;
				}
			}
			++haIt;
		}

		// 3. Process Invulnerability Countdown
		auto invulIt = _activeInvuls.begin();
		while (invulIt != _activeInvuls.end()) {
			if (invulIt->second.duration.has_value()) {
				needsUpdate = true;

				*invulIt->second.duration -= deltaTime;

				if (*invulIt->second.duration <= 0.0f) {
					if (settings::bAdvancedLogging) {
						logger::info("Invulnerability Expired | Actor: {:X} | Reason: Duration Elapsed", invulIt->first);
					}
					invulIt = _activeInvuls.erase(invulIt);
					continue;
				}
			}
			++invulIt;
		}

		if (!needsUpdate) {
			ValhallaCombat::GetSingleton()->deactivateUpdate(ValhallaCombat::HANDLER::specialAttackHandler);
		}
	}

	for (RE::FormID formID : expiredHyperArmors) {
		RE::Actor* actor = RE::TESForm::LookupByID<RE::Actor>(formID);
		if (actor) {
			Utils::Actor::safeRemoveSpell(data::hyperArmorSpell, actor);
		}
	}
}

void SpecialAttackHandler::processAnimEvent(RE::Actor* a_actor, const std::string& a_tag, const std::string& a_payload) {
	if (!a_actor) {
		return;
	}

	/*if (settings::bAdvancedLogging) {
		logger::info("AnimEvent Received | Actor: {:X} | Tag: {} | Payload: {}",
			a_actor->GetFormID(), a_tag, a_payload);
	}*/

	// Helper to expand INI presets
	auto expandPreset = [&](const std::string& payloadKey) -> std::string {
		auto it = settings::specialAttackPresets.find(payloadKey);
		if (it != settings::specialAttackPresets.end()) {
			std::string expanded = inlineUtils::to_lower_string(it->second);
			if (settings::bAdvancedLogging) {
				logger::info("AnimEvent Preset Expanded | Key: '{}' -> Value: '{}'", payloadKey, expanded);
			}
			return expanded;
		}
		return payloadKey;
	};

	// Process Attack Start
	if (a_tag == "val_specialatkstart") {
		std::string finalPayload = expandPreset(a_payload);

		// Helper to check for standalone tokens bounded by '|', '(', spaces, or string start/end
		auto hasStandaloneToken = [](const std::string& a_str, const std::string& a_token) -> bool {
			if (a_str.empty() || a_token.empty()) {
				return false;
			}

			size_t pos = a_str.find(a_token);
			while (pos != std::string::npos) {
				bool validStart = (pos == 0 || a_str[pos - 1] == '|' || std::isspace(static_cast<unsigned char>(a_str[pos - 1])));
				size_t endPos = pos + a_token.length();
				bool validEnd = (endPos == a_str.length() || a_str[endPos] == '|' || a_str[endPos] == '(' || std::isspace(static_cast<unsigned char>(a_str[endPos])));

				if (validStart && validEnd) {
					return true;
				}
				pos = a_str.find(a_token, pos + 1);
			}
			return false;
		};

		bool isParry = hasStandaloneToken(a_payload, "parry") || hasStandaloneToken(finalPayload, "parry");
		bool isDodge = hasStandaloneToken(a_payload, "dodge") || hasStandaloneToken(finalPayload, "dodge");

		startSpecialAttack(a_actor, finalPayload, isParry, isDodge);
	}
	else if (a_tag == "val_specialatkstop") {
		stopSpecialAttack(a_actor);
	}
	else if (a_tag == "val_hyperarmorstart") {
		std::string finalPayload = expandPreset(a_payload);
		startHyperArmor(a_actor, finalPayload);
	}
	else if (a_tag == "val_hyperarmorstop") {
		stopHyperArmor(a_actor);
	}
	// Action On Hit Start/Stop
	else if (a_tag == "val_actiononhitstart") {
		startActionOnHit(a_actor, a_payload);
	} 
	else if (a_tag == "val_actiononhitstop") {
		stopActionOnHit(a_actor);
	}
	// Invulnerability
	else if (a_tag == "val_invulstart") {
		std::string finalPayload = expandPreset(a_payload);
		startInvulnerability(a_actor, finalPayload);
	}
	else if (a_tag == "val_invulstop") {
		stopInvulnerability(a_actor);
	}
	// Process Attack Stop / Cleanup
	else if (a_tag == "attackstop" || a_tag == "bashstop") {
		clearState(a_actor);
	} 
	else if (a_tag == "mco_attackexitnotify" || a_tag == "mco_atkclose" 
		|| a_tag == "sbf_normalattackstart" || a_tag == "sbf_powerattackstart") {
		std::unique_lock<std::shared_mutex> lock(mtx_Attacks);
		auto it = _activeAttacks.find(a_actor->GetFormID());
		if (it != _activeAttacks.end()) {
			// 1. Reset stamina modifiers across all states
			it->second.staminaMult = 1.0f;
			it->second.fixedStamina = std::nullopt;

			bool isSuccessAndShouldClear = it->second.state == ParryState::kSuccess && !it->second.continueAttack.value_or(false);

			// 2. Clear entry on exit if attack finished, failed, or connected
			if (it->second.state == ParryState::kFailure ||
				it->second.state == ParryState::kNone ||
				it->second.state == ParryState::kHit ||
				isSuccessAndShouldClear) {
				lock.unlock();
				clearState(a_actor);
			}
		}
	}
}

// Special Attacks
void SpecialAttackHandler::startSpecialAttack(RE::Actor* a_actor, const std::string& a_payload, bool a_isParry, bool a_isDodge) {
	if (!a_actor) {
		return;
	}

	SpecialAttackData data;
	if (a_isParry || a_isDodge) {
		data.state = ParryState::kTiming;
	}

	// Prevents key collisions (e.g. matching "damagemult" inside "faildamagemult")
	auto findKeyPos = [&a_payload](const std::string& key) -> size_t {
		std::string searchStr = key + "(";
		size_t pos = a_payload.find(searchStr);
		while (pos != std::string::npos) {
			if (pos == 0 || !std::isalnum(static_cast<unsigned char>(a_payload[pos - 1]))) {
				return pos;
			}
			pos = a_payload.find(searchStr, pos + 1);
		}
		return std::string::npos;
	};

	auto extractFloat = [&](const std::string& key, float defaultVal) -> float {
		size_t pos = findKeyPos(key);
		if (pos != std::string::npos) {
			size_t start = pos + key.length() + 1;
			size_t end = a_payload.find(")", start);
			if (end != std::string::npos) {
				try {
					return std::stof(a_payload.substr(start, end - start));
				}
				catch (const std::exception& e) {
					logger::warn("SpecialAttackHandler: Failed to parse float for key [{}] in payload [{}]: {}", key, a_payload, e.what());
				}
			}
		}
		return defaultVal;
	};

	auto extractOptionalFloat = [&](const std::string& key) -> std::optional<float> {
		size_t pos = findKeyPos(key);
		if (pos != std::string::npos) {
			size_t start = pos + key.length() + 1;
			size_t end = a_payload.find(")", start);
			if (end != std::string::npos) {
				try {
					return std::stof(a_payload.substr(start, end - start));
				}
				catch (const std::exception& e) {
					logger::warn("SpecialAttackHandler: Failed to parse optional float for key [{}] in payload [{}]: {}", key, a_payload, e.what());
				}
			}
		}
		return std::nullopt;
	};

	auto extractBool = [&](const std::string& key) -> std::optional<bool> {
		size_t pos = findKeyPos(key);
		if (pos != std::string::npos) {
			size_t start = pos + key.length() + 1;
			size_t end = a_payload.find(")", start);
			if (end != std::string::npos) {
				std::string val = a_payload.substr(start, end - start);
				return (val == "true" || val == "1");
			}
		}
		return std::nullopt;
	};

	auto extractInt = [&](const std::string& key) -> std::optional<int> {
		size_t pos = findKeyPos(key);
		if (pos != std::string::npos) {
			size_t start = pos + key.length() + 1;
			size_t end = a_payload.find(")", start);
			if (end != std::string::npos) {
				try {
					return std::stoi(a_payload.substr(start, end - start));
				}
				catch (const std::exception& e) {
					logger::warn("SpecialAttackHandler: Failed to parse int for key [{}] in payload [{}]: {}", key, a_payload, e.what());
				}
			}
		}
		return std::nullopt;
	};

	// Lowercase & Substring-Safe Key Parsing
	data.damageMult = extractFloat("damagemult", data.damageMult);
	data.staggerMult = extractFloat("staggermult", data.staggerMult);
	data.staminaMult = extractFloat("staminamult", data.staminaMult);
	data.failDamageMult = extractFloat("faildamagemult", data.failDamageMult);
	data.failStaggerMult = extractFloat("failstaggermult", data.failStaggerMult);
	data.window = extractFloat("timing", data.window);
	data.fixedDamage = extractOptionalFloat("fixeddamage");
	data.fixedStamina = extractOptionalFloat("fixedstamina");
	data.poiseBreaksBlock = extractBool("poisebreaksblock");
	data.unblockableAttack = extractBool("unblockable");
	data.forcedStagger = extractInt("stagger");

	data.enableParrySFX = extractBool("parrysfx");
	data.enableDodgeSFX = extractBool("dodgesfx");
	data.enableScreenShake = extractBool("screenshake");
	data.enableSlowTime = extractBool("slowtime");
	data.slowTimeDuration = extractOptionalFloat("slowtimeduration");
	data.slowTimePercentage = extractOptionalFloat("slowtimepercentage");
	data.sparkType = extractInt("parryvfx");

	data.enableRecoil = extractBool("recoil");
	data.enableFollowupSpell = extractBool("followup");
	data.enableParrySpell = extractBool("parryspell");

	data.enableInvincibility = extractBool("iframes");

	data.continueAttack = extractBool("continue");

	data.isDodge = a_isDodge;

	if (settings::bAdvancedLogging) {
		logger::info("SpecialAttack Started | Actor: {:X} | Type: {} | Window: {:.2f}s | DmgMult: {:.2f} | StagMult: {:.2f} | StamMult: {:.2f} | FailDmg: {:.2f} | FailStag: {:.2f} | FixedDmg: {} | FixedStam: {} | PoiseBreaksBlock: {} | Unblockable: {}",
			a_actor->GetFormID(),
			a_isDodge ? "Dodge" : (a_isParry ? "Parry" : "SpecialAttack"),
			data.window,
			data.damageMult,
			data.staggerMult,
			data.staminaMult,
			data.failDamageMult,
			data.failStaggerMult,
			data.fixedDamage.has_value() ? std::to_string(data.fixedDamage.value()) : "None",
			data.fixedStamina.has_value() ? std::to_string(data.fixedStamina.value()) : "None",
			data.poiseBreaksBlock.has_value() ? (data.poiseBreaksBlock.value() ? "True" : "False") : "Unset",
			data.unblockableAttack.has_value() ? (data.unblockableAttack.value() ? "True" : "False") : "Unset");
	}

	// Thread-Safe State Assignment
	{
		std::unique_lock<std::shared_mutex> lock(mtx_Attacks);
		_activeAttacks[a_actor->GetFormID()] = data;
	}

	// Post-Parsing Event Activations
	if (a_isParry || a_isDodge) {
		ValhallaCombat::GetSingleton()->activateUpdate(ValhallaCombat::HANDLER::specialAttackHandler);
	}
	if (data.poiseBreaksBlock.has_value()) {
		a_actor->SetGraphVariableBool("Val_PoiseBreaksBlock", data.poiseBreaksBlock.value());
	}
	if (data.unblockableAttack.has_value()) {
		a_actor->SetGraphVariableBool("Val_Unblockable", data.unblockableAttack.value());
	}
	if (data.enableDodgeSFX.value_or(false)) {
		playDodgeSFX(a_actor);
	}
}

void SpecialAttackHandler::stopSpecialAttack(RE::Actor* a_actor)
{
	if (!a_actor) {
		return;
	}

	bool wasActive = false;
	{
		std::unique_lock<std::shared_mutex> lock(mtx_Attacks);
		wasActive = _activeAttacks.erase(a_actor->GetFormID()) > 0;
	}

	if (wasActive && settings::bAdvancedLogging) {
		logger::info("SpecialAttack Stopped | Actor: {:X}", a_actor->GetFormID());
	}
}

// Hit Logic
bool SpecialAttackHandler::processIncomingHit(RE::Actor* a_victim, RE::Actor* a_aggressor) {
	if (!a_victim || !a_aggressor) {
		return false;
	}

	SpecialAttackData dataCopy;
	bool isTiming = false;

	// 1. Quick read & state check under lock
	{
		std::shared_lock<std::shared_mutex> lock(mtx_Attacks);
		auto it = _activeAttacks.find(a_victim->GetFormID());
		if (it != _activeAttacks.end() && it->second.state == ParryState::kTiming) {
			isTiming = true;
			dataCopy = it->second; // Local copy for safe reading outside lock
		}
	}

	if (!isTiming) {
		return false;
	}

	bool bIsInBlockAngle = Utils::Actor::isInBlockAngle(a_victim, a_aggressor);
	bool bIsBashing = Utils::Actor::isPowerBashing(a_aggressor) || Utils::Actor::isBashing(a_aggressor);
	bool bIsUnblockable = false;

	if (settings::bUnblockableCompatibility) {
		a_aggressor->GetGraphVariableBool("isUnblockableHit", bIsUnblockable);
	}

	// 2. Perform engine angle check outside of lock
	if (!dataCopy.isDodge) {
		if (!bIsInBlockAngle || bIsUnblockable || bIsBashing) {
			// Transition to failure state on missed angle
			std::unique_lock<std::shared_mutex> lock(mtx_Attacks);
			auto it = _activeAttacks.find(a_victim->GetFormID());
			if (it != _activeAttacks.end()) {
				it->second.state = ParryState::kFailure;
			}
			return false; // Parry failed
		}
	}

	// 3. Commit state change to kSuccess under lock
	{
		std::unique_lock<std::shared_mutex> lock(mtx_Attacks);
		auto it = _activeAttacks.find(a_victim->GetFormID());
		if (it != _activeAttacks.end()) {
			it->second.state = ParryState::kSuccess;
		}
		else {
			return false; // State was cleared
		}
	}

	if (dataCopy.enableRecoil.value_or(true)) {
		reactionHandler::triggerRecoil(a_aggressor);
	}
	playParryEffects(a_victim, a_aggressor);

	if (dataCopy.enableParrySpell.value_or(false)) {
		Utils::Actor::safeApplySpell(data::parrySASpell, a_victim);
	}
	if (settings::bAdvancedLogging) {
		logger::info("{} successful for Actor {:X}", dataCopy.isDodge ? "Dodge" : "Parry", a_victim->GetFormID());
	}
	return dataCopy.enableInvincibility.value_or(true);
}

void SpecialAttackHandler::changeStateOnHit(RE::Actor* a_aggressor) {
	if (!a_aggressor) {
		return;
	}
	std::unique_lock<std::shared_mutex> lock(mtx_Attacks);
	auto it = _activeAttacks.find(a_aggressor->GetFormID());

	if (it != _activeAttacks.end()) {
		it->second.state = ParryState::kHit;
	}
}

bool SpecialAttackHandler::isParrySuccess(RE::Actor* a_aggressor) {
	if (!a_aggressor) {
		return false;
	}
	std::shared_lock<std::shared_mutex> lock(mtx_Attacks);
	auto it = _activeAttacks.find(a_aggressor->GetFormID());
	return (it != _activeAttacks.end() && it->second.state == ParryState::kSuccess);
}

// Special Attack Getter Functions
float SpecialAttackHandler::getDamageMultiplier(RE::Actor* a_actor) {
	if (!a_actor) {
		return 1.0f;
	}
	std::shared_lock<std::shared_mutex> lock(mtx_Attacks);
	auto it = _activeAttacks.find(a_actor->GetFormID());

	if (it != _activeAttacks.end()) {
		// Now uses the payload variable instead of hardcoded 0.25f
		if (it->second.state == ParryState::kFailure) return it->second.failDamageMult;
		return it->second.damageMult;
	}
	return 1.0f;
}

float SpecialAttackHandler::getStaggerMultiplier(RE::Actor* a_actor) {
	if (!a_actor) {
		return 1.0f;
	}
	std::shared_lock<std::shared_mutex> lock(mtx_Attacks);
	auto it = _activeAttacks.find(a_actor->GetFormID());

	if (it != _activeAttacks.end()) {
		// Now uses the payload variable instead of hardcoded 0.0f
		if (it->second.state == ParryState::kFailure) return it->second.failStaggerMult;
		return it->second.staggerMult;
	}
	return 1.0f;
}

float SpecialAttackHandler::getStaminaMultiplier(RE::Actor* a_actor) {
	if (!a_actor) {
		return 1.0f;
	}
	std::shared_lock<std::shared_mutex> lock(mtx_Attacks);
	auto it = _activeAttacks.find(a_actor->GetFormID());
	return (it != _activeAttacks.end()) ? it->second.staminaMult : 1.0f;
}

std::optional<float> SpecialAttackHandler::getFixedDamage(RE::Actor* a_actor) {
	if (!a_actor) {
		return std::nullopt;
	}
	std::shared_lock<std::shared_mutex> lock(mtx_Attacks);
	auto it = _activeAttacks.find(a_actor->GetFormID());

	return (it != _activeAttacks.end()) ? it->second.fixedDamage : std::nullopt;
}

std::optional<float> SpecialAttackHandler::getFixedStamina(RE::Actor* a_actor) {
	if (!a_actor) {
		return std::nullopt;
	}
	std::shared_lock<std::shared_mutex> lock(mtx_Attacks);
	auto it = _activeAttacks.find(a_actor->GetFormID());
	return (it != _activeAttacks.end()) ? it->second.fixedStamina : std::nullopt;
}

std::optional<int> SpecialAttackHandler::getForcedStagger(RE::Actor* a_actor)
{
	if (!a_actor) {
		return std::nullopt;
	}
	std::shared_lock<std::shared_mutex> lock(mtx_Attacks);
	auto it = _activeAttacks.find(a_actor->GetFormID());
	return (it != _activeAttacks.end()) ? it->second.forcedStagger : std::nullopt;
}

bool SpecialAttackHandler::shouldApplyParryFollowup(RE::Actor* a_aggressor) {
	if (!a_aggressor) {
		return false;
	}

	std::shared_lock<std::shared_mutex> lock(mtx_Attacks);
	auto it = _activeAttacks.find(a_aggressor->GetFormID());

	if (it != _activeAttacks.end()) {
		return it->second.enableFollowupSpell.value_or(true);
	}
	return false;
}

bool SpecialAttackHandler::shouldApplyFollowup(RE::Actor* a_aggressor) {
	if (!a_aggressor) {
		return false;
	}

	std::shared_lock<std::shared_mutex> lock(mtx_Attacks);
	auto it = _activeAttacks.find(a_aggressor->GetFormID());

	if (it != _activeAttacks.end()) {
		if (it->second.state == ParryState::kNone) {
			return it->second.enableFollowupSpell.value_or(true);
		}
	}
	return false;
}

// Hyper Armor Attacks
void SpecialAttackHandler::startHyperArmor(RE::Actor* a_actor, const std::string& a_payload) {
	if (!a_actor) {
		return;
	}

	HyperArmorData data;

	// Prevents key collisions (e.g. matching "damagemult" inside "faildamagemult")
	auto findKeyPos = [&a_payload](const std::string& key) -> size_t {
		std::string searchStr = key + "(";
		size_t pos = a_payload.find(searchStr);
		while (pos != std::string::npos) {
			if (pos == 0 || !std::isalnum(static_cast<unsigned char>(a_payload[pos - 1]))) {
				return pos;
			}
			pos = a_payload.find(searchStr, pos + 1);
		}
		return std::string::npos;
		};

	auto extractFloat = [&](const std::string& key, float defaultVal) -> float {
		size_t pos = findKeyPos(key);
		if (pos != std::string::npos) {
			size_t start = pos + key.length() + 1;
			size_t end = a_payload.find(")", start);
			if (end != std::string::npos) {
				try {
					return std::stof(a_payload.substr(start, end - start));
				}
				catch (const std::exception& e) {
					logger::warn("SpecialAttackHandler: Failed to parse float for key [{}] in hyper armor payload [{}]: {}", key, a_payload, e.what());
				}
			}
		}
		return defaultVal;
		};

	auto extractOptionalFloat = [&](const std::string& key) -> std::optional<float> {
		size_t pos = findKeyPos(key);
		if (pos != std::string::npos) {
			size_t start = pos + key.length() + 1;
			size_t end = a_payload.find(")", start);
			if (end != std::string::npos) {
				try {
					return std::stof(a_payload.substr(start, end - start));
				}
				catch (const std::exception& e) {
					logger::warn("SpecialAttackHandler: Failed to parse optional float for key [{}] in hyper armor payload [{}]: {}", key, a_payload, e.what());
				}
			}
		}
		return std::nullopt;
		};

	auto extractBool = [&](const std::string& key) -> std::optional<bool> {
		size_t pos = findKeyPos(key);
		if (pos != std::string::npos) {
			size_t start = pos + key.length() + 1;
			size_t end = a_payload.find(")", start);
			if (end != std::string::npos) {
				std::string val = a_payload.substr(start, end - start);
				return (val == "true" || val == "1");
			}
		}
		return std::nullopt;
		};

	// Lowercase & Substring-Safe Key Parsing
	data.damageMult = extractFloat("damagemult", data.damageMult);
	data.staggerMult = extractFloat("staggermult", data.staggerMult);
	data.enableVFX = extractBool("vfx");
	data.duration = extractOptionalFloat("duration");

	if (settings::bAdvancedLogging) {
		logger::info("HyperArmor Started | Actor: {:X} | DmgMult: {:.2f} | StagMult: {:.2f} | Duration: {} | VFX: {}",
			a_actor->GetFormID(),
			data.damageMult,
			data.staggerMult,
			data.duration.has_value() ? (std::to_string(data.duration.value()) + "s") : "Infinite",
			data.enableVFX.value_or(true) ? "True" : "False");
	}

	// Thread-Safe State Assignment
	{
		std::unique_lock<std::shared_mutex> lock(mtx_Attacks);
		_activeHyperArmors[a_actor->GetFormID()] = data;
	}

	// Post-Parsing Event Activations
	if (data.enableVFX.value_or(true)) {
		Utils::Actor::safeApplySpell(data::hyperArmorSpell, a_actor);
	}
	if (data.duration.has_value()) {
		ValhallaCombat::GetSingleton()->activateUpdate(ValhallaCombat::HANDLER::specialAttackHandler);
	}
}

void SpecialAttackHandler::stopHyperArmor(RE::Actor* a_actor) {
	if (!a_actor) {
		return;
	}

	bool wasActive = false;
	{
		std::unique_lock<std::shared_mutex> lock(mtx_Attacks);
		wasActive = _activeHyperArmors.erase(a_actor->GetFormID()) > 0;
	}

	if (wasActive) {
		Utils::Actor::safeRemoveSpell(data::hyperArmorSpell, a_actor);
		if (settings::bAdvancedLogging) {
			logger::info("Hyper Armor Stopped | Actor: {:X}", a_actor->GetFormID());
		}
	}
}

// Hyper Armor Getter Functions

bool SpecialAttackHandler::hasHyperArmor(RE::Actor* a_actor) {
	if (!a_actor) {
		return false;
	}
	std::shared_lock<std::shared_mutex> lock(mtx_Attacks);
	return _activeHyperArmors.find(a_actor->GetFormID()) != _activeHyperArmors.end();
}

float SpecialAttackHandler::getHyperArmorDamageMultiplier(RE::Actor* a_actor) {
	if (!a_actor) {
		return 1.0f;
	}
	std::shared_lock<std::shared_mutex> lock(mtx_Attacks);
	auto it = _activeHyperArmors.find(a_actor->GetFormID());
	return (it != _activeHyperArmors.end()) ? it->second.damageMult : 1.0f;
}

float SpecialAttackHandler::getHyperArmorStaggerMultiplier(RE::Actor* a_actor) {
	if (!a_actor) {
		return 1.0f;
	}
	std::shared_lock<std::shared_mutex> lock(mtx_Attacks);
	auto it = _activeHyperArmors.find(a_actor->GetFormID());
	return (it != _activeHyperArmors.end()) ? it->second.staggerMult : 1.0f;
}

// Invulnerability
void SpecialAttackHandler::startInvulnerability(RE::Actor* a_actor, const std::string& a_payload) {
	if (!a_actor) {
		return;
	}

	InvulData data;

	auto findKeyPos = [&a_payload](const std::string& key) -> size_t {
		std::string searchStr = key + "(";
		size_t pos = a_payload.find(searchStr);
		while (pos != std::string::npos) {
			if (pos == 0 || !std::isalnum(static_cast<unsigned char>(a_payload[pos - 1]))) {
				return pos;
			}
			pos = a_payload.find(searchStr, pos + 1);
		}
		return std::string::npos;
		};

	auto extractOptionalFloat = [&](const std::string& key) -> std::optional<float> {
		size_t pos = findKeyPos(key);
		if (pos != std::string::npos) {
			size_t start = pos + key.length() + 1;
			size_t end = a_payload.find(")", start);
			if (end != std::string::npos) {
				try {
					return std::stof(a_payload.substr(start, end - start));
				}
				catch (const std::exception& e) {
					logger::warn("SpecialAttackHandler: Failed to parse float for key [{}] in invul payload [{}]: {}", key, a_payload, e.what());
				}
			}
		}
		return std::nullopt;
		};

	data.duration = extractOptionalFloat("duration");

	if (settings::bAdvancedLogging) {
		logger::info("Invulnerability Started | Actor: {:X} | Duration: {}",
			a_actor->GetFormID(),
			data.duration.has_value() ? (std::to_string(data.duration.value()) + "s") : "Infinite");
	}

	{
		std::unique_lock<std::shared_mutex> lock(mtx_Attacks);
		_activeInvuls[a_actor->GetFormID()] = data;
	}

	if (data.duration.has_value()) {
		ValhallaCombat::GetSingleton()->activateUpdate(ValhallaCombat::HANDLER::specialAttackHandler);
	}
}

void SpecialAttackHandler::stopInvulnerability(RE::Actor* a_actor) {
	if (!a_actor) {
		return;
	}

	bool wasActive = false;
	{
		std::unique_lock<std::shared_mutex> lock(mtx_Attacks);
		wasActive = _activeInvuls.erase(a_actor->GetFormID()) > 0;
	}

	if (wasActive && settings::bAdvancedLogging) {
		logger::info("Invulnerability Stopped | Actor: {:X}", a_actor->GetFormID());
	}
}

bool SpecialAttackHandler::isInvulnerable(RE::Actor* a_actor) {
	if (!a_actor) {
		return false;
	}

	std::shared_lock<std::shared_mutex> lock(mtx_Attacks);
	return _activeInvuls.find(a_actor->GetFormID()) != _activeInvuls.end();
}

// Action On Hit
void SpecialAttackHandler::startActionOnHit(RE::Actor* a_actor, const std::string& a_payload)
{
	if (!a_actor) {
		return;
	}

	ActionOnHitData data;
	data.actionPayload = a_payload;

	if (settings::bAdvancedLogging) {
		logger::info("ActionOnHit Started | Actor: {:X} | Payload: {}", a_actor->GetFormID(), data.actionPayload);
	}

	{
		std::unique_lock<std::shared_mutex> lock(mtx_Attacks);
		_activeActionOnHits[a_actor->GetFormID()] = data;
	}
}

void SpecialAttackHandler::stopActionOnHit(RE::Actor* a_actor)
{
	if (!a_actor) {
		return;
	}

	bool wasActive = false;
	{
		std::unique_lock<std::shared_mutex> lock(mtx_Attacks);
		wasActive = _activeActionOnHits.erase(a_actor->GetFormID()) > 0;
	}

	if (wasActive && settings::bAdvancedLogging) {
		logger::info("ActionOnHit Stopped | Actor: {:X}", a_actor->GetFormID());
	}
}

bool SpecialAttackHandler::hasActionOnHit(RE::Actor* a_actor)
{
	if (!a_actor) {
		return false;
	}
	std::shared_lock<std::shared_mutex> lock(mtx_Attacks);
	return _activeActionOnHits.find(a_actor->GetFormID()) != _activeActionOnHits.end();
}

void SpecialAttackHandler::processActionOnHit(RE::Actor* a_actor)
{
	if (!a_actor) {
		return;
	}

	std::string payload;
	{
		std::shared_lock<std::shared_mutex> lock(mtx_Attacks);
		auto it = _activeActionOnHits.find(a_actor->GetFormID());
		if (it != _activeActionOnHits.end()) {
			payload = it->second.actionPayload;
		} else {
			return;
		}
	}

	RE::BGSAction* actionToPerform = nullptr;
	if (payload == "rightattack") {
		actionToPerform = data::actionRightAttack;
	}
	else if (payload == "rightpowerattack") {
		actionToPerform = data::actionRightPowerAttack;
	}

	if (actionToPerform) {
		Utils::Actor::performAction(actionToPerform, a_actor);
		if (settings::bAdvancedLogging) {
			logger::info("ActionOnHit Executed | Actor: {:X} | Action: {}", a_actor->GetFormID(), payload);
		}
	}
}

// Cleanup

void SpecialAttackHandler::clearState(RE::Actor* a_actor)
{
	if (!a_actor) {
		return;
	}

	bool clearedAttack = false;
	bool clearedHyperArmor = false;
	bool clearedInvul = false;
	bool clearedActionOnHit = false;

	{
		std::unique_lock<std::shared_mutex> lock(mtx_Attacks);
		clearedAttack = _activeAttacks.erase(a_actor->GetFormID()) > 0;
		clearedHyperArmor = _activeHyperArmors.erase(a_actor->GetFormID()) > 0;
		clearedInvul = _activeInvuls.erase(a_actor->GetFormID()) > 0;
		clearedActionOnHit = _activeActionOnHits.erase(a_actor->GetFormID()) > 0;
	}

	if (clearedHyperArmor) {
		Utils::Actor::safeRemoveSpell(data::hyperArmorSpell, a_actor);
	}

	if ((clearedAttack || clearedHyperArmor || clearedInvul || clearedActionOnHit) && settings::bAdvancedLogging) {
		std::vector<std::string_view> clearedStates;
		if (clearedAttack)
			clearedStates.push_back("SpecialAttack");
		if (clearedHyperArmor)
			clearedStates.push_back("HyperArmor");
		if (clearedInvul)
			clearedStates.push_back("Invul");
		if (clearedActionOnHit)
			clearedStates.push_back("ActionOnHit");

		std::string clearedStr;
		for (size_t i = 0; i < clearedStates.size(); ++i) {
			if (i > 0)
				clearedStr += ", ";
			clearedStr += clearedStates[i];
		}

		logger::info("State Cleared | Actor: {:X} | Cleared: [{}]",
			a_actor->GetFormID(), clearedStr);
	}
}

void SpecialAttackHandler::onActorDeath(RE::Actor* a_actor) {
	if (!a_actor) {
		return;
	}
	{
		std::unique_lock<std::shared_mutex> lock(mtx_Attacks);
		_activeAttacks.erase(a_actor->GetFormID());
		_activeHyperArmors.erase(a_actor->GetFormID());
		_activeInvuls.erase(a_actor->GetFormID());
		_activeActionOnHits.erase(a_actor->GetFormID());
	}
	Utils::Actor::safeRemoveSpell(data::hyperArmorSpell, a_actor);
	Utils::Actor::safeRemoveSpell(data::followupSpell, a_actor);
	Utils::Actor::safeRemoveSpell(data::parryFollowupSpell, a_actor);
	Utils::Actor::safeRemoveSpell(data::parrySASpell, a_actor);
}

void SpecialAttackHandler::reset() {
	{
		std::unique_lock<std::shared_mutex> lock(mtx_Attacks);
		_activeAttacks.clear();
		_activeHyperArmors.clear();
		_activeInvuls.clear();
		_activeActionOnHits.clear();
	}
}

// Effects

void SpecialAttackHandler::playParrySFX(RE::Actor* parrier, bool parriedWithWeapon) {
	if (parriedWithWeapon) {
		ValhallaUtils::playSound(parrier, data::soundParryWeapon_perfect, settings::fParrySFXVolume);
	}
	else {
		ValhallaUtils::playSound(parrier, data::soundParryShield_perfect, settings::fParrySFXVolume);
	}
}

void SpecialAttackHandler::playParryVFX(RE::Actor* parrier, bool parriedWithWeapon) {
	auto blockFXNode = RE::Offset::PlaceAtMe(parrier, data::BlockFX, 1, false, false);
	std::string node = parriedWithWeapon ? "WEAPON" : "SHIELD";

	blockFXNode->MoveToNode(parrier, node);

	RE::Offset::PlaceAtMe(blockFXNode, data::BlockSpark, 1, false, false);
	RE::Offset::PlaceAtMe(blockFXNode, data::BlockSparkFlare, 1, false, false);
	RE::Offset::PlaceAtMe(blockFXNode, data::BlockSparkRing, 1, false, false);

	blockFXNode->SetDelete(true);
}

void SpecialAttackHandler::playParryScreenShake() {
	RE::Offset::shakeCamera(1.5f, RE::PlayerCharacter::GetSingleton()->GetPosition(), 0.4f);
}

void SpecialAttackHandler::playParrySlowTime(std::optional<float> a_durationOverride, std::optional<float> a_percentageOverride) {
	float slowDuration = a_durationOverride.value_or(settings::fParrySlowTimeDuration);
	float slowPercentage = a_percentageOverride.value_or(settings::fParrySlowTimePercentage);

	if (slowDuration > 0.0f) {
		inlineUtils::slowTime(slowDuration, slowPercentage);
	}
}

void SpecialAttackHandler::playParryEffects(RE::Actor* parrier, RE::Actor* attacker) {
	bool shieldEquipped = RE::Offset::getEquippedShield(parrier);
	bool parriedWithWeapon = !shieldEquipped;

	// Fetch local overrides safely if they exist
	std::optional<bool> overrideSFX;
	std::optional<bool> overrideScreenShake;
	std::optional<bool> overrideSlowTime;
	std::optional<int>  overrideSparkType;
	std::optional<float> overrideSlowDuration;
	std::optional<float> overrideSlowPercentage;

	if (parrier) {
		std::shared_lock<std::shared_mutex> lock(mtx_Attacks);
		auto it = _activeAttacks.find(parrier->GetFormID());
		if (it != _activeAttacks.end()) {
			overrideSFX = it->second.enableParrySFX;
			overrideScreenShake = it->second.enableScreenShake;
			overrideSlowTime = it->second.enableSlowTime;
			overrideSparkType = it->second.sparkType;
			overrideSlowDuration = it->second.slowTimeDuration;
			overrideSlowPercentage = it->second.slowTimePercentage;
		}
	}

	// Determine SFX (Payload override -> Global Setting fallback)
	bool useSFX = overrideSFX.value_or(settings::bParrySFX);
	if (useSFX) {
		playParrySFX(parrier, parriedWithWeapon);
	}

	// Determine Screen Shake
	bool useScreenShake = overrideScreenShake.value_or(settings::bParryScreenShake);
	if (useScreenShake &&
		((parrier && parrier->IsPlayerRef()) || (attacker && attacker->IsPlayerRef()))) {
		playParryScreenShake();
	}

	// Determine Slow Time
	bool useSlowTime = overrideSlowTime.value_or(settings::bParrySlowTime);
	if (useSlowTime && parrier && parrier->IsPlayerRef()) {
		playParrySlowTime(overrideSlowDuration, overrideSlowPercentage);
	}

	// Determine Spark Type
	int sparkType = overrideSparkType.value_or(settings::uParrySparkType);
	switch (sparkType) {
	case 0:
		default:
		break;
	case 1:
		playParryVFX(parrier, parriedWithWeapon);
		break;
	case 2:
		MaxsuBlockSpark::blockSpark::playPerfectBlockSpark(parrier);
		break;
	}
}

void SpecialAttackHandler::playDodgeSFX(RE::Actor* a_actor) {
	if (!a_actor) {
		return;
	}

	RE::TESObjectARMO* bodyArmor = a_actor->GetWornArmor(RE::BGSBipedObjectForm::BipedObjectSlot::kBody);

	bool isHeavy = false;
	bool isLight = false;
	bool hasShield = Utils::Actor::isEquippedShield(a_actor);

	if (bodyArmor) {
		if (bodyArmor->IsHeavyArmor()) {
			isHeavy = true;
		}
		else if (bodyArmor->IsLightArmor()) {
			isLight = true;
		}
	}

	// Heavy armor + Shield = Heavy Dodge (Volume 10)
	if (isHeavy && hasShield) {
		ValhallaUtils::playSound(a_actor, data::soundHeavyArmorDodge, 10.0f);
	}
	// Heavy armor + No Shield = Medium Dodge (Volume 9)
	else if (isHeavy && !hasShield) {
		ValhallaUtils::playSound(a_actor, data::soundMediumArmorDodge, 9.0f);
	}
	// Light armor = Light Dodge (Volume 9)
	else if (isLight) {
		ValhallaUtils::playSound(a_actor, data::soundLightArmorDodge, 9.0f);
	}
	// Clothing, robes, or naked (Unarmored) = Unarmored Dodge (Volume 8)
	else {
		ValhallaUtils::playSound(a_actor, data::soundUnarmoredDodge, 8.0f);
	}
}
