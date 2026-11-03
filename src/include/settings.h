#pragma once
#include "lib/robin_hood.h"
#include "lib/SimpleIni.h"

/*All the settings of Valhalla combat*/
class settings
{
public:
	class facts
	{
	public:
		static inline bool TrueHudAPI_HasSpecialBarControl;
		static inline bool TrueHudAPI_PoiseControlsSpecialBar;
		static inline bool TrueHudAPI_Obtained;
		static inline bool PrecisionAPI_Obtained;
		static inline bool EldenParry_ObtainedAPI;
		//static inline bool Exhaustion_Obtained;
	};
	
#pragma region GlobalSettings
	static inline RE::TESGlobal* glob_TrueHudAPI;
	static inline RE::TESGlobal* glob_TrueHudAPI_SpecialMeter;
	static inline RE::TESGlobal* glob_Nemesis_EldenCounter_NPC;
	static inline RE::TESGlobal* glob_Nemesis_EldenCounter_Damage;
	static inline RE::TESGlobal* glob_EldenCounter_EspPluginLoaded;
	static inline RE::TESGlobal* glob_Exhaustion_EspPluginLoaded;
	static inline RE::TESGlobal* glob_ExhaustionBlockSprintRun;
	static inline RE::TESGlobal* glob_UnblockableHits_EspPluginLoaded;
	static inline RE::TESGlobal* glob_NPCTimedBlockEnabled;
	static inline RE::TESGlobal* glob_NPCParryEnabled;
	static inline RE::TESGlobal* glob_PlayerExecutionEnabled;
	static inline RE::TESGlobal* glob_LightAttackStaminaCostEnabled;
	static inline RE::TESGlobal* glob_PowerAttackStaminaCostEnabled;
	static inline RE::TESGlobal* glob_LightBashStaminaCostEnabled;
	static inline RE::TESGlobal* glob_PowerBashStaminaCostEnabled;
#pragma endregion

#pragma region AdvancedSettings
	static inline bool bAdvancedLogging = false;
	static inline bool bApplySpellOnSuccess = false;
	//static inline bool bExecutionGraphReset = false;
	static inline float fGraph_LightAttackCostMult = 1.0f;
	static inline float fGraph_PowerAttackCostMult = 1.0f;
	static inline float fGraph_LightBashCostMult = 1.0f;
	static inline float fGraph_PowerBashCostMult = 1.0f;
#pragma endregion

#pragma region StaminaSettings
	static inline bool bUIAlert = true;
	static inline bool bNonCombatStaminaCost = true;
	
	static inline float fStaminaRegenMult = 5.0f;
	static inline float fStaminaRegenLimit = 50.0f;
	static inline float fStaminaRegenMin = 10.0f;
	static inline float fCombatStaminaRegenMult = 1.0f;
	static inline float fBlockingStaminaRegenMult = 0.5f;
	static inline float fStaminaRegenDelay = 2.0f;

	static inline bool bBlockedHitRegenStamina = true;

	static inline bool bBlockStaminaToggle = true;
	static inline bool bGuardBreak = true;
	static inline float fBlockLockoutDuration_NPC = 2.0f;
	static inline float fExhaustionBlockDuration_NPC = 3.0f;
	static inline float fBckShdStaminaMult_PC_Block_NPC = 1.0f;
	static inline float fBckWpnStaminaMult_PC_Block_NPC = 1.0f;

	static inline float fBckShdStaminaMult_NPC_Block_PC = 1.0f;
	static inline float fBckWpnStaminaMult_NPC_Block_PC = 1.0f;

	static inline float fBckShdStaminaMult_NPC_Block_NPC = 1.0f;
	static inline float fBckWpnStaminaMult_NPC_Block_NPC = 1.0f;
	
	enum LightAttackStaminaMode
	{
		Disabled = 0,
		Legacy = 1,
		HitFrame = 2
	};

	static inline uint32_t uLightAttackStaminaMode = HitFrame;
	static inline bool bAttackStaminaToggle_NPC = true;
	static inline bool bStaminaDebuffToggle = true;
	static inline bool bExhaustionBlockSprintRun = true;
	static inline bool bExhaustionBlockAttacks = true;
	static inline bool bLightAttackPerkMult = true;
	static inline float fMeleeCostLight_Base = 10.0f;
	static inline float fMeleeCostLight_WeightMult = 1.0f;
	static inline float fMeleeRewardLightHit_Point = 10.0f;
	static inline float fMeleeRewardLightHit_WeightMult = 1.0f;
	static inline float fMeleeRewardLightHit_Percent = 0.1f;

	static inline bool bUseCustomPowerAttackCost = true;
	static inline bool bPowerAttackPerkMult = true;
	static inline float fMeleeCostPower_Base = 20.0f;
	static inline float fMeleeCostPower_WeightMult = 2.0f;

	static inline bool bUseCustomLightBashCost = true;
	static inline bool bSuccessfulParryNoCost = false;
	static inline bool bLightBashPerkMult = true;
	static inline float fMeleeCostLightBash_Base = 5.0f;
	static inline float fMeleeCostLightBash_WeightMult = 0.1f;

	static inline bool bUseCustomPowerBashCost = true;
	static inline bool bPowerBashPerkMult = true;
	static inline float fMeleeCostPowerBash_Base = 10.0f;
	static inline float fMeleeCostPowerBash_WeightMult = 0.1f;

	static inline float fAdditionalHitFrameCostMult = 1.0f;
	static inline float fAdditionalHitFrameRewardMult = 1.0f;
#pragma endregion

#pragma region timedBlockSettings
	static inline bool bTimedBlockToggle = true;
	static inline bool bTimedBlockToggle_NPC = true;
	static inline bool bBlockProjectileToggle = true;
	static inline bool bBlockProjectileToggle_NPC = true;
	static inline bool bTimedBlockProjectileToggle = true;
	static inline bool bTimedBlockProjectileToggle_NPC = true;
	//static inline bool bIsBlockingTimedBlock = false;
	static inline bool bWeaponBlockArrow = true;
	static inline bool bWeaponBlockSpell = true;
	static inline bool bShieldBlockArrow = true;
	static inline bool bShieldBlockSpell = true;
	static inline bool bPerfectBlockStaminaRegen = true;
	static inline bool bPerfectBlockStaminaRegenExhaustion = true;
	
	static inline float fTimedBlockWindow = 0.3f;
	static inline float fPerfectBlockWindow = 0.15f;
	static inline float fTimedBlockCooldownTime = 0.5f;
	static inline float fTimedBlockStaminaCostMult = 0.5f;

	static inline bool bBlockCommitmentToggle = true;
	static inline bool bBlockCommitmentToggle_NPC = true;
	static inline float fBlockCommitmentTime = 0.3f;

	static inline bool bTackleToggle = false;
	static inline bool bTackleToggle_NPC = false;
	static inline float fTackleWindow = 0.2f;
	static inline float fTackleCooldownTime = 2.0f;

	static inline bool bEnableBlockExp = true;
	static inline float fTimedBlockMeleeExp = 10.0f;
	static inline float fTimedBlockProjectileExp = 10.0f;
	static inline float fBlockProjectileExp = 5.0f;

	static inline uint32_t uTimedBlockSparkType = 1;  //0 = none, 1 = fantasy, 2 = realism
	static inline bool bTimedBlockScreenShake = false;
	static inline bool bTimeBlockSFX = true;
	static inline float fTimedBlockSFXVolume = 0.5f;
	static inline bool bTimedBlockSlowTime = true;
	static inline float fTimedBlockSlowTime_Timed = 0.0f;
	static inline float fTimedBlockSlowTime_Perfect = 0.3f;
	static inline float fTimedBlockSlowTimePercentage = 0.5f;
#pragma endregion

#pragma region StunSettings
	static inline bool bStunToggle = true;
	static inline bool bStunMeterToggle = true;
	static inline bool bPlayerStunToggle = false;
	static inline bool bPlayerExecutionToggle = false;
	static inline bool bFriendlyStunToggle = false;
	static inline bool bDownedStateToggle = false;
	static inline float fStunRegenDelaySetting = 3.0f;
	
	static inline float fStunTimedBlockMult = 1.0f;
	static inline float fStunParryMult = 1.0f;
	static inline float fStunBashMult = 0.3f;
	static inline float fStunPowerBashMult = 1.0f;
	static inline float fStunNormalAttackMult = 0.0f;
	static inline float fStunPowerAttackMult = 1.0f;
	static inline float fStunCounterAttackMult = 1.2f;

	static inline float fStunUnarmedMult = 1.0f;
	static inline float fStunDaggerMult = 0.5f;
	static inline float fStunSwordMult = 1.0f;
	static inline float fStunWarAxeMult = 1.1f;
	static inline float fStunMaceMult = 1.1f;
	static inline float fStunGreatSwordMult = 1.2f;
	static inline float fStun2HBluntMult = 1.2f;

	// Toggle for essential NPCs to be executed
	static inline bool bExecutionLimit = true; //NPCs cannot be executed unless under certain health threshold.
	static inline float fExecutionHealthThreshold = 0.3f;
	static inline bool bAutoExecution = false; //automatically execute on hit.
	static inline uint32_t uExecutionKey = 0; // 0 = unassigned
#pragma endregion

#pragma region Compatibility
	static inline bool bPoiseCompatibility = false;
	static inline bool bUnblockableCompatibility = false;

	// Struct to hold our target graph variable values
	struct AnimEventData {
		bool bPoiseBreaksBlock = false;
		bool bUnblockable = false;
		float fStaminaMult = 1.0f;
		float fPoiseMult = 1.0f;
	};

	// Map linking an event name (e.g., "PIE.$BackPower") to the variable states
	static inline robin_hood::unordered_map<std::string, AnimEventData> animEventDataMap;

	// Container for animation events to sink/suppress while an actor is debuffed
	static inline robin_hood::unordered_set<RE::BSFixedString> suppressedExhaustionAnimEvents;

	static inline robin_hood::unordered_map<std::string, std::string> specialAttackPresets;
#pragma endregion

#pragma region PoiseSettings
	// Structural System Toggles
	static inline bool bPoise_PoiseToggle = true;
	static inline bool bPoise_PoiseMeterToggle = true;
	static inline bool bPoise_DebugConsoleLog = true;

	// Core Engine & Timers
	static inline float fPoise_StaggerProtectTime = 1.5f;
	static inline float fPoise_PoiseRegen = 0.1f;
	static inline float fPoise_RegenDelayTime = 1.2f;
	static inline float fPoise_DifficultyMultWeight = 0.0f;

	static inline float fPoise_BasePoiseHealth = 50.0f;
	static inline float fPoise_BaseArmorPoiseHealth = 50.0f;
	static inline float fPoise_HeavyArmorPoiseBonus = 0.5f;

	static inline float fPoise_ScaleHealthMult = 0.5f;
	static inline float fPoise_ScaleDamageMult = 0.5f;
	static inline float fPoise_WeightHealthMult = 0.5f;
	static inline float fPoise_WeightDamageMult = 0.5f;

	// Stagger Damage Thresholds
	static inline float fPoise_SmallStaggerTHLD = 0.25f;
	static inline float fPoise_MediumStaggerTHLD = 0.50f;
	static inline float fPoise_LargeStaggerTHLD = 0.75f;

	// Baseline Action Poise Damages
	static inline float fPoise_BaseMeleePoiseDamage = 15.0f;
	static inline float fPoise_BaseRangePoiseDamage = 15.0f;
	static inline float fPoise_BaseMagicPoiseDamage = 100.0f;

	// Action Multipliers
	static inline float fPoise_PowerAttackMult = 1.5f;
	static inline float fPoise_BashMult = 5.0f;
	static inline float fPoise_PowerBashMult = 5.0f;

	// Target State Multipliers
	static inline float fPoise_TargetPowerBashMult = 1.4f;
	static inline float fPoise_TargetBashMult = 1.1f;
	static inline float fPoise_TargetPowerAttackMult = 1.4f;
	static inline float fPoise_TargetLightAttackMult = 1.2f;
	static inline float fPoise_TargetBowDrawMult = 2.0f;
	static inline float fPoise_TargetCastingMult = 2.0f;

	// Weapon Class Multipliers
	static inline float fHandToHandMeleeMult = 0.8f;
	static inline float fOneHandDaggerMult = 0.6f;
	static inline float fOneHandSwordMult = 1.0f;
	static inline float fOneHandAxeMult = 1.1f;
	static inline float fOneHandMaceMult = 1.3f;
	static inline float fTwoHandSwordMult = 1.3f;
	static inline float fTwoHandAxeMult = 1.6f;
	static inline float fBowMult = 0.8f;
	static inline float fCrossbowMult = 0.8f;
	static inline float fStaffMult = 0.0f;

	// Armor Slot Multipliers
	static inline float fHeadMult = 0.15f;
	static inline float fBodyMult = 0.50f;
	static inline float fHandsMult = 0.15f;
	static inline float fFeetMult = 0.20f;
	static inline float fShieldMult = 0.00f;

	// Maps explicit FormIDs (weapons/armor) to custom poise overrides
	static inline robin_hood::unordered_map<RE::FormID, float> poiseDamageMultOverrideMap;

	// Maps Actor Base FormIDs or Race FormIDs to custom Base Poise Damage values
	static inline robin_hood::unordered_map<RE::FormID, float> poiseBaseDamageOverrideMap;

	// Maps Actor Base FormIDs or Race FormIDs to custom Poise Health pools
	static inline robin_hood::unordered_map<RE::FormID, float> poiseHealthOverrideMap;

	// Map storing behavior graph project names (lowercase) -> base poise health value
	static inline robin_hood::unordered_map<std::string, float> poiseHealthGraphOverrideMap;

	// Map storing behavior graph project names (lowercase) -> base poise damage value
	static inline robin_hood::unordered_map<std::string, float> poiseBaseDamageGraphOverrideMap;

	// Armor Slot Multipliers mapped directly to Engine Slot Enums
	static inline robin_hood::unordered_map<uint32_t, float> armorSlotMultMap;
#pragma endregion

#pragma region ParrySettings
	static inline bool bParryToggle = true;
	static inline bool bParryToggle_NPC = true;
	static inline bool bEnableProjectileParry = true;
	static inline bool bEnableProjectileParry_NPC = true;
	static inline bool bWeaponParryArrow = true;
	static inline bool bWeaponParrySpell = true;
	static inline bool bShieldParryArrow = true;
	static inline bool bShieldParrySpell = true;
	//static inline bool bSuccessfulParryNoCost = true;

	static inline float fParryWindow_Start = 0.0f;
	static inline float fParryWindow_End = 0.3f;

	static inline bool bEnableGuardBash = true;

	static inline bool bEnableParryExp = true;
	static inline float fParryMeleeExp = 15.0f;
	static inline float fParryProjectileExp = 20.0f;

	static inline uint32_t uParrySparkType = 1;  //0 = none, 1 = fantasy, 2 = realism
	static inline bool bParrySFX = true;
	static inline float fParrySFXVolume = 0.5f;
	static inline bool bParryScreenShake = false;
	static inline bool bParrySlowTime = true;
	static inline float fParrySlowTimeDuration = 0.3f;
	static inline float fParrySlowTimePercentage = 0.5f;
#pragma endregion

#pragma region AISettings
	static inline bool bToggleAI = true;
	static inline bool bStopAIBash = false;
	static inline bool bStopAIBashPower = false;
	static inline bool bToggleDualWield = true;
	static inline bool bToggleUnarmed = true;
	static inline std::int32_t iMaxSimultaneousDefenders = 2;

	static inline float fParryAttemptChance = 0.10f;
	static inline float fTimedBlockAttemptChance = 0.15f;
	static inline float fPerfectBlockAttemptChance = 0.05f;

	static inline float fConfidenceMult = 1.0f;
	static inline float fConfidenceMin = 0.1f;
	static inline float fConfidenceMax = 2.0f;
	static inline float fSkillMult = 1.0f;
	static inline float fSkillMin = 1.0f;
	static inline float fSkillMax = 2.0f;

	static inline float fTimedBlockCSMult = 1.0f;
	static inline float fPerfectBlockCSMult = 1.0f;
	static inline float fParryCSMult = 1.0f;
	static inline float fTimedBlockSkillMult = 1.0f;
	static inline float fPerfectBlockSkillMult = 1.0f;
	static inline float fParrySkillMult = 1.0f;

	static inline float fParryCooldown = 3.0f;
	static inline float fBlockCooldown = 3.0f;

	static inline float fBaseLightImpactTime = 0.4f;
	static inline float fBasePowerImpactTime = 0.7f;

	static inline float fmaxConeDegrees = 60.0f;
	static inline float fbufferDistance = 35.0f;

	static inline float fParryDelayOffset = 0.0f;
	static inline float fPerfectBlockDelayOffset = 0.0f;
	static inline float fTimedBlockDelayOffset = 0.0f;
	static inline float fJitterScale = 1.0f;
#pragma endregion

#pragma region ReactionSettings
	static inline float fReactionTypeSmallTHLD = 0.26f;
	static inline float fReactionTypeMediumTHLD = 0.51f;
	static inline float fReactionTypeLargeTHLD = 0.76f;
	static inline float fReactionTypeLargestTHLD = 10.0f;
#pragma endregion

	static void readSettings();
	static void updateGlobals();
	static void init();

};
