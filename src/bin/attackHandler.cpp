#include "include/staminaHandler.h"
#include "include/attackHandler.h"
#include "include/utils.h"

std::string_view attackHandler::GetAttackTypeString(const AttackContext& a_ctx)
{
    if (a_ctx.isPowerBash) {
        return "Power Bash";
    }
    if (a_ctx.isBash) {
        return "Bash";
    }
    if (a_ctx.isPower) {
        return "Power Attack";
    }
    if (a_ctx.isLight) {
        return "Light Attack";
    }
    return "Unknown";
}

void attackHandler::cacheAttack(RE::Actor* a_actor)
{
    if (!a_actor) {
        return;
    }

    //a_actor->SetGraphVariableFloat("Val_StaminaMult", 1.0f);

    auto* process = a_actor->GetActorRuntimeData().currentProcess;

    if (!process || !process->high || !process->high->attackData) {
        return;
    }

    //auto atk = process->high->attackData;
    auto* atk = process->high->attackData.get();

    AttackContext ctx;

    ctx.isPowerBash = atk->data.flags.any(RE::AttackData::AttackFlag::kPowerAttack) && atk->data.flags.any(RE::AttackData::AttackFlag::kBashAttack);
    ctx.isBash = atk->data.flags.any(RE::AttackData::AttackFlag::kBashAttack) && !ctx.isPowerBash;
    ctx.isPower = atk->data.flags.any(RE::AttackData::AttackFlag::kPowerAttack) && !ctx.isPowerBash;
    ctx.isLight = !ctx.isBash && !ctx.isPower && !ctx.isPowerBash;

    ctx.hitFrameCount = 0;

    std::scoped_lock lock(mtx);
    attackContexts[a_actor->GetFormID()] = ctx;

    if (settings::bAdvancedLogging) {
        logger::info("Successfully cached attack ({}) for actor: {:X}", GetAttackTypeString(ctx), a_actor->GetFormID());
    }
}

void attackHandler::OnLightHit(RE::Actor* a_actor)
{
    if (!a_actor)
        return;

    if (!a_actor->IsPlayerRef() && !settings::bAttackStaminaToggle_NPC) {
        return;
    }

    std::scoped_lock lock(mtx);

    auto it = attackContexts.find(a_actor->GetFormID());
    if (it == attackContexts.end()) {
        return;
    }

    auto& ctx = it->second;

    // Only light attacks should ever get stamina rewards
    if (!ctx.isLight) {
        return;
    }

    ctx.successfulHitCount++;

    if (settings::uLightAttackStaminaMode == settings::Legacy)
    {
        staminaHandler::staminaLightAttackReward(a_actor, ctx.successfulHitCount);
        attackContexts.erase(it);
    }
    else if (settings::uLightAttackStaminaMode == settings::HitFrame)
    {
        staminaHandler::staminaLightAttackReward(a_actor, ctx.successfulHitCount);
    }
}

void attackHandler::checkout(RE::Actor* a_actor)
{
    if (!a_actor)
        return;

    a_actor->SetGraphVariableFloat("Val_StaminaMult", 1.0f);
    a_actor->SetGraphVariableFloat("Val_PoiseMult", 1.0f);
    a_actor->SetGraphVariableBool("Val_PoiseBreaksBlock", false);
	a_actor->SetGraphVariableBool("Val_Unblockable", false);

    std::scoped_lock lock(mtx);

    auto it = attackContexts.find(a_actor->GetFormID());

    if (it == attackContexts.end()) {
        return;
    }

    auto& ctx = it->second;

    if (settings::uLightAttackStaminaMode == settings::Legacy) {

        if (ctx.isLight) {
            staminaHandler::staminaLightAttackMiss(a_actor);
        }
    }

    if (settings::bAdvancedLogging) {
        logger::info("Successfully checked out attack ({}) for actor: {:X}", GetAttackTypeString(ctx), a_actor->GetFormID());
    }
    attackContexts.erase(it);
}

void attackHandler::onLightAttackHitFrame(RE::Actor* a_actor)
{
    if (!a_actor)
        return;

    if (!a_actor->IsPlayerRef() && !settings::bAttackStaminaToggle_NPC)
        return;

    if (settings::uLightAttackStaminaMode != settings::HitFrame)
        return;

    std::scoped_lock lock(mtx);

    auto it = attackContexts.find(a_actor->GetFormID());
    if (it == attackContexts.end())
        return;

    auto& ctx = it->second;

    if (!ctx.isLight)
        return;

    ctx.hitFrameCount++;

    staminaHandler::staminaLightAttackHitFrameCost(a_actor, ctx.hitFrameCount);
}

void attackHandler::onPowerAttackHitFrame(RE::Actor* a_actor)
{
    if (!a_actor)
        return;

    if (!a_actor->IsPlayerRef() && !settings::bAttackStaminaToggle_NPC)
        return;

    if (!settings::bUseCustomPowerAttackCost)
        return;

    std::scoped_lock lock(mtx);

    auto it = attackContexts.find(a_actor->GetFormID());
    if (it == attackContexts.end())
        return;

    auto& ctx = it->second;

    if (!ctx.isPower)
        return;

    ctx.hitFrameCount++;

    staminaHandler::staminaPowerAttackHitFrameCost(a_actor, ctx.hitFrameCount);
}

void attackHandler::onPowerBashHitFrame(RE::Actor* a_actor)
{
    if (!a_actor)
        return;

    if (!a_actor->IsPlayerRef() && !settings::bAttackStaminaToggle_NPC)
        return;

    if (!settings::bUseCustomPowerBashCost)
        return;

    std::scoped_lock lock(mtx);

    auto it = attackContexts.find(a_actor->GetFormID());
    if (it == attackContexts.end())
        return;

    auto& ctx = it->second;

    if (!ctx.isPowerBash)
        return;

    ctx.hitFrameCount++;

    staminaHandler::staminaPowerBashHitFrameCost(a_actor, ctx.hitFrameCount);
}

void attackHandler::onLightBashHitFrame(RE::Actor* a_actor)
{
    if (!a_actor)
        return;

    if (!a_actor->IsPlayerRef() && !settings::bAttackStaminaToggle_NPC)
        return;

    if (!settings::bUseCustomLightBashCost)
        return;

    std::scoped_lock lock(mtx);

    auto it = attackContexts.find(a_actor->GetFormID());
    if (it == attackContexts.end())
        return;

    auto& ctx = it->second;

    if (!ctx.isBash)
        return;

    ctx.hitFrameCount++;

    staminaHandler::staminaLightBashHitFrameCost(a_actor, ctx.hitFrameCount);
}

void attackHandler::onActorDeath(RE::Actor* a_actor)
{
    if (!a_actor) {
        return;
    }

    std::scoped_lock lock(mtx);
    attackContexts.erase(a_actor->GetFormID());
}

void attackHandler::reset()
{
    std::scoped_lock lock(mtx);
    attackContexts.clear();
}
