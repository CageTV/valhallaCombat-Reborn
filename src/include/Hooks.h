#pragma once

#include "SKSE/Trampoline.h"
#include "include/Utils.h"
namespace Hooks
{
	class Hook_OnGetAttackStaminaCost  //Actor__sub_140627930+16E	call ActorValueOwner__sub_1403BEC90
	{
		/*to cancel out vanilla power attack stamina consumption.*/
	public:
		static void install()
		{
			auto& trampoline = SKSE::GetTrampoline();

			REL::Relocation<uintptr_t> hook{ RELOCATION_ID(37650, 38603) };  //SE:627930 + 16E => 3BEC90 AE:64D350 + 171 => 3D6720

			_getAttackStaminaCost = trampoline.write_call<5>(hook.address() + RELOCATION_OFFSET(0x16E, 0x171), getAttackStaminaCost);
			
			logger::info("hook:OnGetAttackStaminaCost");
		}

	private:
		static float getAttackStaminaCost(RE::ActorValueOwner* avOwner, RE::BGSAttackData* atkData);
		static inline REL::Relocation<decltype(getAttackStaminaCost)> _getAttackStaminaCost;
	};

	class Hook_OnRestoreActorValue
	{
	public:
		static void install()
		{
			REL::Relocation<uintptr_t> hook{ RELOCATION_ID(37510, 38452) };// 140620690       140645AA0
			auto& trampoline = SKSE::GetTrampoline();
			_RestoreActorValue = trampoline.write_call<5>(hook.address() + RELOCATION_OFFSET(0x176, 0xE1), RestoreActorValue);
			logger::info("hook:OnRestoresActorValue");
		}

	private:
		static void RestoreActorValue(RE::Actor* a, RE::ActorValue av, float val);
		static inline REL::Relocation<decltype(RestoreActorValue)> _RestoreActorValue; //140620900		140645d30
	};

	class Hook_OnMeleeHit
	{
	public:
		static void install()
		{
			REL::Relocation<uintptr_t> hook{ RELOCATION_ID(37673, 38627) };  //140628C20       14064E760
			auto& trampoline = SKSE::GetTrampoline();
			_ProcessHit = trampoline.write_call<5>(hook.address() + RELOCATION_OFFSET(0x3C0, 0x4A8), processHit);
			logger::info("hook:OnMeleeHit");
		}

	private:
		static void processHit(RE::Actor* victim, RE::HitData& hitData);
		static inline REL::Relocation<decltype(processHit)> _ProcessHit;  //140626400       14064BAB0
	};

	class Hook_OnPlayerUpdate
	{
	public:
		static void install()
		{
			REL::Relocation<std::uintptr_t> PlayerCharacterVtbl{ RE::VTABLE_PlayerCharacter[0] };

			_Update = PlayerCharacterVtbl.write_vfunc(0xAD, Update);
			logger::info("hook:OnPlayerUpdate");
		}

	private:
		static void Update(RE::PlayerCharacter* a_this, float a_delta);
		static inline REL::Relocation<decltype(Update)> _Update;
	};

	/*class Hook_OnCharacterUpdate
	{
	public:
		static void install()
		{
			REL::Relocation<std::uintptr_t> CharacterVtbl{ RE::VTABLE_Character[0] };
			_Update = CharacterVtbl.write_vfunc(0xAD, Update);
			logger::info("hook:OnCharacterUpdate");
		}

	private:
		static void Update(RE::Character* a_this, float a_delta);
		static inline REL::Relocation<decltype(Update)> _Update;
	};*/

	class Hook_NotifyAnimationGraph
	{
	public:
		static void install()
		{
			// 1. Hook Characters (NPCs, Creatures, Mounts)
			REL::Relocation<std::uintptr_t> charVtbl{ RE::VTABLE_Character[3] };
			_NotifyAnimationGraph_Char = charVtbl.write_vfunc(0x1, NotifyAnimationGraph_Char);
			logger::info("hook:NotifyAnimationGraph_Char");

			// 2. Hook PlayerCharacter
			REL::Relocation<std::uintptr_t> playerVtbl{ RE::VTABLE_PlayerCharacter[3] };
			_NotifyAnimationGraph_Player = playerVtbl.write_vfunc(0x1, NotifyAnimationGraph_Player);
			logger::info("hook:NotifyAnimationGraph_Player");
		}

	private:
		static bool NotifyAnimationGraph_Char(RE::IAnimationGraphManagerHolder* a_holder, const RE::BSFixedString& a_eventName);
		static bool NotifyAnimationGraph_Player(RE::IAnimationGraphManagerHolder* a_holder, const RE::BSFixedString& a_eventName);

		// Shared internal handler
		static bool ProcessNotifyAnimationGraph(RE::IAnimationGraphManagerHolder* a_holder, const RE::BSFixedString& a_eventName, std::function<bool()> a_originalCall);

		static inline REL::Relocation<decltype(&NotifyAnimationGraph_Char)> _NotifyAnimationGraph_Char;
		static inline REL::Relocation<decltype(&NotifyAnimationGraph_Player)> _NotifyAnimationGraph_Player;
	};

	class Hook_OnProjectileCollision
	{
	public:
		static void install()
		{
			REL::Relocation<std::uintptr_t> arrowProjectileVtbl{ RE::VTABLE_ArrowProjectile[0] };
			REL::Relocation<std::uintptr_t> missileProjectileVtbl{ RE::VTABLE_MissileProjectile[0] };

			_arrowCollission = arrowProjectileVtbl.write_vfunc(190, OnArrowCollision);
			_missileCollission = missileProjectileVtbl.write_vfunc(190, OnMissileCollision);
			logger::info("hook:OnProjectileCollision");
		};

	private:
		static void OnArrowCollision(RE::Projectile* a_this, RE::hkpAllCdPointCollector* a_AllCdPointCollector);
		static void OnMissileCollision(RE::Projectile* a_this, RE::hkpAllCdPointCollector* a_AllCdPointCollector);
		
		static inline REL::Relocation<decltype(OnArrowCollision)> _arrowCollission;
		static inline REL::Relocation<decltype(OnMissileCollision)> _missileCollission;
	};

	class Hook_OnMeleeCollision
	{
	public:
		static void install()
		{
			REL::Relocation<uintptr_t> hook{ RELOCATION_ID(37650, 38603) };  //SE:627930 + 38B AE:64D350 +  45A
			auto& trampoline = SKSE::GetTrampoline();

			_ProcessHit = trampoline.write_call<5>(hook.address() + RELOCATION_OFFSET(0x38B, 0x45A), processHit);

			logger::info("hook:OnMeleeCollision");
		}
	private:
		static void processHit(RE::Actor* a_aggressor, RE::Actor* a_victim, std::int64_t a_int1, bool a_bool, void* a_unkptr);
		static inline REL::Relocation<decltype(processHit)> _ProcessHit;
	};

	class Hook_MagicStagger
	{
	public:
		static void install()
		{
			auto& trampoline = SKSE::GetTrampoline();

			// REL ID 34188 (SE) / 34982 (AE) -> Handles incoming magic/shout/script stagger events
			REL::Relocation<uintptr_t> hook{ RELOCATION_ID(34188, 34982) };

			_ProcessStagger = trampoline.write_call<5>(hook.address() + 0x6F, processStagger);
			logger::info("hook:MagicStagger");
		}

	private:
		static void processStagger(RE::Actor* a_target, float a_staggerMult, RE::Actor* a_aggressor);
		static inline REL::Relocation<decltype(processStagger)> _ProcessStagger;
	};

	class Hook_PerkEntryBypass
	{
	public:
		static void install()
		{
			auto& trampoline = SKSE::GetTrampoline();

			// Target address of Skyrim's ApplyPerkEntryPoint layout
			REL::Relocation<uintptr_t> hook{ RELOCATION_ID(36700, 37710) }; // SE: 5FA1B0, AE: 6116F0

			// Safely patch the call sites using the trampoline.
			// Even though we don't call the originals, we MUST hold their return addresses to prevent stack mismatching.
			_AggressorFunc = trampoline.write_call<5>(hook.address() + RELOCATION_OFFSET(0x9A, 0xA1), AggressorThunk);
			_TargetFunc = trampoline.write_call<5>(hook.address() + RELOCATION_OFFSET(0xAE, 0xB9), TargetThunk);

			logger::info("hook:PerkEntryBypass");
		}

	private:
		// Dead-end thunks: We do absolutely nothing and return, bypassing the original game logic completely.
		static void AggressorThunk(int32_t a_entry, RE::Actor* a_target, RE::Actor* a_aggressor, float& a_staggerMult);
		static void TargetThunk(int32_t a_entry, RE::Actor* a_target, RE::Actor* a_aggressor, float& a_staggerMult);

		// Keeps these in scope so the trampoline remains stable throughout execution lifetimes
		static inline REL::Relocation<void(int32_t, RE::Actor*, RE::Actor*, float&)> _AggressorFunc;
		static inline REL::Relocation<void(int32_t, RE::Actor*, RE::Actor*, float&)> _TargetFunc;
	};

	inline void install()
	{
		logger::info("Installing hooks...");
		SKSE::AllocTrampoline(1ULL << 14);
		Hook_OnGetAttackStaminaCost::install();
		Hook_OnRestoreActorValue::install();
		Hook_OnMeleeHit::install();
		Hook_OnPlayerUpdate::install();
		//Hook_OnCharacterUpdate::install();
		Hook_NotifyAnimationGraph::install();
		Hook_OnProjectileCollision::install();
		Hook_OnMeleeCollision::install();
		Hook_PerkEntryBypass::install();
		Hook_MagicStagger::install();

		logger::info("...done");
	}
}

