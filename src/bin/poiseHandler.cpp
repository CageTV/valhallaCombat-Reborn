#include "include/poiseHandler.h"
#include "include/PoiseHealthHandler.h"
#include "include/PoiseDamageCalculator.h"
#include "include/Utils.h"
#include "include/settings.h"
#include "include/offsets.h"
#include "ValhallaCombat.hpp"

using uniqueLocker = std::unique_lock<std::shared_mutex>;
using sharedLocker = std::shared_lock<std::shared_mutex>;

poiseHandler::actorPoiseData_ptr poiseHandler::safeInsert_ActorPoiseData(RE::Actor* a_actor) {
    if (!a_actor) {
        return nullptr;
    }

    float initialPoise = PoiseHealthHandler::GetTotalPoiseHealth(a_actor);

    RE::FormID formID = a_actor->GetFormID();
    uniqueLocker lock(mtx_ActorPoiseDataMap);
    auto it = actorPoiseDataMap.find(formID);
    if (it != actorPoiseDataMap.end()) {
        return it->second;
    }
    actorPoiseData_ptr dataPtr(new actorPoiseData(initialPoise));
    actorPoiseDataMap.insert({ formID, dataPtr });

    ValhallaCombat::GetSingleton()->activateUpdate(ValhallaCombat::poiseHandler);

    return dataPtr;
}

poiseHandler::actorPoiseData_ptr poiseHandler::safeGet_ActorPoiseData(RE::Actor* a_actor) {
    if (!a_actor) {
        return nullptr;
    }

    sharedLocker lock(mtx_ActorPoiseDataMap);
    auto it = actorPoiseDataMap.find(a_actor->GetFormID());

    return (it != actorPoiseDataMap.end()) ? it->second : nullptr;
}

void poiseHandler::update() {
    float delta = *RE::Offset::g_deltaTime;

    std::vector<std::pair<RE::FormID, actorPoiseData_ptr>> snapshot;
    {
        uniqueLocker mapLock(mtx_ActorPoiseDataMap);

        // Deactivate update loop if map is empty to save performance
        if (actorPoiseDataMap.empty()) {
            ValhallaCombat::GetSingleton()->deactivateUpdate(ValhallaCombat::poiseHandler);
            return;
        }

        for (const auto& [formID, dataPtr] : actorPoiseDataMap) {
            snapshot.push_back({ formID, dataPtr });
        }
    }

    std::vector<RE::FormID> toRemove;

    for (auto& [formID, data] : snapshot) {
        auto actor = RE::TESForm::LookupByID<RE::Actor>(formID);

        if (!actor || !actor->Is3DLoaded() || !actor->IsInitialized() || actor->IsDead()) {
            toRemove.push_back(formID);
            continue;
        }

        bool isTargetStaggering = Utils::Actor::isStaggering(actor);
        bool bTrueHUDRevert = false;
        bool bFullyHealed = false;

        {
            std::lock_guard<std::mutex> dataLock(data->dataLock);

            // Periodically sync maxPoise in case armor/scaling changes mid-combat
            //data->maxPoise = PoiseHealthHandler::GetTotalPoiseHealth(actor);

            if (!actor->IsInCombat() || (data->currentPoise - data->maxPoise) >= 0.01f) {
                data->currentPoise = data->maxPoise;
            }
            else if (!isTargetStaggering) {
                if (data->regenDelayTimer > 0.0f) {
                    data->regenDelayTimer -= delta;
                }
                else {
                    data->currentPoise += data->maxPoise * delta * settings::fPoise_PoiseRegen;
                }

                if (data->staggerProtectTimer > 0.0f) {
                    data->staggerProtectTimer -= delta;
                    if (data->staggerProtectTimer <= 0.0f) {
                        bTrueHUDRevert = true;
                    }
                }
            }

            if (data->currentPoise >= data->maxPoise && data->staggerProtectTimer <= 0.0f) {
                bFullyHealed = true;
            }
        }

        if (bTrueHUDRevert && settings::facts::TrueHudAPI_PoiseControlsSpecialBar) {
            TrueHUDUtils::revertSpecialMeter(actor);
        }

        if (bFullyHealed) {
            toRemove.push_back(formID);
        }
    }

    if (!toRemove.empty()) {
        uniqueLocker mapLock(mtx_ActorPoiseDataMap);
        for (auto formID : toRemove) {
            actorPoiseDataMap.erase(formID);
        }
    }
}

// Data Accessors

float poiseHandler::getCurrentPoise(RE::Actor* a_actor) {
    if (auto data = safeGet_ActorPoiseData(a_actor)) {
        std::lock_guard<std::mutex> lock(data->dataLock);
        return data->currentPoise;
    }
    return PoiseHealthHandler::GetTotalPoiseHealth(a_actor);
}

bool poiseHandler::setCurrentPoise(RE::Actor* a_actor, float a_val) {
    auto data = safeGet_ActorPoiseData(a_actor);
    if (!data) {
        data = safeInsert_ActorPoiseData(a_actor);
    }
    std::lock_guard<std::mutex> lock(data->dataLock);
    data->currentPoise = a_val;

    return true;
}

float poiseHandler::getTotalPoise(RE::Actor* a_actor) {
    if (auto data = safeGet_ActorPoiseData(a_actor)) {
        std::lock_guard<std::mutex> lock(data->dataLock);
        return data->maxPoise;
    }
    // Fallback for untracked actors on their first hit
    return PoiseHealthHandler::GetTotalPoiseHealth(a_actor);
}

float poiseHandler::getWeaponPoiseDamage(RE::Actor* a_aggressor, RE::Actor* a_target, const RE::HitData* a_hitData) {
    return PoiseDamageCalculator::GetWeaponPoiseDamage(a_aggressor, a_target, a_hitData);
}

float poiseHandler::getMagicPoiseDamage(RE::Actor* a_target, float a_staggerMult, RE::Actor* a_aggressor) {
    return PoiseDamageCalculator::GetMagicPoiseDamage(a_target, a_staggerMult, a_aggressor);
}

float poiseHandler::getRegenDelay(RE::Actor* a_actor) {
    if (auto data = safeGet_ActorPoiseData(a_actor)) {
        std::lock_guard<std::mutex> lock(data->dataLock);
        return data->regenDelayTimer;
    }
    return 0.0f;
}

bool poiseHandler::setRegenDelay(RE::Actor* a_actor, float a_val) {
    auto data = safeGet_ActorPoiseData(a_actor);
    if (!data) {
        data = safeInsert_ActorPoiseData(a_actor);
    }
    std::lock_guard<std::mutex> lock(data->dataLock);
    data->regenDelayTimer = a_val;
    return true;
}

float poiseHandler::getStaggerProtect(RE::Actor* a_actor) {
    if (auto data = safeGet_ActorPoiseData(a_actor)) {
        std::lock_guard<std::mutex> lock(data->dataLock);
        return data->staggerProtectTimer;
    }
    return 0.0f;
}

bool poiseHandler::setStaggerProtect(RE::Actor* a_actor, float a_val) {
    auto data = safeGet_ActorPoiseData(a_actor);
    if (!data) {
        data = safeInsert_ActorPoiseData(a_actor);
    }
    std::lock_guard<std::mutex> lock(data->dataLock);
    data->staggerProtectTimer = a_val;
    return true;
}

float poiseHandler::getCurrentPoise_static(RE::Actor* a_actor) {
    return GetSingleton()->getCurrentPoise(a_actor);
}

float poiseHandler::getTotalPoise_static(RE::Actor* a_actor) {
    return GetSingleton()->getTotalPoise(a_actor);
}

// Cleanup Methods

void poiseHandler::onActorDeath(RE::Actor* a_actor) {
    if (!a_actor) {
        return;
    }
    uniqueLocker lock(mtx_ActorPoiseDataMap);
    actorPoiseDataMap.erase(a_actor->GetFormID());
}

void poiseHandler::reset() {
    uniqueLocker lock(mtx_ActorPoiseDataMap);
    actorPoiseDataMap.clear();
}