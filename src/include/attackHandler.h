#pragma once
#include <mutex>
#include "lib/robin_hood.h"


/*Handle attack registration, stamina consumption, and stamina reward.*/
class attackHandler
{
private:
    std::mutex mtx;

    struct AttackContext
    {
        bool isLight = false;
        bool isPower = false;
        bool isBash = false;
        bool isPowerBash = false;

        uint32_t hitFrameCount = 0;
        uint32_t successfulHitCount = 0;
    };

    static std::string_view GetAttackTypeString(const AttackContext& a_ctx);
    robin_hood::unordered_map<RE::FormID, AttackContext> attackContexts;

public:
    static attackHandler* GetSingleton()
    {
        static attackHandler singleton;
        return std::addressof(singleton);
    }

    void cacheAttack(RE::Actor* a_actor);
    void OnLightHit(RE::Actor* a_actor);
    void checkout(RE::Actor* a_actor);
    void reset();

    void onLightAttackHitFrame(RE::Actor* a_actor);
    void onPowerAttackHitFrame(RE::Actor* a_actor);
    void onLightBashHitFrame(RE::Actor* a_actor);
    void onPowerBashHitFrame(RE::Actor* a_actor);
    void onActorDeath(RE::Actor* a_actor);
};