#pragma once
#include "lib/robin_hood.h"
#include <chrono>
#include <shared_mutex>
#include "include/settings.h"

using uniqueLocker = std::unique_lock<std::shared_mutex>;
using sharedLocker = std::shared_lock<std::shared_mutex>;

struct AttackerProfile {
    float avgLightImpactTime = 0.0f;
    float avgPowerImpactTime = 0.0f;
};

class AILearning {
public:
    static AILearning* GetSingleton() {
        static AILearning singleton;
        return &singleton;
    }

    // --- Record Attack Start ---
    void safeRecordAttackStart(RE::FormID a_attackerID);
    void safeRecordAttackStart(RE::Actor* a_attacker);

    // --- Record Impact---
    void safeRecordImpact(RE::FormID a_attackerID, RE::FormID a_defenderID, bool a_isPowerAttack);
    void safeRecordImpact(RE::Actor* a_attacker, RE::Actor* a_defender, bool a_isPowerAttack);

    // --- Record Attack Stop ---
    void safeRecordAttackStop(RE::FormID a_attackerID);
    void safeRecordAttackStop(RE::Actor* a_attacker);

    // --- Retrieve Timing ---
    float safeGetEstimatedImpactTime(RE::FormID a_defenderID, RE::FormID a_attackerID, bool a_isPowerAttack);
    float safeGetEstimatedImpactTime(RE::Actor* a_defender, RE::Actor* a_attacker, bool a_isPowerAttack);

    // --- Memory Management ---
    void safeClearProfile(RE::FormID a_defenderID);
    void safeClearProfile(RE::Actor* a_defender);
    void update();
    void reset();

private:
    std::shared_mutex mtx_activeAttacks;
    std::shared_mutex mtx_learnedProfiles;

    robin_hood::unordered_map<RE::FormID, float> activeAttacksMap;
    robin_hood::unordered_map<RE::FormID, robin_hood::unordered_map<RE::FormID, AttackerProfile>> learnedProfilesMap;
};