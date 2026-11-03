#include "include/stunHandler.h"
#include "include/settings.h"
#include "include/Utils.h"
#include "ValhallaCombat.hpp"
using namespace inlineUtils;
/*tweaks the value of designated game setting
	@param gameSettingStr game setting to be tweaked.
	@param val desired float value of gamesetting.*/
static void setGameSettingf(const char* a_setting, float a_value)
{
	RE::Setting* setting = nullptr;
	RE::GameSettingCollection* _settingCollection = RE::GameSettingCollection::GetSingleton();
	setting = _settingCollection->GetSetting(a_setting);
	if (!setting) {
		logger::info("Error: invalid setting: {}", a_setting);
	} else {
		//logger::info("setting {} from {} to {}", settingStr, setting->GetFloat(), val);
		setting->data.f = a_value;
	}
}

/*a map of all game races' original stamina regen, in case player wants to tweak the stamina regen values again*/
static inline robin_hood::unordered_map<RE::TESRace*, float> staminaRegenMap;

/*offset stamina regen of every single race. Multiplying values by MULT and clamping the final value.
	@param mult multiplier for stamina regen.
	@param upperLimit upper limit for stamina regen.*/
static void offsetStaminaRegen(float mult, float upperLimit, float lowerLimit = 10)
{
	for (auto& race : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESRace>()) {
		if (race) {
			float staminaRegen;
			if (!staminaRegenMap.contains(race)) {
				float val = race->data.staminaRegen;
				staminaRegenMap[race] = val;
				staminaRegen = val * mult;
			} else {
				staminaRegen = mult * staminaRegenMap[race];
			}
			/* Clamp regen val.*/
			staminaRegen = std::min(staminaRegen, upperLimit);
			staminaRegen = std::max(staminaRegen, lowerLimit);
			race->data.staminaRegen = staminaRegen;
			//logger::info("setting stamina regen rate for race {} to {}.", race->GetName(), staminaRegen);
		}
	}
}

void settings::init() {
	logger::info("Initilize settings...");
	DtryUtils::formLoader loader("ValhallaCombat.esp");
	loader.load(glob_Nemesis_EldenCounter_Damage, 0x56A23);
	loader.load(glob_Nemesis_EldenCounter_NPC, 0x56A24);
	loader.load(glob_TrueHudAPI, 0x56A25);
	loader.load(glob_TrueHudAPI_SpecialMeter, 0x56A26);
	loader.load(glob_Exhaustion_EspPluginLoaded, 0x56A27);
	loader.load(glob_ExhaustionBlockSprintRun, 0x56A28);
	loader.load(glob_UnblockableHits_EspPluginLoaded, 0x56A29);
	loader.load(glob_NPCTimedBlockEnabled, 0x56A30);
	loader.load(glob_NPCParryEnabled, 0x56A31);
	loader.load(glob_PlayerExecutionEnabled, 0x56A32);
	loader.load(glob_LightAttackStaminaCostEnabled, 0x56A33);
	loader.load(glob_PowerAttackStaminaCostEnabled, 0x56A34);
	loader.load(glob_LightBashStaminaCostEnabled, 0x56A35);
	loader.load(glob_PowerBashStaminaCostEnabled, 0x56A36);
	loader.log();
	logger::info("...done");
}

void settings::updateGlobals() {
	logger::info("Update globals...");
	if (glob_TrueHudAPI) {
		glob_TrueHudAPI->value = ValhallaCombat::GetSingleton()->ersh_TrueHUD != nullptr ? 1.f : 0.f;
	}
	if (glob_TrueHudAPI_SpecialMeter) {
		glob_TrueHudAPI_SpecialMeter->value = facts::TrueHudAPI_HasSpecialBarControl ? 1.f : 0.f;
	}
	auto pc = RE::PlayerCharacter::GetSingleton();
	if (pc) {
		if (glob_Nemesis_EldenCounter_Damage) {
			bool bDummy;
			glob_Nemesis_EldenCounter_Damage->value = pc->GetGraphVariableBool("is_Guard_Countering", bDummy);
		}
		if (glob_Nemesis_EldenCounter_NPC) {
			bool bDummy;
			glob_Nemesis_EldenCounter_NPC->value = pc->GetGraphVariableBool("IsValGC", bDummy);
		}
		if (glob_UnblockableHits_EspPluginLoaded) {
			bool bDummy;
			glob_UnblockableHits_EspPluginLoaded->value = pc->GetGraphVariableBool("isUnblockableHit", bDummy);
		}
	}
	if (glob_Exhaustion_EspPluginLoaded) {
		glob_Exhaustion_EspPluginLoaded->value = data::bExhaustionInstalled ? 1.0f : 0.0f;
	}
	if (glob_ExhaustionBlockSprintRun) {
		glob_ExhaustionBlockSprintRun->value = settings::bExhaustionBlockSprintRun ? 1.0f : 0.0f;
	}
	if (glob_NPCTimedBlockEnabled) {
		glob_NPCTimedBlockEnabled->value = settings::bTimedBlockToggle_NPC ? 1.0f : 0.0f;
	}
	if (glob_NPCParryEnabled) {
		glob_NPCParryEnabled->value = settings::bParryToggle_NPC ? 1.0f : 0.0f;
	}
	if (glob_PlayerExecutionEnabled) {
		glob_PlayerExecutionEnabled->value = settings::bPlayerExecutionToggle ? 1.0f : 0.0f;
	}
	if (glob_LightAttackStaminaCostEnabled) {
		glob_LightAttackStaminaCostEnabled->value = (settings::uLightAttackStaminaMode != Disabled) ? 1.0f : 0.0f;
	}
	if (glob_PowerAttackStaminaCostEnabled) {
		glob_PowerAttackStaminaCostEnabled->value = settings::bUseCustomPowerAttackCost ? 1.0f : 0.0f;
	}
	if (glob_LightBashStaminaCostEnabled) {
		glob_LightBashStaminaCostEnabled->value = settings::bUseCustomLightBashCost ? 1.0f : 0.0f;
	}
	if (glob_PowerBashStaminaCostEnabled) {
		glob_PowerBashStaminaCostEnabled->value = settings::bUseCustomPowerBashCost ? 1.0f : 0.0f;
	}
	logger::info("...done");
}

#define SETTINGFILE_PATH "Data\\MCM\\Settings\\ValhallaCombat.ini"
/*Load the setting from ini file with the same name as the variable in code.*/
#define FETCH(setting) load(setting, #setting)
/*read settings from ini, and update them into game settings.*/
void settings::readSettings() {
	logger::info("Read ini settings...");
	DtryUtils::settingsLoader MCM_Settings(SETTINGFILE_PATH);

	MCM_Settings.setActiveSection("Stamina");
	MCM_Settings.FETCH(bUIAlert);
	MCM_Settings.FETCH(bNonCombatStaminaCost);
	MCM_Settings.FETCH(fStaminaRegenMult);
	MCM_Settings.FETCH(fStaminaRegenLimit);
	MCM_Settings.FETCH(fStaminaRegenMin);
	MCM_Settings.FETCH(fCombatStaminaRegenMult);
	MCM_Settings.FETCH(fBlockingStaminaRegenMult);
	MCM_Settings.FETCH(fStaminaRegenDelay);
	MCM_Settings.FETCH(bBlockStaminaToggle);
	MCM_Settings.FETCH(fBlockLockoutDuration_NPC);
	MCM_Settings.FETCH(fExhaustionBlockDuration_NPC);
	MCM_Settings.FETCH(bBlockedHitRegenStamina);
	MCM_Settings.FETCH(bGuardBreak);
	MCM_Settings.FETCH(fBckShdStaminaMult_NPC_Block_NPC);
	MCM_Settings.FETCH(fBckShdStaminaMult_NPC_Block_PC);
	MCM_Settings.FETCH(fBckShdStaminaMult_PC_Block_NPC);
	MCM_Settings.FETCH(fBckWpnStaminaMult_NPC_Block_NPC);
	MCM_Settings.FETCH(fBckWpnStaminaMult_NPC_Block_PC);
	MCM_Settings.FETCH(fBckWpnStaminaMult_PC_Block_NPC);
	
	MCM_Settings.FETCH(bStaminaDebuffToggle);
	MCM_Settings.FETCH(bExhaustionBlockSprintRun);
	MCM_Settings.FETCH(bExhaustionBlockAttacks);
	MCM_Settings.FETCH(bAttackStaminaToggle_NPC);
	MCM_Settings.FETCH(bLightAttackPerkMult);
	MCM_Settings.FETCH(fMeleeCostLight_Base);
	MCM_Settings.FETCH(fMeleeCostLight_WeightMult);
	MCM_Settings.FETCH(fMeleeRewardLightHit_Point);
	MCM_Settings.FETCH(fMeleeRewardLightHit_WeightMult);
	MCM_Settings.FETCH(fMeleeRewardLightHit_Percent);

	MCM_Settings.FETCH(bUseCustomPowerAttackCost);
	MCM_Settings.FETCH(bPowerAttackPerkMult);
	MCM_Settings.FETCH(fMeleeCostPower_Base);
	MCM_Settings.FETCH(fMeleeCostPower_WeightMult);

	MCM_Settings.FETCH(bUseCustomLightBashCost);
	MCM_Settings.FETCH(bSuccessfulParryNoCost);
	MCM_Settings.FETCH(bLightBashPerkMult);
	MCM_Settings.FETCH(fMeleeCostLightBash_Base);
	MCM_Settings.FETCH(fMeleeCostLightBash_WeightMult);

	MCM_Settings.FETCH(bUseCustomPowerBashCost);
	MCM_Settings.FETCH(bPowerBashPerkMult);
	MCM_Settings.FETCH(fMeleeCostPowerBash_Base);
	MCM_Settings.FETCH(fMeleeCostPowerBash_WeightMult);

	MCM_Settings.FETCH(uLightAttackStaminaMode);
	MCM_Settings.FETCH(fAdditionalHitFrameCostMult);
	MCM_Settings.FETCH(fAdditionalHitFrameRewardMult);
	
	MCM_Settings.setActiveSection("TimedBlocking");
	MCM_Settings.FETCH(bTimedBlockToggle);
	MCM_Settings.FETCH(bTimedBlockToggle_NPC);
	MCM_Settings.FETCH(bBlockProjectileToggle);
	MCM_Settings.FETCH(bBlockProjectileToggle_NPC);
	MCM_Settings.FETCH(bTimedBlockProjectileToggle);
	MCM_Settings.FETCH(bTimedBlockProjectileToggle_NPC);
	//MCM_Settings.FETCH(bIsBlockingTimedBlock);
	MCM_Settings.FETCH(bWeaponBlockArrow);
	MCM_Settings.FETCH(bWeaponBlockSpell);
	MCM_Settings.FETCH(bShieldBlockArrow);
	MCM_Settings.FETCH(bShieldBlockSpell);
	MCM_Settings.FETCH(bPerfectBlockStaminaRegen);
	MCM_Settings.FETCH(bPerfectBlockStaminaRegenExhaustion);

	MCM_Settings.FETCH(fTimedBlockWindow);
	MCM_Settings.FETCH(fPerfectBlockWindow);
	MCM_Settings.FETCH(fTimedBlockCooldownTime);
	MCM_Settings.FETCH(fTimedBlockStaminaCostMult);

	MCM_Settings.FETCH(bBlockCommitmentToggle);
	MCM_Settings.FETCH(bBlockCommitmentToggle_NPC);
	MCM_Settings.FETCH(fBlockCommitmentTime);

	MCM_Settings.FETCH(bTackleToggle);
	MCM_Settings.FETCH(bTackleToggle_NPC);
	MCM_Settings.FETCH(fTackleWindow);
	MCM_Settings.FETCH(fTackleCooldownTime);

	MCM_Settings.FETCH(bEnableBlockExp);
	MCM_Settings.FETCH(fBlockProjectileExp);
	MCM_Settings.FETCH(fTimedBlockProjectileExp);
	MCM_Settings.FETCH(fTimedBlockMeleeExp);

	MCM_Settings.FETCH(uTimedBlockSparkType);
	MCM_Settings.FETCH(bTimedBlockScreenShake);
	MCM_Settings.FETCH(bTimeBlockSFX);
	MCM_Settings.FETCH(fTimedBlockSFXVolume);
	MCM_Settings.FETCH(bTimedBlockSlowTime);
	MCM_Settings.FETCH(fTimedBlockSlowTime_Timed);
	MCM_Settings.FETCH(fTimedBlockSlowTime_Perfect);
	MCM_Settings.FETCH(fTimedBlockSlowTimePercentage);

	MCM_Settings.setActiveSection("Parry");
	MCM_Settings.FETCH(bParryToggle);
	MCM_Settings.FETCH(bParryToggle_NPC);
	MCM_Settings.FETCH(bEnableProjectileParry);
	MCM_Settings.FETCH(bEnableProjectileParry_NPC);
	MCM_Settings.FETCH(bWeaponParryArrow);
	MCM_Settings.FETCH(bWeaponParrySpell);
	MCM_Settings.FETCH(bShieldParryArrow);
	MCM_Settings.FETCH(bShieldParrySpell);
	//MCM_Settings.FETCH(bSuccessfulParryNoCost);

	MCM_Settings.FETCH(fParryWindow_Start);
	MCM_Settings.FETCH(fParryWindow_End);

	MCM_Settings.FETCH(bEnableGuardBash);

	MCM_Settings.FETCH(bEnableParryExp);
	MCM_Settings.FETCH(fParryMeleeExp);
	MCM_Settings.FETCH(fParryProjectileExp);

	MCM_Settings.FETCH(uParrySparkType);
	MCM_Settings.FETCH(bParrySFX);
	MCM_Settings.FETCH(fParrySFXVolume);
	MCM_Settings.FETCH(bParryScreenShake);
	MCM_Settings.FETCH(bParrySlowTime);
	MCM_Settings.FETCH(fParrySlowTimeDuration);
	MCM_Settings.FETCH(fParrySlowTimePercentage);
	
	MCM_Settings.setActiveSection("Stun");
	MCM_Settings.FETCH(bStunToggle);
	MCM_Settings.FETCH(bStunMeterToggle);
	MCM_Settings.FETCH(bDownedStateToggle);
	MCM_Settings.FETCH(bPlayerStunToggle);
	MCM_Settings.FETCH(bPlayerExecutionToggle);
	MCM_Settings.FETCH(bFriendlyStunToggle);
	MCM_Settings.FETCH(fStunRegenDelaySetting);
	MCM_Settings.FETCH(bExecutionLimit);
	MCM_Settings.FETCH(fExecutionHealthThreshold);
	MCM_Settings.FETCH(fStunTimedBlockMult);
	MCM_Settings.FETCH(fStunBashMult);
	MCM_Settings.FETCH(fStunPowerBashMult);
	MCM_Settings.FETCH(fStunNormalAttackMult);
	MCM_Settings.FETCH(fStunPowerAttackMult);
	MCM_Settings.FETCH(fStunParryMult);
	MCM_Settings.FETCH(fStunCounterAttackMult);
	MCM_Settings.FETCH(fStunUnarmedMult);
	MCM_Settings.FETCH(fStunDaggerMult);
	MCM_Settings.FETCH(fStunSwordMult);
	MCM_Settings.FETCH(fStunWarAxeMult);
	MCM_Settings.FETCH(fStunMaceMult);
	MCM_Settings.FETCH(fStunGreatSwordMult);
	MCM_Settings.FETCH(fStun2HBluntMult);
	MCM_Settings.FETCH(bAutoExecution);
	MCM_Settings.FETCH(uExecutionKey);

	MCM_Settings.setActiveSection("Compatibility");
	MCM_Settings.FETCH(bPoiseCompatibility);
	MCM_Settings.FETCH(bUnblockableCompatibility);

	MCM_Settings.setActiveSection("Poise");
	MCM_Settings.FETCH(bPoise_PoiseToggle);
	MCM_Settings.FETCH(bPoise_PoiseMeterToggle);
	MCM_Settings.FETCH(bPoise_DebugConsoleLog);

	MCM_Settings.FETCH(fPoise_StaggerProtectTime);
	MCM_Settings.FETCH(fPoise_PoiseRegen);
	MCM_Settings.FETCH(fPoise_RegenDelayTime);
	MCM_Settings.FETCH(fPoise_DifficultyMultWeight);

	MCM_Settings.FETCH(fPoise_SmallStaggerTHLD);
	MCM_Settings.FETCH(fPoise_MediumStaggerTHLD);
	MCM_Settings.FETCH(fPoise_LargeStaggerTHLD);

	MCM_Settings.FETCH(fPoise_BaseMeleePoiseDamage);
	MCM_Settings.FETCH(fPoise_BaseRangePoiseDamage);
	MCM_Settings.FETCH(fPoise_BaseMagicPoiseDamage);

	MCM_Settings.FETCH(fPoise_BasePoiseHealth);
	MCM_Settings.FETCH(fPoise_HeavyArmorPoiseBonus);
	MCM_Settings.FETCH(fPoise_BaseArmorPoiseHealth);

	MCM_Settings.FETCH(fPoise_ScaleHealthMult);
	MCM_Settings.FETCH(fPoise_ScaleDamageMult);
	MCM_Settings.FETCH(fPoise_WeightHealthMult);
	MCM_Settings.FETCH(fPoise_WeightDamageMult);

	MCM_Settings.FETCH(fPoise_PowerAttackMult);
	MCM_Settings.FETCH(fPoise_BashMult);
	MCM_Settings.FETCH(fPoise_PowerBashMult);

	MCM_Settings.FETCH(fPoise_TargetPowerBashMult);
	MCM_Settings.FETCH(fPoise_TargetBashMult);
	MCM_Settings.FETCH(fPoise_TargetPowerAttackMult);
	MCM_Settings.FETCH(fPoise_TargetLightAttackMult);
	MCM_Settings.FETCH(fPoise_TargetBowDrawMult);
	MCM_Settings.FETCH(fPoise_TargetCastingMult);

	MCM_Settings.FETCH(fHandToHandMeleeMult);
	MCM_Settings.FETCH(fOneHandDaggerMult);
	MCM_Settings.FETCH(fOneHandSwordMult);
	MCM_Settings.FETCH(fOneHandAxeMult);
	MCM_Settings.FETCH(fOneHandMaceMult);
	MCM_Settings.FETCH(fTwoHandSwordMult);
	MCM_Settings.FETCH(fTwoHandAxeMult);
	MCM_Settings.FETCH(fBowMult);
	MCM_Settings.FETCH(fCrossbowMult);
	MCM_Settings.FETCH(fStaffMult);

	MCM_Settings.FETCH(fHeadMult);
	MCM_Settings.FETCH(fBodyMult);
	MCM_Settings.FETCH(fHandsMult);
	MCM_Settings.FETCH(fFeetMult);
	MCM_Settings.FETCH(fShieldMult);

	MCM_Settings.setActiveSection("AI");
	MCM_Settings.FETCH(bToggleAI);
	MCM_Settings.FETCH(bStopAIBash);
	MCM_Settings.FETCH(bStopAIBashPower);
	MCM_Settings.FETCH(bToggleDualWield);
	MCM_Settings.FETCH(bToggleUnarmed);
	MCM_Settings.FETCH(iMaxSimultaneousDefenders);

	MCM_Settings.FETCH(fParryAttemptChance);
	MCM_Settings.FETCH(fTimedBlockAttemptChance);
	MCM_Settings.FETCH(fPerfectBlockAttemptChance);

	MCM_Settings.FETCH(fConfidenceMult);
	MCM_Settings.FETCH(fConfidenceMin);
	MCM_Settings.FETCH(fConfidenceMax);

	MCM_Settings.FETCH(fSkillMult);
	MCM_Settings.FETCH(fSkillMin);
	MCM_Settings.FETCH(fSkillMax);

	MCM_Settings.FETCH(fTimedBlockCSMult);
	MCM_Settings.FETCH(fPerfectBlockCSMult);
	MCM_Settings.FETCH(fParryCSMult);
	MCM_Settings.FETCH(fTimedBlockSkillMult);
	MCM_Settings.FETCH(fPerfectBlockSkillMult);
	MCM_Settings.FETCH(fParrySkillMult);

	MCM_Settings.FETCH(fParryCooldown);
	MCM_Settings.FETCH(fBlockCooldown);

	MCM_Settings.FETCH(fBaseLightImpactTime);
	MCM_Settings.FETCH(fBasePowerImpactTime);

	MCM_Settings.FETCH(fmaxConeDegrees);
	MCM_Settings.FETCH(fbufferDistance);

	MCM_Settings.FETCH(fParryDelayOffset);
	MCM_Settings.FETCH(fPerfectBlockDelayOffset);
	MCM_Settings.FETCH(fTimedBlockDelayOffset);
	MCM_Settings.FETCH(fJitterScale);

	MCM_Settings.log();

	logger::info("Apply game settings...");

	setGameSettingf("fDamagedStaminaRegenDelay", fStaminaRegenDelay);
	setGameSettingf("fCombatStaminaRegenRateMult", fCombatStaminaRegenMult);

	offsetStaminaRegen(fStaminaRegenMult, fStaminaRegenLimit, fStaminaRegenMin);

	// Poise MAP PARSING INTEGRATION

	// Clear the maps on load/reload to prevent duplicate accumulation bloat
	armorSlotMultMap.clear();

	armorSlotMultMap[static_cast<uint32_t>(RE::BGSBipedObjectForm::BipedObjectSlot::kHead)] = fHeadMult;
	armorSlotMultMap[static_cast<uint32_t>(RE::BGSBipedObjectForm::BipedObjectSlot::kBody)] = fBodyMult;
	armorSlotMultMap[static_cast<uint32_t>(RE::BGSBipedObjectForm::BipedObjectSlot::kHands)] = fHandsMult;
	armorSlotMultMap[static_cast<uint32_t>(RE::BGSBipedObjectForm::BipedObjectSlot::kFeet)] = fFeetMult;
	armorSlotMultMap[static_cast<uint32_t>(RE::BGSBipedObjectForm::BipedObjectSlot::kShield)] = fShieldMult;

	logger::info("Valhalla Poise: Populated {} structural Armor Slot multipliers via native framework.", armorSlotMultMap.size());

	// Priority 1: Valhalla Stun Meter
	if (bStunMeterToggle && bStunToggle) {
		facts::TrueHudAPI_PoiseControlsSpecialBar = false;
		ValhallaCombat::GetSingleton()->requestTrueHudSpecialBarControl();
		logger::info("Valhalla Combat: TrueHUD Special Bar assigned to Stun System.");
	}
	// Priority 2: Valhalla Poise Meter
	else if (bPoise_PoiseMeterToggle && bPoise_PoiseToggle) {
		facts::TrueHudAPI_PoiseControlsSpecialBar = true;
		ValhallaCombat::GetSingleton()->requestTrueHudSpecialBarControl();
		logger::info("Valhalla Combat: TrueHUD Special Bar assigned to Poise System.");
	}
	// No meters active: Release control entirely
	else {
		facts::TrueHudAPI_PoiseControlsSpecialBar = false;
		ValhallaCombat::GetSingleton()->releaseTrueHudSpecialBarControl();
		logger::info("Valhalla Combat: TrueHUD Special Bar control released.");
	}
	logger::info("...done");
}

