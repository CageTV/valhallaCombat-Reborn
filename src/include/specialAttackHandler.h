#pragma once
#include "lib/robin_hood.h"
#include <shared_mutex>
#include <string>
#include <optional>

class SpecialAttackHandler {
public:
    static SpecialAttackHandler* GetSingleton() {
        static SpecialAttackHandler singleton;
        return std::addressof(singleton);
    }

    enum class ParryState {
        kNone,
        kTiming,
        kSuccess,
        kFailure,
        kHit
    };

    struct SpecialAttackData {
        float damageMult = 1.0f;
        float staggerMult = 1.0f;
        float staminaMult = 1.0f;
        float failDamageMult = 1.0f;
        float failStaggerMult = 1.0f;
        std::optional<float> fixedDamage = std::nullopt;
        std::optional<float> fixedStamina = std::nullopt;
        std::optional<bool> poiseBreaksBlock = std::nullopt;
		std::optional<bool> unblockableAttack = std::nullopt;
		std::optional<int> forcedStagger = std::nullopt;

        ParryState state = ParryState::kNone;
        float timer = 0.0f;
        float window = 0.3f;

        std::optional<bool> enableParrySFX;
        std::optional<bool> enableDodgeSFX;
        std::optional<bool> enableScreenShake;
        std::optional<bool> enableSlowTime;
        std::optional<int>  sparkType;
        std::optional<float> slowTimeDuration;
        std::optional<float> slowTimePercentage;

        std::optional<bool> enableRecoil;
        std::optional<bool> enableFollowupSpell;
        std::optional<bool> enableParrySpell;

        std::optional<bool> enableInvincibility;

        std::optional<bool> continueAttack;

        bool isDodge = false;
    };

    struct HyperArmorData {
        float damageMult = 1.0f;
        float staggerMult = 0.0f;
        std::optional<bool> enableVFX;
        std::optional<float> duration;
    };

    struct InvulData {
        std::optional<float> duration;
    };

	struct ActionOnHitData
	{
		std::string actionPayload;
	};

    void update();
    void processAnimEvent(RE::Actor* a_actor, const std::string& a_tag, const std::string& a_payload);

    // Parry
    bool processIncomingHit(RE::Actor* a_victim, RE::Actor* a_aggressor);
    bool isParrySuccess(RE::Actor* a_aggressor);
    void changeStateOnHit(RE::Actor* a_aggressor);
    bool shouldApplyParryFollowup(RE::Actor* a_aggressor);
    bool shouldApplyFollowup(RE::Actor* a_aggressor);

    //  Special Attack Getters
    float getDamageMultiplier(RE::Actor* a_actor);
    float getStaggerMultiplier(RE::Actor* a_actor);
    float getStaminaMultiplier(RE::Actor* a_actor);
    std::optional<float> getFixedDamage(RE::Actor* a_actor);
    std::optional<float> getFixedStamina(RE::Actor* a_actor);
	std::optional<int> getForcedStagger(RE::Actor* a_actor);

    // Hyper Armor
    bool hasHyperArmor(RE::Actor* a_actor);
    float getHyperArmorDamageMultiplier(RE::Actor* a_actor);
    float getHyperArmorStaggerMultiplier(RE::Actor* a_actor);

    // Invulnerability
    bool isInvulnerable(RE::Actor* a_actor);

	// Action On Hit
	bool hasActionOnHit(RE::Actor* a_actor);
	void processActionOnHit(RE::Actor* a_actor);

    void clearState(RE::Actor* a_actor);
    void onActorDeath(RE::Actor* a_actor);
    void reset();

private:
    void startSpecialAttack(RE::Actor* a_actor, const std::string& a_payload, bool a_isParry, bool a_isDodge);
	void stopSpecialAttack(RE::Actor* a_actor);

	void startHyperArmor(RE::Actor* a_actor, const std::string& a_payload);
	void stopHyperArmor(RE::Actor* a_actor);

	void startInvulnerability(RE::Actor* a_actor, const std::string& a_payload);
	void stopInvulnerability(RE::Actor* a_actor);

	void startActionOnHit(RE::Actor* a_actor, const std::string& a_payload);
	void stopActionOnHit(RE::Actor* a_actor);

    void playParrySFX(RE::Actor* parrier, bool parriedWithWeapon);
    void playParryVFX(RE::Actor* parrier, bool parriedWithWeapon);
    void playParryScreenShake();
    void playParrySlowTime(std::optional<float> a_durationOverride, std::optional<float> a_percentageOverride);
    void playParryEffects(RE::Actor* parrier, RE::Actor* attacker);

    void playDodgeSFX(RE::Actor* a_actor);

	robin_hood::unordered_map<RE::FormID, SpecialAttackData> _activeAttacks;
    robin_hood::unordered_map<RE::FormID, HyperArmorData> _activeHyperArmors;
    robin_hood::unordered_map<RE::FormID, InvulData> _activeInvuls;
	robin_hood::unordered_map<RE::FormID, ActionOnHitData> _activeActionOnHits;
    std::shared_mutex mtx_Attacks;
};
