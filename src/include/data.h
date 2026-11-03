#pragma once
#include "lib/robin_hood.h"
#include "lib/SimpleIni.h"
#pragma warning(disable: 4146)

/*Everything stored in game.*/
class data
{
public:
	static data* GetSingleton()
	{
		static data singleton;
		return  std::addressof(singleton);
	}
	/*Load all game data and store their pointers for later use.*/
	static void loadData();

	const static inline std::string AnimEvent_GuardCounter = "Val_GuardCounter_Trigger";
	const static inline std::string GraphBool_IsGuardCountering = "is_Guard_Countering";

#pragma region blockFX
	static inline RE::TESObjectACTI* BlockFX;
	static inline RE::Explosion* BlockSpark;
	static inline RE::Explosion* BlockSparkFlare;
	static inline RE::Explosion* BlockSparkRing;

#pragma endregion

#pragma region Perk
	static inline RE::BGSPerk* debuffPerk;
	static inline RE::BGSPerk* stunDebuffPerk;
#pragma endregion
	
#pragma region Spell
	static inline RE::SpellItem* exhaustionDebuffSpell_1;
	static inline RE::SpellItem* exhaustionDebuffSpell_2;
	static inline RE::SpellItem* stunDebuffSpell;
	static inline RE::SpellItem* timedBlockSpell;
	static inline RE::SpellItem* perfectBlockSpell;
	static inline RE::SpellItem* timedProjectileBlockSpell;
	static inline RE::SpellItem* tackleSpell;
	static inline RE::SpellItem* parrySpell;
	static inline RE::SpellItem* parryProjectileSpell;
	static inline RE::SpellItem* parryFollowupSpell;
	static inline RE::SpellItem* followupSpell;
	static inline RE::SpellItem* hyperArmorSpell;
	static inline RE::SpellItem* parrySASpell;
#pragma endregion

#pragma region Keyword
	static inline RE::BGSKeyword* executionImmunityKeyword = nullptr;
	static inline RE::BGSKeyword* Poise_ImmuneSmall = nullptr;
	static inline RE::BGSKeyword* Poise_ImmuneMedium = nullptr;
	static inline RE::BGSKeyword* Poise_ImmuneLarge = nullptr;
	static inline RE::BGSKeyword* Poise_ImmuneLargest = nullptr;

	static inline RE::BGSKeyword* npcKeyword = nullptr;
#pragma endregion

#pragma region Action
	inline static RE::BGSAction* actionStaggerStart = nullptr;
	inline static RE::BGSAction* actionRecoilLarge = nullptr;
	inline static RE::BGSAction* actionRightAttack = nullptr;
	inline static RE::BGSAction* actionRightPowerAttack = nullptr;
#pragma endregion

#pragma region Compatibility
	static inline bool bExhaustionInstalled = false;
	static inline RE::TESGlobal* incrementalFatigueCap = nullptr;
#pragma endregion

#pragma region ExecutionIdles
	using KM = std::vector<RE::TESIdleForm*>;
	static inline KM KM_Humanoid_H2H = std::vector<RE::TESIdleForm*>();
	static inline KM KM_Humanoid_Dagger = std::vector<RE::TESIdleForm*>();
	static inline KM KM_Humanoid_Sword = std::vector<RE::TESIdleForm*>();
	static inline KM KM_Humanoid_Axe = std::vector<RE::TESIdleForm*>();
	static inline KM KM_Humanoid_Mace = std::vector<RE::TESIdleForm*>();
	static inline KM KM_Humanoid_GreatSword = std::vector<RE::TESIdleForm*>();
	static inline KM KM_Humanoid_2hw = std::vector<RE::TESIdleForm*>();
	static inline KM KM_Humanoid_DW = std::vector<RE::TESIdleForm*>();
	static inline KM KM_Humanoid_1hm_Back = std::vector<RE::TESIdleForm*>();
	static inline KM KM_Humanoid_2hm_Back = std::vector<RE::TESIdleForm*>();
	static inline KM KM_Humanoid_H2H_Back = std::vector<RE::TESIdleForm*>();
	static inline KM KM_Humanoid_2hw_Back = std::vector<RE::TESIdleForm*>();
	static inline KM KM_Falmer_1hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Falmer_2hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Falmer_2hw= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Undead_1hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Undead_2hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Undead_2hw= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Spider_1hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Spider_2hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Spider_2hw= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Gargoyle_1hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Gargoyle_2hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Gargoyle_2hw= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Giant_1hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Giant_2hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Giant_2hw= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Bear_1hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Bear_2hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Bear_2hw= std::vector<RE::TESIdleForm*>();
	static inline KM KM_SabreCat_1hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_SabreCat_2hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_SabreCat_2hw= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Wolf_1hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Wolf_2hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Wolf_2hw= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Troll_1hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Troll_2hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Troll_2hw= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Hagraven_1hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Hagraven_2hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Hagraven_2hw= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Spriggan_1hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Spriggan_2hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Spriggan_2hw= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Boar_1hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Boar_2hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Boar_2hw= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Riekling_1hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Riekling_2hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Riekling_2hw= std::vector<RE::TESIdleForm*>();
	static inline KM KM_AshHopper_1hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_AshHopper_2hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_AshHopper_2hw= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Centurion_1hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Centurion_2hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Centurion_2hw= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Ballista_1hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Ballista_2hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Ballista_2hw= std::vector<RE::TESIdleForm*>();
	static inline KM KM_ChaurusFlyer_1hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_ChaurusFlyer_2hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_ChaurusFlyer_2hw= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Lurker_1hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Lurker_2hm= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Lurker_2hw= std::vector<RE::TESIdleForm*>();
	static inline KM KM_Dragon_1hm = std::vector<RE::TESIdleForm*>();
	static inline KM KM_Dragon_2hm = std::vector<RE::TESIdleForm*>();
	static inline KM KM_Dragon_2hw = std::vector<RE::TESIdleForm*>();

#pragma endregion
#pragma region ExecutionRaces
	enum raceCatagory
	{
		Humanoid = 0,
		Undead,
		Falmer,
		Spider,
		Gargoyle,
		Giant,
		Bear,
		SabreCat,
		Wolf,
		Troll,
		Hagraven,
		Spriggan,
		Boar,
		Riekling,
		AshHopper,
		SteamCenturion,
		DwarvenBallista,
		ChaurusFlyer,
		Lurker,
		Dragon
	};
	/*Mapping of all loaded races -> catagory of those races.*/
	//Note: catagorization of execution races has to be done manually.
	static inline robin_hood::unordered_map<RE::TESRace*, raceCatagory> raceMapping;

	/*Mapping of behavior graph project names (lowercase) -> catagory of those graphs.*/
	static inline robin_hood::unordered_map<std::string, raceCatagory> graphRaceMapping;

	/*Look up the actor from race mapping, and return whether the actor belongs to a race catagory.
	@param a_actor: actor whose race will be verified.
	@param a_catagory: race catagory to match.*/
	static bool isRaceType(RE::Actor* a_actor, raceCatagory a_catagory);
#pragma endregion
#pragma region sounds
	static inline RE::BGSSoundDescriptorForm* soundParryShield;
	static inline RE::BGSSoundDescriptorForm* soundParryWeapon;

	static inline RE::BGSSoundDescriptorForm* soundParryWeapon_perfect;
	static inline RE::BGSSoundDescriptorForm* soundParryShield_perfect;

	static inline RE::BGSSoundDescriptorForm* soundParryShield_gb;
	static inline RE::BGSSoundDescriptorForm* soundParryWeapon_gb;

	static inline RE::BGSSoundDescriptorForm* soundTackleStart;

	static inline RE::BGSSoundDescriptorForm* soundStunBreak;
	
	static inline RE::BGSSoundDescriptorForm* soundUnarmoredDodge;
	static inline RE::BGSSoundDescriptorForm* soundLightArmorDodge;
	static inline RE::BGSSoundDescriptorForm* soundMediumArmorDodge;
	static inline RE::BGSSoundDescriptorForm* soundHeavyArmorDodge;
	
#pragma endregion
#pragma region gameDifficultySettings
	static inline float fDiffMultHPByPCVE;
	static inline float fDiffMultHPByPCE;
	static inline float fDiffMultHPByPCN;
	static inline float fDiffMultHPByPCH;
	static inline float fDiffMultHPByPCVH;
	static inline float fDiffMultHPByPCL;
	static inline float fDiffMultHPToPCVE;
	static inline float fDiffMultHPToPCE;
	static inline float fDiffMultHPToPCN;
	static inline float fDiffMultHPToPCH;
	static inline float fDiffMultHPToPCVH;
	static inline float fDiffMultHPToPCL;
	
	static inline float fCombatHitConeAngle;
#pragma endregion
private:
	// Directory storing all executable race and the mapping to their killmoves.*/
	static inline const std::string kmRaceDir = "Data/SKSE/Plugins/ValhallaCombat/RaceMapping";
	// Path of file storing all killmoves.
	static inline const char* kmFilePath = "Data\\SKSE\\Plugins\\ValhallaCombat\\Killmoves.ini";
	// Path of file storing all settings.
	static inline const char* settingsFilePath = "Data\\SKSE\\Plugins\\ValhallaCombat\\Settings.ini";
	// Path of file storing all perk configurations.
	static inline const char* perksFilePath = "Data\\SKSE\\Plugins\\ValhallaCombat\\Perks.ini";
	// Path of file storing all poise configurations.
	static inline const char* poiseFilePath = "Data\\SKSE\\Plugins\\ValhallaCombat\\Poise.ini";
	// Path of file storing all animation configurations.
	static inline const char* animationFilePath = "Data\\SKSE\\Plugins\\ValhallaCombat\\Animation.ini";
	
	//Load all Valhalla Combat static INI settings
	static void loadINISettings();
	//Load all Valhalla Combat static INI perk lock settings
	static void loadINIPerks();

	/*Lookup an idle from a certain esp, and store it in an idleContainer. Errors if the idle does not exist.
	@param DATA: pointer to TESDataHandler's singleton.
	@param form: formID of the idle.
	@param pluginName: name of the plugin storing idle.
	@param idleContainer: Vector to store loaded idle.
	@return Whether the idle exists and therefore can be loaded or not.*/
	static bool lookupIdle(RE::TESDataHandler* DATA, RE::FormID form, std::string pluginName, std::vector<RE::TESIdleForm*>* idleContainer);

	/*Load a simpleIni section storing a set of idles belonging to that section.
	@param DATA: pointer to TESDataHandler's singleton.
	@param idleContainer: Vector to store loaded idles.
	@param ini: ini storing idle sections.
	@param section: a simpleIni's section, in string, to be loaded.*/
	static void loadIdleSection(RE::TESDataHandler* DATA, std::vector<RE::TESIdleForm*>* idleContainer,
		CSimpleIniA& ini, const char* section);

	/*Load all execution animations.*/
	static void loadIdle(RE::TESDataHandler* data);

	/*Read race from the plugin, and map it to an execution type through ExecutionRaceMap.
	@param DATA: pointer to TESDataHandler's singleton.
	@param form: formID of the race.
	@param pluginName: name of the plugin storing the race.
	@param raceCatagory: type of the execution race to be mapped to.
	@return If the race form exists or not.*/
	static bool pairUpRace(RE::TESDataHandler* DATA, RE::FormID form, std::string pluginName, raceCatagory raceType);

	/*Load a simpleIni section storing a set of races belonging to that section's corresponding killmove.
	@param DATA: pointer to TESDataHandler's singleton.
	@param raceType: type of the execution race to be mapped to.
	@param ini: ini whose section to load.
	@param section: a simpleIni's section, in string, to be loaded.*/
	static void loadRaceSection(RE::TESDataHandler* DATA, raceCatagory raceType, CSimpleIniA& ini, const char* section);

	/*Load a simpleIni file storing mapping of executable races.*/
	static void loadExecutableRaceIni(RE::TESDataHandler* DATA, const char* ini_path);

	/*Load all executable races from all ini files by iterating over them.*/
	static void loadExecutableRace(RE::TESDataHandler* data);

	/*Load all sounds from game.*/
	static void loadSound(RE::TESDataHandler* data);

	/*Load all perks from game.*/
	static void loadPerk(RE::TESDataHandler* data);

	/*Load all spells from game.*/
	static void loadSpell(RE::TESDataHandler* data);

	/*Load all Keywords from game.*/
	static void loadKeyword(RE::TESDataHandler* data);

	/*Load all Actions from game.*/
	static void loadAction(RE::TESDataHandler* a_data);

	/*Load all difficulty multiplier from game.*/
	static void loadDifficultySettings(RE::GameSettingCollection* gameSettings);

	static void loadBlockFX(RE::TESDataHandler* data);


};

namespace inlineUtils
{
	/*Parse a string into a vector of tokens.*/
	inline std::vector<std::string> tokenize(const std::string& delimiter, const std::string& str) {
		std::vector<std::string> arr;
		size_t strleng = str.length();
		size_t delleng = delimiter.length();
		if (delleng == 0)
			return arr;//no change

		size_t i = 0;
		size_t k = 0;
		while (i < strleng)
		{
			int j = 0;
			while (i + j < strleng && j < delleng && str[i + j] == delimiter[j])
				j++;
			if (j == delleng)//found delimiter
			{
				arr.push_back(str.substr(k, i - k));
				i += delleng;
				k = i;
			}
			else
			{
				i++;
			}
		}
		arr.push_back(str.substr(k, i - k));
		return arr;
	}

	/*Turn a formID into a uint32. Useful when reading from ini.*/
	inline bool ToInt32(std::string str, int& value)
	{
		const char* strVal = str.c_str();
		char* endVal = NULL;
		long ret = strtol(strVal, &endVal, 0);
		if (ret == LONG_MAX || ret == LONG_MIN || endVal != strVal + strlen(strVal))
			return false;
		value = static_cast<int>(ret);
		return true;
	}
}
