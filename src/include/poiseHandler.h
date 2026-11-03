#pragma once
#include "lib/robin_hood.h"
#include <shared_mutex>
#include <memory>
#include <vector>

class poiseHandler {
private:
    struct actorPoiseData {
        mutable std::mutex dataLock;
        float maxPoise;
        float currentPoise;
        float regenDelayTimer;
        float staggerProtectTimer;

        actorPoiseData(float a_maxPoise) {
            maxPoise = a_maxPoise;
            currentPoise = maxPoise;
            regenDelayTimer = 0.0f;
            staggerProtectTimer = 0.0f;
        }
    };

    using actorPoiseData_ptr = std::shared_ptr<actorPoiseData>;
    robin_hood::unordered_map<RE::FormID, actorPoiseData_ptr> actorPoiseDataMap;
    mutable std::shared_mutex mtx_ActorPoiseDataMap;

    actorPoiseData_ptr safeInsert_ActorPoiseData(RE::Actor* a_actor);
    actorPoiseData_ptr safeGet_ActorPoiseData(RE::Actor* a_actor);

public:
    static poiseHandler* GetSingleton() {
        static poiseHandler singleton;
        return std::addressof(singleton);
    }

    void update();
    void reset();
    void onActorDeath(RE::Actor* a_actor);

    // Member Accessors
    float getCurrentPoise(RE::Actor* a_actor);
    bool setCurrentPoise(RE::Actor* a_actor, float a_val);
    float getTotalPoise(RE::Actor* a_actor);

    float getWeaponPoiseDamage(RE::Actor* a_aggressor, RE::Actor* a_target, const RE::HitData* a_hitData);
    float getMagicPoiseDamage(RE::Actor* a_target, float a_staggerMult, RE::Actor* a_aggressor);

    float getRegenDelay(RE::Actor* a_actor);
    bool setRegenDelay(RE::Actor* a_actor, float a_val);

    float getStaggerProtect(RE::Actor* a_actor);
    bool setStaggerProtect(RE::Actor* a_actor, float a_val);

    // Static Wrappers for TrueHUD Integration
    static float getCurrentPoise_static(RE::Actor* a_actor);
    static float getTotalPoise_static(RE::Actor* a_actor);
};