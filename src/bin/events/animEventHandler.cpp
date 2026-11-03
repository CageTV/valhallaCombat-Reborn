#include "include/events.h"
#include "include/attackHandler.h"
#include "include/staminaHandler.h"
#include "include/blockHandler.h"
#include "include/parryHandler.h"
#include "include/executionHandler.h"
#include "include/settings.h"
#include "include/Utils.h"
#include "include/data.h"
#include "include/AILearning.h"
#include "include/specialAttackHandler.h"

// all credits to Bingle & Ersh
constexpr uint32_t hash(const char* data, size_t size) noexcept
{
    uint32_t h = 5381;
    for (const char* c = data; c < data + size; ++c) {
        h = ((h << 5) + h) + static_cast<uint8_t>(*c);
    }
    return h;
}

constexpr uint32_t operator"" _h(const char* str, size_t size) noexcept
{
    return hash(str, size);
}

void animEventHandler::ProcessEvent(
    RE::BSTEventSink<RE::BSAnimationGraphEvent>*,
    RE::BSAnimationGraphEvent* a_event,
    RE::BSTEventSource<RE::BSAnimationGraphEvent>*)
{
    if (!a_event || !a_event->holder) {
        return;
    }

    auto* actor = const_cast<RE::TESObjectREFR*>(a_event->holder)->As<RE::Actor>();
    if (!actor) {
        return;
    }

    // 1. Fetch raw strings
    std::string tagStr = a_event->tag.c_str();
    std::string payloadStr = a_event->payload.c_str();

    // 2. Extract embedded payload
    if (auto pos = tagStr.find_first_of(". "); pos != std::string::npos) {
        if (payloadStr.empty()) {
            payloadStr = tagStr.substr(pos + 1);
        }
        tagStr = tagStr.substr(0, pos);
    }

    // 3. Lowercase 
    std::string lowerTag = inlineUtils::to_lower_string(tagStr);
    std::string lowerPayload = inlineUtils::to_lower_string(payloadStr);

    // 4. Hash the lowercase tag
    uint32_t tagHash = hash(lowerTag.data(), lowerTag.size());

    // 5. Route to SpecialAttackHandler
    switch (tagHash) {
    case "val_specialatkstart"_h:
    case "val_specialatkstop"_h:
    case "val_hyperarmorstart"_h:
    case "val_hyperarmorstop"_h:
	case "val_invulstart"_h:
	case "val_invulstop"_h:
	case "val_actiononhitstart"_h:
	case "val_actiononhitstop"_h:
	case "sbf_normalattackstart"_h:
	case "sbf_powerattackstart"_h:
    case "attackstop"_h:
    case "bashstop"_h:
    case "mco_attackexitnotify"_h:
    case "mco_atkclose"_h:
        SpecialAttackHandler::GetSingleton()->processAnimEvent(actor, lowerTag, lowerPayload);
        break;
    default:
        break;
    }

    // 6. Dynamic animation event data map lookup
    if (!lowerTag.empty()) {
        std::string fullKey = !lowerPayload.empty() ? lowerTag + "." + lowerPayload : lowerTag;

        // 1. Try "tag.payload" (or "tag" if payload is empty)
        auto it = settings::animEventDataMap.find(fullKey);

        // 2. Fallback to base "tag" if "tag.payload" wasn't mapped
        if (it == settings::animEventDataMap.end() && !lowerPayload.empty()) {
            it = settings::animEventDataMap.find(lowerTag);
        }

        if (it != settings::animEventDataMap.end()) {
            actor->SetGraphVariableBool("Val_PoiseBreaksBlock", it->second.bPoiseBreaksBlock);
			actor->SetGraphVariableBool("Val_Unblockable", it->second.bUnblockable);
            actor->SetGraphVariableFloat("Val_StaminaMult", it->second.fStaminaMult);
            actor->SetGraphVariableFloat("Val_PoiseMult", it->second.fPoiseMult);

            if (settings::bAdvancedLogging) {
                logger::info("Matched event [{}] on Actor [{:X}]", it->first, actor->GetFormID());
            }
        }
    }

    // 7. Engine Event Switches
    switch (tagHash) {

    case "weapequip_out"_h:
    {
        float lightCost = staminaHandler::getLightAttackCost(actor) * settings::fGraph_LightAttackCostMult;
        float powerCost = staminaHandler::getPowerAttackCost(actor) * settings::fGraph_PowerAttackCostMult;
        float lightBashCost = staminaHandler::getLightBashCost(actor) * settings::fGraph_LightBashCostMult;
        float powerBashCost = staminaHandler::getPowerBashCost(actor) * settings::fGraph_PowerBashCostMult;

        actor->SetGraphVariableFloat("Val_LightAttackCost", lightCost);
        actor->SetGraphVariableFloat("Val_PowerAttackCost", powerCost);
        actor->SetGraphVariableFloat("Val_LightBashCost", lightBashCost);
        actor->SetGraphVariableFloat("Val_PowerBashCost", powerBashCost);

        if (settings::bAdvancedLogging) {
            logger::info("[Equip Event] Actor [{:X}] -> Set Graph Variables | Light: {:.2f} | Power: {:.2f} | LightBash: {:.2f} | PowerBash: {:.2f}",
                actor->GetFormID(), lightCost, powerCost, lightBashCost, powerBashCost);
        }
    }
    break;

    case "prehitframe"_h:
        attackHandler::GetSingleton()->cacheAttack(actor);

        if (settings::bStaminaDebuffToggle) {
            staminaHandler::checkStamina(actor);
        }
        break;

    case "fakehit"_h:
    case "hitframe"_h:
        if (settings::uLightAttackStaminaMode == settings::HitFrame) {
            attackHandler::GetSingleton()->onLightAttackHitFrame(actor);
        }
        if (settings::bUseCustomPowerAttackCost) {
            attackHandler::GetSingleton()->onPowerAttackHitFrame(actor);
        }
		if (settings::bUseCustomLightBashCost && !settings::bSuccessfulParryNoCost) {
            attackHandler::GetSingleton()->onLightBashHitFrame(actor);
        }
        if (settings::bUseCustomPowerBashCost) {
            attackHandler::GetSingleton()->onPowerBashHitFrame(actor);
        }
        break;

    case "bashstop"_h:
        if (settings::bParryToggle) {
            parryHandler::GetSingleton()->finishTimingParry(actor);
        }
        if (settings::bToggleAI) {
            actor->SetGraphVariableBool("Val_BashAllowed", false);
        }
        [[fallthrough]];
    case "mco_attackexitnotify"_h:
    case "mco_atkclose"_h:
	case "sbf_normalattackstart"_h:
	case "sbf_powerattackstart"_h:
    case "attackstop"_h:
        attackHandler::GetSingleton()->checkout(actor);

        if (settings::bStaminaDebuffToggle) {
            staminaHandler::checkStamina(actor);
        }

        Utils::Actor::safeRemoveSpell(data::followupSpell, actor);
        Utils::Actor::safeRemoveSpell(data::parryFollowupSpell, actor);
        Utils::Actor::safeRemoveSpell(data::parrySASpell, actor);
        break;

    case "tkdr_iframeend"_h:
    case "dodge"_h:
        staminaHandler::checkStamina(actor);
        break;

    default:
        break;
    }
}

EventResult animEventHandler::ProcessEvent_NPC(
    RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink,
    RE::BSAnimationGraphEvent* a_event,
    RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource)
{
    ProcessEvent(a_sink, a_event, a_eventSource);
    return _ProcessEvent_NPC(a_sink, a_event, a_eventSource);
}

EventResult animEventHandler::ProcessEvent_PC(
    RE::BSTEventSink<RE::BSAnimationGraphEvent>* a_sink,
    RE::BSAnimationGraphEvent* a_event,
    RE::BSTEventSource<RE::BSAnimationGraphEvent>* a_eventSource)
{
    ProcessEvent(a_sink, a_event, a_eventSource);
    return _ProcessEvent_PC(a_sink, a_event, a_eventSource);
}
