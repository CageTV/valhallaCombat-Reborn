#pragma once
#include "lib/robin_hood.h"
#include <shared_mutex>
#include <chrono>
#include <mutex>
#include <vector>

class AI {
public:
    static AI* GetSingleton() {
        static AI singleton;
        return &singleton;
    }

    void ProcessCombatState(RE::Actor* a_actor, RE::Actor* a_target, RE::ACTOR_COMBAT_STATE a_state);
    void EvaluateAttackReaction(RE::Actor* a_attacker);
    bool ShouldSinkEvent(RE::Actor* actor, const RE::BSFixedString& a_eventName);
    void onActorDeath(RE::Actor* a_actor);
    void update();
    void reset();

private:

    enum class AIActionType {
        Parry,
        TimedBlock
    };

    struct PendingAIReaction {
        RE::FormID actorID;
        AIActionType action;
        float delayRemaining;
    };

    struct AIState {
        RE::FormID currentTarget = 0;
        float parryCooldown = 0.0f;
        float blockCooldown = 0.0f;
    };

    robin_hood::unordered_map<RE::FormID, AIState> _aiStates;
    std::shared_mutex mtx_aiStates;

    std::vector<PendingAIReaction> _pendingReactions;
    std::mutex mtx_pendingReactions;

    // Trigger actions
    bool TriggerTimedBlock(RE::Actor* a_defender, float a_delaySeconds);
    bool TriggerParry(RE::Actor* a_defender, float a_delaySeconds);

    // Multiplier calculation helpers
    float GetSkillLevelMult(RE::Actor* a_actor);
    float GetParryCombatStyleMult(RE::Actor* a_actor);
    float GetBlockCombatStyleMult(RE::Actor* a_actor);

    // Roll evaluation helpers
    bool ShouldAttemptParry(RE::Actor* a_defender, RE::Actor* a_attacker);
    bool ShouldAttemptPerfectBlock(RE::Actor* a_defender, RE::Actor* a_attacker);
    bool ShouldAttemptTimedBlock(RE::Actor* a_defender, RE::Actor* a_attacker);

    // FormID Base Functions
    bool IsOnCooldown(RE::FormID a_actorID, bool a_isParry);
    void SetCooldown(RE::FormID a_actorID, bool a_isParry);

    // Actor Pointer Safe Wrappers
    bool IsOnCooldown(RE::Actor* a_actor, bool a_isParry);
    void SetCooldown(RE::Actor* a_actor, bool a_isParry);
};