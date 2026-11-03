#include "include/AILearning.h"
#include "include/Utils.h"
#include "ValhallaCombat.hpp"

// Attack Start Tracking

void AILearning::safeRecordAttackStart(RE::FormID a_attackerID) {
    {
        uniqueLocker lock(mtx_activeAttacks);
        activeAttacksMap[a_attackerID] = 0.0f; // Initialize elapsed time to 0
    }

    ValhallaCombat::GetSingleton()->activateUpdate(ValhallaCombat::aiLearningHandler);

    if (settings::bAdvancedLogging) {
        logger::info("AI Learning: Recorded attack start for [{:X}]", a_attackerID);
    }
}

void AILearning::safeRecordAttackStart(RE::Actor* a_attacker) {
    if (a_attacker) {
        safeRecordAttackStart(a_attacker->GetFormID());
    }
}

// Impact Learning

void AILearning::safeRecordImpact(RE::FormID a_attackerID, RE::FormID a_defenderID, bool a_isPowerAttack) {
    float actualTime = 0.0f;
    bool hasActiveAttack = false;

    // Read the attack elapsed time and erase it from memory
    {
        uniqueLocker lock(mtx_activeAttacks);
        auto it = activeAttacksMap.find(a_attackerID);
        if (it != activeAttacksMap.end()) {
            actualTime = it->second;
            hasActiveAttack = true;
            activeAttacksMap.erase(it);
        }
    }

    if (hasActiveAttack) {
        // Sanity range check (0.1s to 3.0s)
        if (actualTime > 0.1f && actualTime < 3.0f) {
            float updatedAvg = 0.0f;
            {
                uniqueLocker lock(mtx_learnedProfiles);
                AttackerProfile& profile = learnedProfilesMap[a_defenderID][a_attackerID];

                if (a_isPowerAttack) {
                    if (profile.avgPowerImpactTime <= 0.0f) {
                        profile.avgPowerImpactTime = actualTime;
                    }
                    else {
                        profile.avgPowerImpactTime = (profile.avgPowerImpactTime * 0.7f) + (actualTime * 0.3f);
                    }
                    updatedAvg = profile.avgPowerImpactTime;
                }
                else {
                    if (profile.avgLightImpactTime <= 0.0f) {
                        profile.avgLightImpactTime = actualTime;
                    }
                    else {
                        profile.avgLightImpactTime = (profile.avgLightImpactTime * 0.7f) + (actualTime * 0.3f);
                    }
                    updatedAvg = profile.avgLightImpactTime;
                }
            }
            if (settings::bAdvancedLogging) {
                logger::info("AI Learning: Recorded {} impact [{:X} -> {:X}] | Raw: {:.3f}s, New Avg: {:.3f}s",
                    a_isPowerAttack ? "Power" : "Light", a_attackerID, a_defenderID, actualTime, updatedAvg);
            }
        }
    }
}

void AILearning::safeRecordImpact(RE::Actor* a_attacker, RE::Actor* a_defender, bool a_isPowerAttack) {
    if (a_attacker && a_defender && Utils::Actor::isAISupportedNPC(a_defender)) {
        safeRecordImpact(a_attacker->GetFormID(), a_defender->GetFormID(), a_isPowerAttack);
    }
}

void AILearning::safeRecordAttackStop(RE::FormID a_attackerID) {
    {
        uniqueLocker lock(mtx_activeAttacks);
        activeAttacksMap.erase(a_attackerID);
    }
}

void AILearning::safeRecordAttackStop(RE::Actor* a_attacker) {
    if (a_attacker) {
        safeRecordAttackStop(a_attacker->GetFormID());
    }
}

// Get Timing Estimates

float AILearning::safeGetEstimatedImpactTime(RE::FormID a_defenderID, RE::FormID a_attackerID, bool a_isPowerAttack) {
    sharedLocker lock(mtx_learnedProfiles);

    auto defenderIt = learnedProfilesMap.find(a_defenderID);
    if (defenderIt != learnedProfilesMap.end()) {
        auto attackerIt = defenderIt->second.find(a_attackerID);
        if (attackerIt != defenderIt->second.end()) {
            float learnedTime = a_isPowerAttack ? attackerIt->second.avgPowerImpactTime : attackerIt->second.avgLightImpactTime;

            // Return learned value if at least one hit has been recorded
            if (learnedTime > 0.0f) {
                return learnedTime;
            }
        }
    }

    // Fallback value used only when no empirical data exists
    return a_isPowerAttack ? settings::fBasePowerImpactTime : settings::fBaseLightImpactTime;
}

float AILearning::safeGetEstimatedImpactTime(RE::Actor* a_defender, RE::Actor* a_attacker, bool a_isPowerAttack) {
    if (!a_defender || !a_attacker) {
        return a_isPowerAttack ? settings::fBasePowerImpactTime : settings::fBaseLightImpactTime;
    }
    return safeGetEstimatedImpactTime(a_defender->GetFormID(), a_attacker->GetFormID(), a_isPowerAttack);
}

void AILearning::update() {
    float deltaTime = *RE::Offset::g_deltaTime;
    bool keepActive = false;

    {
        uniqueLocker lock(mtx_activeAttacks);

        for (auto it = activeAttacksMap.begin(); it != activeAttacksMap.end(); ) {
            it->second += deltaTime;

            // If an attack has been active for more than 3.0 seconds without an impact,
            // it was likely interrupted (stagger, death, etc.). Garbage collect it.
            if (it->second > 3.0f) {
                if (settings::bAdvancedLogging) {
                    logger::info("AI Learning: Discarded stale attack for [{:X}] (Exceeded 3.0s)", it->first);
                }
                it = activeAttacksMap.erase(it);
            }
            else {
                keepActive = true;
                ++it;
            }
        }
    }

    // Shut down the handler loop if there are no more active attacks to track
    if (!keepActive) {
        ValhallaCombat::GetSingleton()->deactivateUpdate(ValhallaCombat::aiLearningHandler);
    }
}

// Cleanup Methods

void AILearning::safeClearProfile(RE::FormID a_actorID) {
    std::scoped_lock lock(mtx_activeAttacks, mtx_learnedProfiles);
    activeAttacksMap.erase(a_actorID);
    learnedProfilesMap.erase(a_actorID);
}

void AILearning::safeClearProfile(RE::Actor* a_actor) {
    if (a_actor) {
        safeClearProfile(a_actor->GetFormID());
    }
}

void AILearning::reset() {
    std::scoped_lock lock(mtx_activeAttacks, mtx_learnedProfiles);
    activeAttacksMap.clear();
    learnedProfilesMap.clear();
}