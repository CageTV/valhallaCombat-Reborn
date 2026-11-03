#pragma once

#include "SKSE/Trampoline.h"
#include "include/Utils.h"
namespace Hooks
{
	/* 1.7.x build: the call offsets below were measured on 1.6.x. A write_call over anything but a CALL (E8) would corrupt the code, so a hook is installed
	only when the byte at the site really is a call into the code section; otherwise it is skipped and logged (this game version needs a new offset). */
	/*True on Skyrim 1.7.99 and newer.*/
	inline bool IsRuntime17()
	{
		return REL::Module::IsAtLeast(SKSE::RUNTIME_SSE_1_7_99);
	}

	inline bool CheckCallSite(std::uintptr_t a_site, std::string_view a_name)
	{
		const auto op = *reinterpret_cast<const std::uint8_t*>(a_site);
		if (op != 0xE8) {
			logger::error("hook:{} SKIPPED: the byte at the call site is {:#04x}, not a call (E8). This game version needs a new offset", a_name, op);
			return false;
		}
		const auto rel = *reinterpret_cast<const std::int32_t*>(a_site + 1);
		const auto target = a_site + 5 + rel;
		const auto text = REL::Module::get().segment(REL::Segment::textx);
		if (target < text.address() || target >= text.address() + text.size()) {
			logger::error("hook:{} SKIPPED: the E8 at the call site points outside the code section (target RVA {:#x}), so it is the middle of another instruction. This game version needs a new offset",
				a_name, static_cast<std::int64_t>(target - REL::Module::get().base()));
			return false;
		}
		logger::info("hook:{} site ok, calls RVA {:#x}", a_name, target - REL::Module::get().base());
		return true;
	}

	/*Every call inside [a_base, a_base + a_len) that goes to the function with the given Address Library id. The offset of a call site moves between game
	versions, the function it calls does not, so the site is found by what it calls.*/
	inline std::vector<std::uintptr_t> FindCallsTo(std::uintptr_t a_base, std::size_t a_len, std::uint64_t a_targetId)
	{
		std::vector<std::uintptr_t> sites;
		const auto                  want = REL::ID(a_targetId).address();
		for (std::size_t i = 0; i + 5 <= a_len; ++i) {
			const auto p = a_base + i;
			if (*reinterpret_cast<const std::uint8_t*>(p) != 0xE8) {
				continue;
			}
			if (p + 5 + *reinterpret_cast<const std::int32_t*>(p + 1) == want) {
				sites.push_back(p);
			}
		}
		return sites;
	}
	class Hook_OnGetAttackStaminaCost  //Actor__sub_140627930+16E	call ActorValueOwner__sub_1403BEC90
	{
		/*to cancel out vanilla power attack stamina consumption.*/
	public:
		static void install()
		{
			auto& trampoline = SKSE::GetTrampoline();

			REL::Relocation<uintptr_t> hook{ RELOCATION_ID(37650, 38603) };  //SE:627930 + 16E => 3BEC90 AE:64D350 + 171 => 3D6720

			const auto site = hook.address() + RELOCATION_OFFSET(0x16E, 0x171);
			if (!CheckCallSite(site, "getAttackStaminaCost")) {
				return;
			}
			_getAttackStaminaCost = trampoline.write_call<5>(site, getAttackStaminaCost);
			
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
			const auto site = hook.address() + RELOCATION_OFFSET(0x176, 0xE1);
			if (!CheckCallSite(site, "RestoreActorValue")) {
				return;
			}
			_RestoreActorValue = trampoline.write_call<5>(site, RestoreActorValue);
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
			std::uintptr_t site = hook.address() + RELOCATION_OFFSET(0x3C0, 0x4A8);
			if (Hooks::IsRuntime17()) {
				// 1.7: find the call to the hit function (AE id 38586) inside the collision function instead of trusting the 1.6 offset
				const auto sites = FindCallsTo(hook.address(), 0x900, 38586);
				for (const auto s : sites) {
					logger::info("hook:OnMeleeHit processHit: call to id 38586 at +{:#x}", s - hook.address());
				}
				if (sites.empty()) {
					// Another plugin (Precision, an impact framework ...) may already have rewritten this call, so no call to 38586 is left to find. On 1.7.104 the site is at the
					// same offset as on 1.6 AE (+0x4A8); take it only if the code around it is exactly the game's (the 8 bytes before the call and the 23 after it) and hook it as a
					// chain: write_call returns whatever the call went to, so the other plugin keeps working.
					static constexpr std::uint8_t before[] = { 0x48, 0x8D, 0x54, 0x24, 0x50, 0x48, 0x8B, 0xCF };
					static constexpr std::uint8_t after[] = { 0xF3, 0x0F, 0x10, 0x8C, 0x24, 0xCC, 0x00, 0x00, 0x00, 0x0F, 0x2F, 0xCE, 0x76, 0x0E, 0x45, 0x33, 0xC9, 0x4C, 0x8B, 0xC7, 0x48, 0x8B, 0xCB };
					const auto                    fixed = hook.address() + 0x4A8;
					if (*reinterpret_cast<const std::uint8_t*>(fixed) == 0xE8 && std::memcmp(reinterpret_cast<const void*>(fixed - sizeof(before)), before, sizeof(before)) == 0 &&
						std::memcmp(reinterpret_cast<const void*>(fixed + 5), after, sizeof(after)) == 0) {
						logger::info("hook:OnMeleeHit processHit: the call to id 38586 was already rewritten by another plugin; the code around +0x4A8 is the game's, hooking it as a chain");
						_ProcessHit = trampoline.write_call<5>(fixed, processHit);
						logger::info("hook:OnMeleeHit (chained)");
						return;
					}
				}
				if (sites.size() != 1) {
					logger::error("hook:OnMeleeHit processHit SKIPPED: found {} calls to id 38586 in id 38627, expected exactly one", sites.size());
					return;
				}
				site = sites.front();
			}
			if (!CheckCallSite(site, "OnMeleeHit processHit")) {
				return;
			}
			_ProcessHit = trampoline.write_call<5>(site, processHit);
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

			const auto site = hook.address() + RELOCATION_OFFSET(0x38B, 0x45A);
			if (!CheckCallSite(site, "OnMeleeCollision processHit")) {
				return;
			}
			_ProcessHit = trampoline.write_call<5>(site, processHit);

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

			const auto site = hook.address() + 0x6F;
			if (!CheckCallSite(site, "MagicStagger")) {
				return;
			}
			_ProcessStagger = trampoline.write_call<5>(site, processStagger);
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
			const auto aggressorSite = hook.address() + RELOCATION_OFFSET(0x9A, 0xA1);
			const auto targetSite = hook.address() + RELOCATION_OFFSET(0xAE, 0xB9);
			if (!CheckCallSite(aggressorSite, "PerkEntryBypass aggressor") || !CheckCallSite(targetSite, "PerkEntryBypass target")) {
				return;  // both sites or none: the two thunks belong together
			}
			_AggressorFunc = trampoline.write_call<5>(aggressorSite, AggressorThunk);
			_TargetFunc = trampoline.write_call<5>(targetSite, TargetThunk);

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

