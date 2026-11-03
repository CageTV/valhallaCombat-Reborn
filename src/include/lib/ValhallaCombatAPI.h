#pragma once
#include <functional>
#include <windows.h>
#include <stdint.h>
/*
* For modders: Copy this file into your own project if you wish to use this API
*/
namespace VAL_API
{
	constexpr const auto ValhallaCombatPluginName = "ValhallaCombat";

	using PluginHandle = SKSE::PluginHandle;
	using ActorHandle = RE::ActorHandle;

	enum class InterfaceVersion : uint8_t
	{
		V1,
		V2,
		V3
	};

	enum STUNSOURCE
	{
		lightAttack,
		powerAttack,
		bash,
		powerBash,
		timedBlock,
		parry,
		counterAttack
	};

	// Valhalla Combat's modder interface
	class IVVAL1
	{
	public:
		virtual void processStunDamage(STUNSOURCE stunSource, RE::TESObjectWEAP* weapon, RE::Actor* aggressor, RE::Actor* victim, float baseDamage) noexcept = 0;
	};

	class IVVAL2 : public IVVAL1
	{
	public:
		virtual bool getIsTimedBlocking(RE::Actor* a_actor) noexcept = 0;
		virtual bool getIsPerfectBlocking(RE::Actor* a_actor) noexcept = 0;
		virtual void triggerTimedBlockSuccess(RE::Actor* a_actor) noexcept = 0;

		virtual bool isActorStunned(RE::Actor* a_actor) noexcept = 0;
		virtual bool isActorExhausted(RE::Actor* a_actor) noexcept = 0;
	};

	class IVVAL3 : public IVVAL2
	{
	public:
		virtual bool isEligibleForExecution(RE::Actor* a_actor) noexcept = 0;
		virtual bool getIsTackling(RE::Actor* a_actor) noexcept = 0;
		virtual bool inParryState(RE::Actor* a_actor) noexcept = 0;
		virtual void applyMovementRestrictions(RE::Actor* a_actor) noexcept = 0;
		virtual void clearMovementRestrictions(RE::Actor* a_actor) noexcept = 0;
		virtual void initStaminaDebuff(RE::Actor* a_actor) noexcept = 0;
		virtual void stopDebuff(RE::Actor* a_actor) noexcept = 0;
	};


	typedef void* (*_RequestPluginAPI)(const InterfaceVersion interfaceVersion);

	/// <summary>
	/// Request the Valhalla Combat API interface.
	/// Recommended: Send your request during or after SKSEMessagingInterface::kMessage_PostLoad to make sure the dll has already been loaded
	/// </summary>
	/// <param name="a_interfaceVersion">The interface version to request</param>
	/// <returns>The pointer to the API singleton, or nullptr if request failed</returns>
	[[nodiscard]] inline void* RequestPluginAPI(const InterfaceVersion a_interfaceVersion = InterfaceVersion::V3)
	{
		auto pluginHandle = GetModuleHandle("ValhallaCombat.dll");
		_RequestPluginAPI requestAPIFunction = (_RequestPluginAPI)GetProcAddress(pluginHandle, "RequestPluginAPI");
		if (requestAPIFunction) {
			return requestAPIFunction(a_interfaceVersion);
		}
		return nullptr;
	}
}
