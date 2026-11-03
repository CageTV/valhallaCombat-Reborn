#include "include/AI.h"
#include "include/events.h"
#include "include/Utils.h"
#include "include/AILearning.h"
#include "include/settings.h"
#include "ValhallaCombat.hpp"

static const robin_hood::unordered_set<RE::BSFixedString> kDodgeEvents = {
    "TKDodgeForward",
    "TKDodgeRight",
    "TKDodgeLeft",
    "TKDodgeBack"
};

static bool CanDefend(RE::Actor* a_defender)
{
    if (!a_defender) {
        return false;
    }

    // 1. Dual Wield Check
    if (!settings::bToggleDualWield && Utils::Actor::isDualWielding(a_defender)) {
        logger::info("AI: [{:X}] cannot defend -> Dual Wielding defense disabled", a_defender->GetFormID());
        return false;
    }

    // 2. Unarmed Check
    if (!settings::bToggleUnarmed) {
        auto rhs = a_defender->GetEquippedObject(false);
        auto lhs = a_defender->GetEquippedObject(true);

        bool hasRightWeapon = rhs && rhs->IsWeapon();
        bool hasLeftWeapon = lhs && lhs->IsWeapon();
        bool hasLeftShield = lhs && lhs->IsArmor();

        if (!hasRightWeapon && !hasLeftWeapon && !hasLeftShield) {
            logger::info("AI: [{:X}] cannot defend -> Unarmed defense disabled", a_defender->GetFormID());
            return false;
        }
    }

    return true;
}

constexpr auto ScaleAroundBaseline = [](float a_value, float a_scale) {
    return 1.0f + ((a_value - 1.0f) * a_scale);
};

void AI::ProcessCombatState(RE::Actor* a_actor, RE::Actor* a_target, RE::ACTOR_COMBAT_STATE a_state)
{
    if (!a_actor || !Utils::Actor::isAISupportedNPC(a_actor)) {
        return;
    }

    if (a_state == RE::ACTOR_COMBAT_STATE::kCombat) {
        RE::FormID targetID = a_target ? a_target->GetFormID() : 0;
        {
            std::unique_lock<std::shared_mutex> lock(mtx_aiStates);
            _aiStates[a_actor->GetFormID()].currentTarget = targetID;
        }
        if (settings::bAdvancedLogging) {
            logger::info("AI: Registered combat state for [{:X}] -> Target [{:X}]", a_actor->GetFormID(), targetID);
        }
    }
    else if (a_state == RE::ACTOR_COMBAT_STATE::kNone) {
        a_actor->SetGraphVariableBool("Val_ReactionPending", false);
        const RE::FormID actorID = a_actor->GetFormID();
        {
            std::unique_lock<std::shared_mutex> lock(mtx_aiStates);
            _aiStates.erase(actorID);
        }
        {
            std::lock_guard<std::mutex> lock(mtx_pendingReactions);

            std::erase_if(_pendingReactions, [actorID](const PendingAIReaction& reaction) {
                return reaction.actorID == actorID;
                });
        }
        AILearning::GetSingleton()->safeClearProfile(a_actor);
        if (settings::bAdvancedLogging) {
            logger::info("AI: Cleared combat state for [{:X}]", actorID);
        }
    }
}

// AI Event Sink

bool AI::ShouldSinkEvent(RE::Actor* actor, const RE::BSFixedString& a_eventName)
{
    if (!actor || actor->IsPlayerRef()) {
        return false;
    }

    std::string_view eventStr = a_eventName.c_str();

    // 1. Regular Bash Check
    if (eventStr == "bashStart") {
        if (settings::bStopAIBash) {
            if (settings::bAdvancedLogging) {
                logger::info("AI: Sunk 'bashStart' for [{:X}] (bStopAIBash=true)", actor->GetFormID());
            }
            return true;
        }
        bool bashAllowed = true;
        if (actor->GetGraphVariableBool("Val_BashAllowed", bashAllowed) && !bashAllowed && settings::bToggleAI) {
            if (settings::bAdvancedLogging) {
                logger::info("AI: Sunk 'bashStart' for [{:X}] (bashAllowed=false)", actor->GetFormID());
            }
            return true;
        }
    }
    // 2. Power Bash Check
    else if (eventStr == "bashPowerStart") {
        if (settings::bStopAIBashPower) {
            if (settings::bAdvancedLogging) {
                logger::info("AI: Sunk 'bashPowerStart' for [{:X}] (bStopAIBashPower=true)", actor->GetFormID());
            }
            return true;
        }
        bool reactionPending = false;
        if (actor->GetGraphVariableBool("Val_ReactionPending", reactionPending) && reactionPending && settings::bToggleAI) {
            if (settings::bAdvancedLogging) {
                logger::info("AI: Sunk 'bashPowerStart' for [{:X}] (reactionPending=true)", actor->GetFormID());
            }
            return true;
        }
        if (settings::bAttackStaminaToggle_NPC && settings::bUseCustomPowerBashCost) {
            if (auto avOwner = actor->AsActorValueOwner()) {
                float powerBashCost = 0.0f;
                if (actor->GetGraphVariableFloat("Val_PowerBashCost", powerBashCost)) {
                    float currentStamina = avOwner->GetActorValue(RE::ActorValue::kStamina);
                    if (currentStamina < powerBashCost) {
                        bool bypassStaminaCheck = Utils::Random::RollNormalized(0.20f);
                        if (!bypassStaminaCheck) {
                            if (settings::bAdvancedLogging) {
                                logger::info("AI: Sunk 'bashPowerStart' for [{:X}] due to low stamina ({:.1f}/{:.1f})", actor->GetFormID(), currentStamina, powerBashCost);
                            }
                            return true;
                        }
                        else if (settings::bAdvancedLogging) {
                            logger::info("AI: [{:X}] bypassed low stamina check for 'bashPowerStart'", actor->GetFormID());
                        }
                    }
                }
            }
        }
    }
    // 3. Block Check
    else if (eventStr == "blockStart") {
        bool reactionPending = false;
        if (actor->GetGraphVariableBool("Val_ReactionPending", reactionPending)) {
            if (reactionPending && settings::bToggleAI) {
                if (settings::bAdvancedLogging) {
                    logger::info("AI: Sunk 'blockStart' for [{:X}] (reactionPending=true)", actor->GetFormID());
                }
                return true;
            }
        }
    }
    // 4. Dodge Check
    else if (kDodgeEvents.contains(a_eventName)) {
        bool reactionPending = false;
        if (actor->GetGraphVariableBool("Val_ReactionPending", reactionPending) && reactionPending && settings::bToggleAI) {
            if (settings::bAdvancedLogging) {
                logger::info("AI: Sunk Dodge '{}' for [{:X}] (reactionPending=true)", eventStr, actor->GetFormID());
            }
            return true;
        }
    }
    else {
        std::string lowerStr = inlineUtils::to_lower_string(eventStr);

        bool hasPower = lowerStr.find("power") != std::string::npos;
        bool hasAttackStart = lowerStr.find("attackstart") != std::string::npos;
        bool hasPowerStart = lowerStr.find("attackpowerstart") != std::string::npos;

        bool isPowerAttack = hasPowerStart || (hasAttackStart && hasPower);
        bool isLightAttack = hasAttackStart && !hasPower;

        // 5. Power Attack Check
        if (isPowerAttack) {
            bool reactionPending = false;
            if (actor->GetGraphVariableBool("Val_ReactionPending", reactionPending) && reactionPending && settings::bToggleAI) {
                if (settings::bAdvancedLogging) {
                    logger::info("AI: Sunk Power Attack '{}' for [{:X}] (reactionPending=true)", eventStr, actor->GetFormID());
                }
                return true;
            }
            if (settings::bAttackStaminaToggle_NPC && settings::bUseCustomPowerAttackCost) {
                if (auto avOwner = actor->AsActorValueOwner()) {
                    float powerAttackCost = 0.0f;
                    if (actor->GetGraphVariableFloat("Val_PowerAttackCost", powerAttackCost)) {
                        float currentStamina = avOwner->GetActorValue(RE::ActorValue::kStamina);
                        if (currentStamina < powerAttackCost) {
                            bool bypassStaminaCheck = Utils::Random::RollNormalized(0.20f);
                            if (!bypassStaminaCheck) {
                                if (settings::bAdvancedLogging) {
                                    logger::info("AI: Sunk Power Attack '{}' for [{:X}] (Stamina: {:.1f}/{:.1f})", eventStr, actor->GetFormID(), currentStamina, powerAttackCost);
                                }
                                return true;
                            }
                            else if (settings::bAdvancedLogging) {
                                logger::info("AI: [{:X}] bypassed low stamina check for Power Attack '{}'", actor->GetFormID(), eventStr);
                            }
                        }
                    }
                }
            }
        }
        // 6. Light Attack Check
        else if (isLightAttack) {
            bool reactionPending = false;
            if (actor->GetGraphVariableBool("Val_ReactionPending", reactionPending) && reactionPending && settings::bToggleAI) {
                if (settings::bAdvancedLogging) {
                    logger::info("AI: Sunk Light Attack '{}' for [{:X}] (reactionPending=true)", eventStr, actor->GetFormID());
                }
                return true;
            }
            if (settings::bAttackStaminaToggle_NPC && settings::uLightAttackStaminaMode != 0) {
                if (auto avOwner = actor->AsActorValueOwner()) {
                    float lightAttackCost = 0.0f;
                    if (actor->GetGraphVariableFloat("Val_LightAttackCost", lightAttackCost)) {
                        float currentStamina = avOwner->GetActorValue(RE::ActorValue::kStamina);
                        if (currentStamina < lightAttackCost) {
                            bool bypassStaminaCheck = Utils::Random::RollNormalized(0.20f);
                            if (!bypassStaminaCheck) {
                                if (settings::bAdvancedLogging) {
                                    logger::info("AI: Sunk Light Attack '{}' for [{:X}] (Stamina: {:.1f}/{:.1f})", eventStr, actor->GetFormID(), currentStamina, lightAttackCost);
                                }
                                return true;
                            }
                            else if (settings::bAdvancedLogging) {
                                logger::info("AI: [{:X}] bypassed low stamina check for Light Attack '{}'", actor->GetFormID(), eventStr);
                            }
                        }
                    }
                }
            }
        }
    }
    return false;
}

// Cooldown Helpers

bool AI::IsOnCooldown(RE::Actor* a_actor, bool a_isParry)
{
    if (a_actor) {
        return IsOnCooldown(a_actor->GetFormID(), a_isParry);
    }
    return false;
}

void AI::SetCooldown(RE::Actor* a_actor, bool a_isParry)
{
    if (a_actor) {
        SetCooldown(a_actor->GetFormID(), a_isParry);
    }
}

bool AI::IsOnCooldown(RE::FormID a_actorID, bool a_isParry)
{
    std::shared_lock<std::shared_mutex> lock(mtx_aiStates);
    auto it = _aiStates.find(a_actorID);

    if (it != _aiStates.end()) {
        return a_isParry ? (it->second.parryCooldown > 0.0f) : (it->second.blockCooldown > 0.0f);
    }
    return false;
}

void AI::SetCooldown(RE::FormID a_actorID, bool a_isParry)
{
    {
        std::unique_lock<std::shared_mutex> lock(mtx_aiStates);
        auto it = _aiStates.find(a_actorID);

        if (it != _aiStates.end()) {
            if (a_isParry) {
                it->second.parryCooldown = settings::fParryCooldown;
            }
            else {
                it->second.blockCooldown = settings::fBlockCooldown;
            }
        }
        else {
            AIState newState;
            if (a_isParry) {
                newState.parryCooldown = settings::fParryCooldown;
            }
            else {
                newState.blockCooldown = settings::fBlockCooldown;
            }
            _aiStates[a_actorID] = newState;
        }
    }
    ValhallaCombat::GetSingleton()->activateUpdate(ValhallaCombat::aiHandler);
}

// Probability & Multiplier Helpers

float AI::GetSkillLevelMult(RE::Actor* a_actor)
{
    if (!a_actor) {
        return 1.0f;
    }

    // Fetch raw block skill
    float skillLevel = Utils::Actor::GetSkillLevel(a_actor, Utils::Actor::CombatSkill::Block);

    // Clamp between 0 and 100 for interpolation math
    skillLevel = (std::max)(0.0f, (std::min)(100.0f, skillLevel));

    // Normalize skill to a 0.0 to 1.0 range
    float normalizedSkill = skillLevel / 100.0f;

    // Linear interpolation: 0 -> fSkillMin, 100 -> fSkillMax
    float mult = settings::fSkillMin + (normalizedSkill * (settings::fSkillMax - settings::fSkillMin));

    // Apply global skill scaling multiplier around the neutral baseline of 1.0
    float finalMult = 1.0f + ((mult - 1.0f) * settings::fSkillMult);
    return (std::max)(settings::fSkillMin, (std::min)(settings::fSkillMax, finalMult));
}

float AI::GetParryCombatStyleMult(RE::Actor* a_actor)
{
    if (!a_actor) {
        return 1.0f;
    }

    float offensiveMult = Utils::Actor::GetCombatStyleMult(a_actor, Utils::Actor::CombatStyleMult::Offensive);
    float bashMult = Utils::Actor::GetCombatStyleMult(a_actor, Utils::Actor::CombatStyleMult::Bash);

    return (offensiveMult + bashMult) / 2.0f;
}

float AI::GetBlockCombatStyleMult(RE::Actor* a_actor)
{
    if (!a_actor) {
        return 1.0f;
    }
    return Utils::Actor::GetCombatStyleMult(a_actor, Utils::Actor::CombatStyleMult::Defensive);
}

// Roll Logic

bool AI::ShouldAttemptParry(RE::Actor* a_defender, RE::Actor* a_attacker)
{
    if (!a_defender||!a_attacker) {
        return false;
    }

    if (!settings::bParryToggle_NPC) {
        return false;
    }

    if (IsOnCooldown(a_defender, true)) {
        if (settings::bAdvancedLogging) {
            logger::info("AI Parry: [{:X}] is on Parry Cooldown", a_defender->GetFormID());
        }
        return false;
    }

    // Stamina Restriction with 20% Bypass Chance
    if (settings::bAttackStaminaToggle_NPC && settings::bUseCustomLightBashCost) {
        if (auto avOwner = a_defender->AsActorValueOwner()) {
            float lightBashCost = 0.0f;
            if (a_defender->GetGraphVariableFloat("Val_LightBashCost", lightBashCost)) {
                float currentStamina = avOwner->GetActorValue(RE::ActorValue::kStamina);
                if (currentStamina < lightBashCost) {
                    bool bypassStaminaCheck = Utils::Random::RollNormalized(0.20f);
                    if (!bypassStaminaCheck) {
                        if (settings::bAdvancedLogging) {
                            logger::info("AI Parry: [{:X}] failed stamina check ({:.1f}/{:.1f})",
                                a_defender->GetFormID(), currentStamina, lightBashCost);
                        }
                        return false;
                    }
                    else if (settings::bAdvancedLogging) {
                        logger::info("AI Parry: [{:X}] bypassed low stamina check",
                            a_defender->GetFormID());
                    }
                }
            }
        }
    }

    float confidence = Utils::Actor::GetConfidence(a_defender, a_attacker, settings::fConfidenceMult, settings::fConfidenceMin, settings::fConfidenceMax);
    float skillMult = ScaleAroundBaseline(GetSkillLevelMult(a_defender), settings::fParrySkillMult);
    float csMult = ScaleAroundBaseline(GetParryCombatStyleMult(a_defender), settings::fParryCSMult);

    float finalChance = settings::fParryAttemptChance * ((confidence + skillMult + csMult) / 3);
    bool result = Utils::Random::RollNormalized(finalChance);
    
    if (settings::bAdvancedLogging) {
        logger::info("AI Parry [{:X} vs {:X}]: Conf={:.2f}, Skill={:.2f}, CS={:.2f} -> Final Chance={:.1f}% | Result: {}",
            a_defender->GetFormID(), a_attacker->GetFormID(), confidence, skillMult, csMult, finalChance * 100.0f, result ? "SUCCESS" : "FAILED");
    }
    return result;
}

bool AI::ShouldAttemptPerfectBlock(RE::Actor* a_defender, RE::Actor* a_attacker)
{
    if (!a_defender || !a_attacker) {
        return false;
    }

    if (!settings::bTimedBlockToggle_NPC) {
        return false;
    }

    if (IsOnCooldown(a_defender, false)) {
        if (settings::bAdvancedLogging) {
            logger::info("AI Perfect Block: [{:X}] is on Block Cooldown", a_defender->GetFormID());
        }
        return false;
    }

    float confidence = Utils::Actor::GetConfidence(a_defender, a_attacker, settings::fConfidenceMult, settings::fConfidenceMin, settings::fConfidenceMax);
    float skillMult = ScaleAroundBaseline(GetSkillLevelMult(a_defender), settings::fPerfectBlockSkillMult);
    float csMult = ScaleAroundBaseline(GetBlockCombatStyleMult(a_defender), settings::fPerfectBlockCSMult);

    float finalChance = settings::fPerfectBlockAttemptChance * ((confidence + skillMult + csMult) / 3);
    bool result = Utils::Random::RollNormalized(finalChance);
    
    if (settings::bAdvancedLogging) {
        logger::info("AI Perfect Block [{:X} vs {:X}]: Conf={:.2f}, Skill={:.2f}, CS={:.2f} -> Final Chance={:.1f}% | Result: {}",
            a_defender->GetFormID(), a_attacker->GetFormID(), confidence, skillMult, csMult, finalChance * 100.0f, result ? "SUCCESS" : "FAILED");
    }
    return result;
}

bool AI::ShouldAttemptTimedBlock(RE::Actor* a_defender, RE::Actor* a_attacker)
{
    if (!a_defender || !a_attacker) {
        return false;
    }

    if (!settings::bTimedBlockToggle_NPC) {
        return false;
    }

    if (IsOnCooldown(a_defender, false)) {
        if (settings::bAdvancedLogging) {
            logger::info("AI Timed Block: [{:X}] is on Block Cooldown", a_defender->GetFormID());
        }
        return false;
    }

    float confidence = Utils::Actor::GetConfidence(a_defender, a_attacker, settings::fConfidenceMult, settings::fConfidenceMin, settings::fConfidenceMax);
    float skillMult = ScaleAroundBaseline(GetSkillLevelMult(a_defender), settings::fTimedBlockSkillMult);
    float csMult = ScaleAroundBaseline(GetBlockCombatStyleMult(a_defender), settings::fTimedBlockCSMult);

    float finalChance = settings::fTimedBlockAttemptChance * ((confidence + skillMult + csMult) / 3);
    bool result = Utils::Random::RollNormalized(finalChance);

    if (settings::bAdvancedLogging) {
        logger::info("AI Timed Block Roll [{:X} vs {:X}]: Conf={:.2f}, Skill={:.2f}, CS={:.2f} -> Final Chance={:.1f}% | Result: {}",
            a_defender->GetFormID(), a_attacker->GetFormID(), confidence, skillMult, csMult, finalChance * 100.0f, result ? "SUCCESS" : "FAILED");
    }
    return result;
}

// Main Evaluation

void AI::EvaluateAttackReaction(RE::Actor* a_attacker)
{
    if (!a_attacker) {
        return;
    }

    // Check attack type upfront to avoid running checks if not an attack
    bool isPower = Utils::Actor::isPowerAttacking(a_attacker);
    bool isLight = Utils::Actor::isLightAttacking(a_attacker);

    if (!isPower && !isLight) {
        return;
    }

    struct CandidateTarget {
        RE::Actor* actor;
        float distanceSq;
    };

    std::vector<CandidateTarget> validDefenders;
    std::vector<RE::FormID> activeCombatants;

    // 1. Snapshot Phase
    {
        std::shared_lock<std::shared_mutex> lock(mtx_aiStates);
        activeCombatants.reserve(_aiStates.size());
        for (const auto& [actorID, state] : _aiStates) {
            activeCombatants.push_back(actorID);
        }
    }

    // 2. Evaluation Phase (Collect all matching candidates)
    for (RE::FormID actorID : activeCombatants) {
        auto candidate = RE::TESForm::LookupByID<RE::Actor>(actorID);

        // Basic Validity
        if (!candidate || candidate == a_attacker || candidate->IsDead()) {
            continue;
        }

		// Hostility / Combat Target Check :
       if (!candidate->IsCombatTarget(a_attacker)) {
			if (settings::bAdvancedLogging) {
				logger::info("AI Evaluation: Candidate [{:X}] failed -> Attacker [{:X}] is not an active combat target",
					candidate->GetFormID(), a_attacker->GetFormID());
			}
			continue;
		}

        // State Check
        if (!CanDefend(candidate) || !Utils::Actor::canReact(candidate)) {
            if (settings::bAdvancedLogging) {
                logger::info("AI Evaluation: Candidate [{:X}] failed -> Cannot physically defend or react", candidate->GetFormID());
            }
            continue;
        }

        // Distance Check
        if (!Utils::Actor::isInAttackRange(a_attacker, candidate, settings::fbufferDistance)) {
            if (settings::bAdvancedLogging) {
                logger::info("AI Evaluation: Candidate [{:X}] failed -> Out of attack range", candidate->GetFormID());
            }
            continue;
        }

        // Attacker facing the candidate?
        if (!Utils::Actor::isFacingTarget(a_attacker, candidate, settings::fmaxConeDegrees)) {
            if (settings::bAdvancedLogging) {
                logger::info("AI Evaluation: Candidate [{:X}] failed -> Attacker not facing candidate", candidate->GetFormID());
            }
            continue;
        }

        // Candidate facing the attacker? (Block angle)
        if (!Utils::Actor::isInBlockAngle(candidate, a_attacker)) {
            if (settings::bAdvancedLogging) {
                logger::info("AI Evaluation: Candidate [{:X}] failed -> Candidate not facing attacker", candidate->GetFormID());
            }
            continue;
        }

        // Calculate squared distance for sorting
        float distSq = a_attacker->GetPosition().GetSquaredDistance(candidate->GetPosition());
        validDefenders.push_back({ candidate, distSq });

        if (settings::bAdvancedLogging) {
            logger::info("AI Evaluation: Candidate [{:X}] SUCCESS -> Added as valid defender candidate (Dist: {:.1f})",
                candidate->GetFormID(), std::sqrt(distSq));
        }
    }

    // 3. Sort candidates by distance (Closest first)
    std::sort(validDefenders.begin(), validDefenders.end(), [](const CandidateTarget& a, const CandidateTarget& b) {
        return a.distanceSq < b.distanceSq;
        });

    // Clamp configured defender limit between 1 and 5
    int defenderLimit = std::clamp(settings::iMaxSimultaneousDefenders, 1, 5);
    int defendersProcessed = 0;

    // Timing helper lambda
    auto applyJitter = [](float a_delay) {
        float scale = settings::fJitterScale;
        float jitter = (Utils::Random::RollNormalized(1.0f) * 0.2f * scale) - (0.1f * scale);
        return (std::max)(0.0f, a_delay + jitter);
    };

    // 4. Process reactions for closest N defenders
    for (const auto& entry : validDefenders) {
        if (defendersProcessed >= defenderLimit) {
            break;
        }

        RE::Actor* target = entry.actor;

        if (settings::bAdvancedLogging) {
            logger::info("AI Evaluation: Evaluating incoming {} attack from [{:X}] targeting defender [{:X}]",
                isPower ? "Power" : "Light", a_attacker->GetFormID(), target->GetFormID());
        }

        float timeToImpact = AILearning::GetSingleton()->safeGetEstimatedImpactTime(target, a_attacker, isPower);

        if (settings::bAdvancedLogging) {
            logger::info("AI Evaluation: Estimated {} impact time for [{:X}] vs [{:X}]: {:.3f}s",
                isPower ? "Power" : "Light", target->GetFormID(), a_attacker->GetFormID(), timeToImpact);
        }

        //float raiseShieldOffset = settings::bIsBlockingTimedBlock ? 0.15f : 0.0f;
        float minBlockDelay = 0.15f;

        float timedBlockDelay = (std::max)(0.0f, timeToImpact - (settings::fTimedBlockWindow / 2.0f));
        float perfectBlockDelay = (std::max)(0.0f, timeToImpact - (settings::fPerfectBlockWindow / 2.0f));
        float parryDelay = (std::max)(0.0f, timeToImpact - ((settings::fParryWindow_End + settings::fParryWindow_Start) / 2.0f));

        bool canBlockPerfect = !Utils::Actor::isBlocking(target) || (perfectBlockDelay >= minBlockDelay);
		bool canTimedBlock = !Utils::Actor::isBlocking(target) || (timedBlockDelay >= minBlockDelay);

        bool reactionTriggered = false;

        if (ShouldAttemptParry(target, a_attacker)) {
            float finalDelay = (std::max)(0.0f, applyJitter(parryDelay) + settings::fParryDelayOffset);
            if (TriggerParry(target, finalDelay)) {
                if (settings::bAdvancedLogging) {
                    logger::info("AI Decision: [{:X}] selected PARRY against [{:X}] (Delay: {:.3f}s)", target->GetFormID(), a_attacker->GetFormID(), finalDelay);
                }
                target->SetGraphVariableBool("Val_ReactionPending", true);
                SetCooldown(target, true);
                reactionTriggered = true;
            }
        }
        else if (canBlockPerfect && ShouldAttemptPerfectBlock(target, a_attacker)) {
            float finalDelay = (std::max)(0.0f, applyJitter(perfectBlockDelay) + settings::fPerfectBlockDelayOffset);
            if (TriggerTimedBlock(target, finalDelay)) {
                if (settings::bAdvancedLogging) {
                    logger::info("AI Decision: [{:X}] selected PERFECT BLOCK against [{:X}] (Delay: {:.3f}s)", target->GetFormID(), a_attacker->GetFormID(), finalDelay);
                }
				if (Utils::Actor::isBlocking(target)) {
                    target->NotifyAnimationGraph("blockStop");
                }
                target->SetGraphVariableBool("Val_ReactionPending", true);
                SetCooldown(target, false);
                reactionTriggered = true;
            }
        }
        else if (canTimedBlock && ShouldAttemptTimedBlock(target, a_attacker)) {
            float finalDelay = (std::max)(0.0f, applyJitter(timedBlockDelay) + settings::fTimedBlockDelayOffset);
            if (TriggerTimedBlock(target, finalDelay)) {
                if (settings::bAdvancedLogging) {
                    logger::info("AI Decision: [{:X}] selected TIMED BLOCK against [{:X}] (Delay: {:.3f}s)", target->GetFormID(), a_attacker->GetFormID(), finalDelay);
                }
				if (Utils::Actor::isBlocking(target)) {
                    target->NotifyAnimationGraph("blockStop");
                }
                target->SetGraphVariableBool("Val_ReactionPending", true);
                SetCooldown(target, false);
                reactionTriggered = true;
            }
        }

        if (reactionTriggered) {
            defendersProcessed++;
        }
    }
}

bool AI::TriggerParry(RE::Actor* a_defender, float a_delaySeconds)
{
    if (!a_defender) {
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(mtx_pendingReactions);
        auto it = std::find_if(_pendingReactions.begin(), _pendingReactions.end(), [actorID = a_defender->GetFormID()](const PendingAIReaction& r) {
            return r.actorID == actorID;
            });
        if (it != _pendingReactions.end()) {
            return false; // Already queued
        }
        _pendingReactions.push_back({ a_defender->GetFormID(), AIActionType::Parry, a_delaySeconds });
    }
    ValhallaCombat::GetSingleton()->activateUpdate(ValhallaCombat::aiHandler);
    return true;
}

bool AI::TriggerTimedBlock(RE::Actor* a_defender, float a_delaySeconds)
{
    if (!a_defender) {
        return false;
    }
    {
        std::lock_guard<std::mutex> lock(mtx_pendingReactions);
        auto it = std::find_if(_pendingReactions.begin(), _pendingReactions.end(), [actorID = a_defender->GetFormID()](const PendingAIReaction& r) {
            return r.actorID == actorID;
            });
        if (it != _pendingReactions.end()) {
            return false; // Already queued
        }
        _pendingReactions.push_back({ a_defender->GetFormID(), AIActionType::TimedBlock, a_delaySeconds });
    }
    ValhallaCombat::GetSingleton()->activateUpdate(ValhallaCombat::aiHandler);
    return true;
}

void AI::update()
{
    float deltaTime = *RE::Offset::g_deltaTime;
    bool keepActive = false;

    // 1. Tick Cooldowns
    {
        std::unique_lock<std::shared_mutex> lock(mtx_aiStates);
        for (auto& [actorID, state] : _aiStates) {
            if (state.parryCooldown > 0.0f) {
                state.parryCooldown -= deltaTime;
                keepActive = true;
            }
            if (state.blockCooldown > 0.0f) {
                state.blockCooldown -= deltaTime;
                keepActive = true;
            }
        }
    }

    // 2. Process Pending Reactions
    struct ReadyAction {
        RE::FormID actorID;
        AIActionType action;
    };

    std::vector<ReadyAction> readyActions;

    {
        std::lock_guard<std::mutex> lock(mtx_pendingReactions);
        for (auto it = _pendingReactions.begin(); it != _pendingReactions.end(); ) {
            it->delayRemaining -= deltaTime;

            if (it->delayRemaining <= 0.0f) {
                readyActions.push_back({ it->actorID, it->action });
                it = _pendingReactions.erase(it);
            }
            else {
                keepActive = true;
                ++it;
            }
        }
    }

    // 3. Lifecycle Management
    if (!keepActive) {
        ValhallaCombat::GetSingleton()->deactivateUpdate(ValhallaCombat::aiHandler);
        if (readyActions.empty()) {
            return;
        }
    }

    // 4. Execution Phase
    for (const auto& ready : readyActions) {
        if (auto defender = RE::TESForm::LookupByID<RE::Actor>(ready.actorID)) {
            defender->SetGraphVariableBool("Val_ReactionPending", false);
            if (!defender->IsDead() && Utils::Actor::canReact(defender)) {
                if (ready.action == AIActionType::Parry) {
                    if (settings::bAdvancedLogging) {
                        logger::info("AI: Executing queued PARRY for [{:X}]", ready.actorID);
                    }
                    defender->SetGraphVariableBool("Val_BashAllowed", true);
                    // Manually assign Bash AttackData
                    if (auto currentProcess = defender->GetActorRuntimeData().currentProcess) {
                        if (currentProcess->high) {
                            if (auto race = defender->GetRace(); race && race->attackDataMap) {
                                auto& dataMap = race->attackDataMap->attackDataMap;

                                // Look up the exact event string shown in the Race record
                                RE::BSFixedString bashEvent("bashStart");
                                auto it = dataMap.find(bashEvent);

                                // Inject the AttackData pointer into the actor's HighProcess
                                if (it != dataMap.end() && it->second) {
                                    currentProcess->high->attackData = it->second;

                                    if (settings::bAdvancedLogging) {
                                        logger::info("AI: Successfully injected 'bashStart' attack data for [{:X}]", ready.actorID);
                                    }
                                }
                                else if (settings::bAdvancedLogging) {
                                    logger::info("AI: Failed to find 'bashStart' attack data for [{:X}]'s race", ready.actorID);
                                }
                            }
                        }
                    }
                    defender->NotifyAnimationGraph("bashStart");
                }
                else if (ready.action == AIActionType::TimedBlock) {
                    if (settings::bAdvancedLogging) {
                        logger::info("AI: Executing queued TIMED BLOCK/PERFECT BLOCK for [{:X}]", ready.actorID);
                    }
                    defender->NotifyAnimationGraph("blockStart");
                }
            }
            else if (settings::bAdvancedLogging) {
                logger::info("AI: Executing queued defense action failed for [{:X}]", ready.actorID);
            }
        }
    }
}

// Cleanup Methods

void AI::onActorDeath(RE::Actor* a_actor)
{
    if (!a_actor) {
        return;
    }
    a_actor->SetGraphVariableBool("Val_ReactionPending", false);
    const RE::FormID actorID = a_actor->GetFormID();

    {
        std::unique_lock<std::shared_mutex> lock(mtx_aiStates);
        _aiStates.erase(actorID);
    }

    {
        std::lock_guard<std::mutex> lock(mtx_pendingReactions);

        std::erase_if(_pendingReactions, [actorID](const PendingAIReaction& reaction) {
            return reaction.actorID == actorID;
        });
    }

    AILearning::GetSingleton()->safeClearProfile(a_actor);
}

void AI::reset()
{
    {
        std::unique_lock<std::shared_mutex> lock(mtx_aiStates);
        _aiStates.clear();
    }
    {
        std::lock_guard<std::mutex> lockReactions(mtx_pendingReactions);
        _pendingReactions.clear();
    }
}
