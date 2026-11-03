#include "include/data.h"
#include "include/Utils.h"
#include "include/settings.h"
#include "include/perks.h"

using namespace inlineUtils;
void data::loadData() {
	logger::info("Loading data from game...");
	auto data = RE::TESDataHandler::GetSingleton();
	if (!data) {
		logger::critical("Error: TESDataHandler not found.");
		return;
	}
	loadBlockFX(data);
	loadSound(data);
	loadPerk(data);
	loadSpell(data);
	loadKeyword(data);
	loadAction(data);
	loadIdle(data);
	loadExecutableRace(data);
	loadINISettings();
	loadINIPerks();

	auto gameSettings = RE::GameSettingCollection::GetSingleton();
	if (!gameSettings) {
		logger::critical("Error: GameSettingCollection not found.");
		return;
	}
	loadDifficultySettings(gameSettings);

	logger::info("Data fetched.");
	
}

void data::loadSound(RE::TESDataHandler* a_data) {
	(void)a_data;
	logger::info("Loading sound descriptors...");
	DtryUtils::formLoader loader("ValhallaCombat.esp");
	loader.load(soundParryShield, 0x433c);
	loader.load(soundParryShield_perfect, 0x60c2e);
	loader.load(soundParryShield_gb, 0X47720);
	loader.load(soundParryWeapon, 0X3DD9);
	loader.load(soundParryWeapon_perfect, 0X60C2D);
	loader.load(soundParryWeapon_gb, 0X47721);
	loader.load(soundStunBreak, 0X56A22);
	loader.load(soundTackleStart, 0x60c36);
	loader.load(soundUnarmoredDodge, 0x60C73);
	loader.load(soundLightArmorDodge, 0x60C74);
	loader.load(soundMediumArmorDodge, 0x60C75);
	loader.load(soundHeavyArmorDodge, 0x60C76);
	loader.log();
	logger::info("...done");
}

void data::loadPerk(RE::TESDataHandler* a_data) {
	logger::info("Loading perk...");
	DtryUtils::formLoader loader("ValhallaCombat.esp");

	loader.load(debuffPerk, 0x2DB2);
	loader.load(stunDebuffPerk, 0x060C4B);

	incrementalFatigueCap = a_data->LookupForm<RE::TESGlobal>(0x0802, "exhaustion_incremental_Fatigue.esp");

	bExhaustionInstalled = (incrementalFatigueCap != nullptr);

	logger::info("Exhaustion Incremental Fatigue {}", bExhaustionInstalled ? "detected" : "not detected");

	logger::info("...done");
}

void data::loadSpell(RE::TESDataHandler* a_data)
{
	(void)a_data;

	logger::info("Loading spells...");

	DtryUtils::formLoader loader("ValhallaCombat.esp");
	
	loader.load(exhaustionDebuffSpell_1, 0x001827);
	loader.load(exhaustionDebuffSpell_2, 0x060C56);
	loader.load(stunDebuffSpell, 0x060C48);
	loader.load(timedBlockSpell, 0x060C5A);
	loader.load(perfectBlockSpell, 0x060C5B);
	loader.load(tackleSpell, 0x060C5C);
	loader.load(parrySpell, 0x060C5D);
	loader.load(timedProjectileBlockSpell, 0x060C5E);
	loader.load(parryProjectileSpell, 0x060C5F);
	loader.load(parryFollowupSpell, 0x060C6C);
	loader.load(followupSpell, 0x060C6D);
	loader.load(hyperArmorSpell, 0x060C6E);
	loader.load(parrySASpell, 0x060C6F);

	logger::info("...done");
}

void data::loadKeyword(RE::TESDataHandler* a_data)
{
	(void)a_data;

	logger::info("Loading keywords...");

	{
		DtryUtils::formLoader loader("ValhallaCombat.esp");

		loader.load(executionImmunityKeyword, 0x060C38);
		loader.load(Poise_ImmuneSmall, 0x060C51);
		loader.load(Poise_ImmuneMedium, 0x060C52);
		loader.load(Poise_ImmuneLarge, 0x060C53);
		loader.load(Poise_ImmuneLargest, 0x060C54);
	}

	{
		DtryUtils::formLoader loader("Skyrim.esm");
		loader.load(npcKeyword, 0x13794);
	}

	logger::info("...done");
}

void data::loadAction(RE::TESDataHandler* a_data)
{
	(void)a_data;

	logger::info("Loading actions...");

	DtryUtils::formLoader loader("Skyrim.esm");
	loader.load(actionStaggerStart, 0x138D2);
	loader.load(actionRecoilLarge, 0x13EC8);
	loader.load(actionRightAttack, 0x13005);
	loader.load(actionRightPowerAttack, 0x13383);

	logger::info("...done");
}

void data::loadBlockFX(RE::TESDataHandler* a_data) {
	(void)a_data;
	logger::info("Loading block FX...");
	DtryUtils::formLoader loader("ValhallaCombat.esp");
	loader.load(BlockFX, 0x60c2f);
	loader.load(BlockSpark, 0x60c30);
	loader.load(BlockSparkFlare, 0x60c31);
	loader.load(BlockSparkRing, 0x60c32);
	loader.log();
	logger::info("...done");
}


bool data::lookupIdle(RE::TESDataHandler* data, RE::FormID form, std::string pluginName, 
	std::vector<RE::TESIdleForm*>* idleContainer) {
	auto idle = data->LookupForm<RE::TESIdleForm>(form, pluginName);
	if (!idle) {
		return false;
	}
	idleContainer->push_back(idle);
	return true;
}

void data::loadIdleSection(RE::TESDataHandler* a_data, std::vector<RE::TESIdleForm*>* idleContainer, 
	CSimpleIniA& ini, const char* section) {
	//logger::info("Loading from section {}", section);
	CSimpleIniA::TNamesDepend keys;
	ini.GetAllKeys(section, keys);
	int idlesLoaded = 0;
	for (CSimpleIniA::TNamesDepend::iterator s_it1 = keys.begin(); s_it1 != keys.end(); s_it1++) {
		const char* idle = s_it1->pItem;
		auto line = ini.GetValue(section, idle);
		std::vector<std::string> idleConfigs = tokenize("|", line);
		if (idleConfigs.size() != 2) {
			//ERROR("Error: wrong config length");
			continue;
		}
		std::string plugin = idleConfigs[0];
		int form = 0;
		if (!inlineUtils::ToInt32(idleConfigs[1], form)) {
			//ERROR("Error: wrong formID");
			continue;
		}
		if (lookupIdle(a_data, form, plugin, idleContainer)) {
			idlesLoaded++;
			//logger::info("Loaded idle. FormID: {}, plugin: {}", form, plugin);
		}
		else {
			logger::info("Error: Failed to load idle. FormID: {}, plugin: {}", form, plugin);
		}

	}
	logger::info("Loaded {} idles from section {}.", idlesLoaded, section);
}

void data::loadIdle(RE::TESDataHandler* a_actor) {
	(void)a_actor;
	CSimpleIniA ini;
	ini.LoadFile(kmFilePath);
	logger::info("Loading idle...");
	auto DATA = RE::TESDataHandler::GetSingleton();
	loadIdleSection(DATA, &KM_Humanoid_H2H, ini ,"Humanoid-Unarmed");
	loadIdleSection(DATA, &KM_Humanoid_Dagger, ini, "Humanoid-Dagger");
	loadIdleSection(DATA, &KM_Humanoid_Sword, ini, "Humanoid-Sword");
	loadIdleSection(DATA, &KM_Humanoid_Axe, ini, "Humanoid-Axe");
	loadIdleSection(DATA, &KM_Humanoid_Mace, ini, "Humanoid-Mace");
	loadIdleSection(DATA, &KM_Humanoid_GreatSword, ini, "Humanoid-GreatSword");
	loadIdleSection(DATA, &KM_Humanoid_2hw, ini, "Humanoid-2HW");

	loadIdleSection(DATA, &KM_Humanoid_DW, ini, "Humanoid-DW");

	loadIdleSection(DATA, &KM_Humanoid_1hm_Back, ini, "Humanoid-1HM-Back");
	loadIdleSection(DATA, &KM_Humanoid_2hm_Back, ini, "Humanoid-2HM-Back");
	loadIdleSection(DATA, &KM_Humanoid_2hw_Back, ini, "Humanoid-2HW-Back");
	loadIdleSection(DATA, &KM_Humanoid_H2H_Back, ini, "Humanoid-Unarmed-Back");

	loadIdleSection(DATA, &KM_Undead_1hm, ini, "Undead-1HM");
	loadIdleSection(DATA, &KM_Undead_2hm, ini, "Undead-2HM");
	loadIdleSection(DATA, &KM_Undead_2hw, ini, "Undead-2HW");

	loadIdleSection(DATA, &KM_Falmer_1hm, ini, "Falmer-1HM");
	loadIdleSection(DATA, &KM_Falmer_2hm, ini, "Falmer-2HM");
	loadIdleSection(DATA, &KM_Falmer_2hw, ini, "Falmer-2HW");


	loadIdleSection(DATA, &KM_Spider_1hm, ini, "Spider-1HM");
	loadIdleSection(DATA, &KM_Spider_2hm, ini, "Spider-2HM");
	loadIdleSection(DATA, &KM_Spider_2hw, ini, "Spider-2HW");

	loadIdleSection(DATA, &KM_Gargoyle_1hm, ini, "Gargoyle-1HM");
	loadIdleSection(DATA, &KM_Gargoyle_2hm, ini, "Gargoyle-2HM");
	loadIdleSection(DATA, &KM_Gargoyle_2hw, ini, "Gargoyle-2HW");

	loadIdleSection(DATA, &KM_Giant_1hm, ini, "Giant-1HM");
	loadIdleSection(DATA, &KM_Giant_2hm, ini, "Giant-2HM");
	loadIdleSection(DATA, &KM_Giant_2hw, ini, "Ginat-2HW");

	loadIdleSection(DATA, &KM_Bear_1hm, ini, "Bear-1HM");
	loadIdleSection(DATA, &KM_Bear_2hm, ini, "Bear-2HM");
	loadIdleSection(DATA, &KM_Bear_2hw, ini, "Bear-2HW");

	loadIdleSection(DATA, &KM_SabreCat_1hm, ini, "SabreCat-1HM");
	loadIdleSection(DATA, &KM_SabreCat_2hm, ini, "SabreCat-2HM");
	loadIdleSection(DATA, &KM_SabreCat_2hw, ini, "SabreCat-2HW");

	loadIdleSection(DATA, &KM_Wolf_1hm, ini, "Wolf-1HM");
	loadIdleSection(DATA, &KM_Wolf_2hm, ini, "Wolf-2HM");
	loadIdleSection(DATA, &KM_Wolf_2hw, ini, "Wolf-2HW");

	loadIdleSection(DATA, &KM_Troll_1hm, ini, "Troll-1HM");
	loadIdleSection(DATA, &KM_Troll_2hm, ini, "Troll-2HM");
	loadIdleSection(DATA, &KM_Troll_2hw, ini, "Troll-2HW");


	loadIdleSection(DATA, &KM_Hagraven_1hm, ini, "Hagraven-1HM");
	loadIdleSection(DATA, &KM_Hagraven_2hm, ini, "Hagraven-2HM");
	loadIdleSection(DATA, &KM_Hagraven_2hw, ini, "Hagraven-2HW");

	loadIdleSection(DATA, &KM_Spriggan_1hm, ini, "Spriggan-1HM");
	loadIdleSection(DATA, &KM_Spriggan_2hm, ini, "Spriggan-2HM");
	loadIdleSection(DATA, &KM_Spriggan_2hw, ini, "Spriggan-2HW");

	loadIdleSection(DATA, &KM_Boar_1hm, ini, "Boar-1HM");
	loadIdleSection(DATA, &KM_Boar_2hm, ini, "Boar-2HM");
	loadIdleSection(DATA, &KM_Boar_2hw, ini, "Boar-2HW");

	loadIdleSection(DATA, &KM_Riekling_1hm, ini, "Riekling-1HM");
	loadIdleSection(DATA, &KM_Riekling_2hm, ini, "Riekling-2HM");
	loadIdleSection(DATA, &KM_Riekling_2hw, ini, "Riekling-2HW");

	loadIdleSection(DATA, &KM_AshHopper_1hm, ini, "AshHopper-1HM");
	loadIdleSection(DATA, &KM_AshHopper_2hm, ini, "AshHopper-2HM");
	loadIdleSection(DATA, &KM_AshHopper_2hw, ini, "AshHopper-2HW");

	loadIdleSection(DATA, &KM_Ballista_1hm, ini, "DwarvenBallista-1HM");
	loadIdleSection(DATA, &KM_Ballista_2hm, ini, "DwarvenBallista-2HM");
	loadIdleSection(DATA, &KM_Ballista_2hw, ini, "DwarvenBallista-2HW");

	loadIdleSection(DATA, &KM_Centurion_1hm, ini, "SteamCenturion-1HM");
	loadIdleSection(DATA, &KM_Centurion_2hm, ini, "SteamCenturion-2HM");
	loadIdleSection(DATA, &KM_Centurion_2hw, ini, "SteamCenturion-2HW");

	loadIdleSection(DATA, &KM_ChaurusFlyer_1hm, ini, "ChaurusFlyer-1HM");
	loadIdleSection(DATA, &KM_ChaurusFlyer_2hm, ini, "ChaurusFlyer-2HM");
	loadIdleSection(DATA, &KM_ChaurusFlyer_2hw, ini, "ChaurusFlyer-2HW");


	loadIdleSection(DATA, &KM_Lurker_1hm, ini, "Lurker-1HM");
	loadIdleSection(DATA, &KM_Lurker_2hm, ini, "Lurker-2HM");
	loadIdleSection(DATA, &KM_Lurker_2hw, ini, "Lurker-2HW");


	loadIdleSection(DATA, &KM_Dragon_1hm, ini, "Dragon-1HM");
	loadIdleSection(DATA, &KM_Dragon_2hm, ini, "Dragon-2HM");
	loadIdleSection(DATA, &KM_Dragon_2hw, ini, "Dragon-2HW");
	logger::info("Idle loaded.");
}

bool data::pairUpRace(RE::TESDataHandler* a_actor, RE::FormID a_formID, std::string a_plugin, raceCatagory a_raceType) {
	auto race = a_actor->LookupForm<RE::TESRace>(a_formID, a_plugin);
	if (!race) {
		return false;
	}
	raceMapping.emplace(race, a_raceType);
	//logger::info("Mapped {} from {} to race catagory {}", race->GetName(), pluginName, raceType);
	return true;
}

void data::loadRaceSection(RE::TESDataHandler* a_data, raceCatagory a_raceType, CSimpleIniA& a_ini, const char* a_section) {
	CSimpleIniA::TNamesDepend keys;
	a_ini.GetAllKeys(a_section, keys);
	int raceLoaded = 0;
	int graphLoaded = 0;

	for (CSimpleIniA::TNamesDepend::iterator s_it1 = keys.begin(); s_it1 != keys.end(); s_it1++) {
		const char* key = s_it1->pItem;
		auto line = a_ini.GetValue(a_section, key);
		if (!line) {
			continue;
		}

		std::string lineStr = line;
		// Trim whitespace
		lineStr.erase(0, lineStr.find_first_not_of(" \t\r\n"));
		lineStr.erase(lineStr.find_last_not_of(" \t\r\n") + 1);

		if (lineStr.empty()) {
			continue;
		}

		std::vector<std::string> raceConfigs = tokenize("|", lineStr);

		if (raceConfigs.size() == 2) {
			// Race Form ID entry (e.g. Skyrim.esm|0x131F0)
			std::string plugin = raceConfigs[0];
			int form = 0;
			if (!inlineUtils::ToInt32(raceConfigs[1], form)) {
				logger::critical("Error: wrong formID in line {}", lineStr);
				continue;
			}
			if (pairUpRace(a_data, form, plugin, a_raceType)) {
				raceLoaded++;
			}
			else {
				logger::info("Failed to load race. FormID: {}, plugin: {}", form, plugin);
			}
		}
		else if (raceConfigs.size() == 1) {
			// Behavior graph project name entry (e.g. FalmerProject)
			std::string projName = inlineUtils::to_lower_string(lineStr);

			graphRaceMapping[projName] = a_raceType;
			graphLoaded++;
		}
	}
	logger::info("Loaded {} races and {} graph project mappings from section {}.", raceLoaded, graphLoaded, a_section);
}

void data::loadExecutableRaceIni(RE::TESDataHandler* a_data, const char* a_path) {
	CSimpleIniA ini;
	ini.LoadFile(a_path);
	loadRaceSection(a_data, raceCatagory::Humanoid, ini, "Humanoid");
	loadRaceSection(a_data, raceCatagory::Undead, ini, "Undead");
	loadRaceSection(a_data, raceCatagory::Falmer, ini, "Falmer");
	loadRaceSection(a_data, raceCatagory::Spider, ini, "Spider");
	loadRaceSection(a_data, raceCatagory::Gargoyle, ini, "Gargoyle");
	loadRaceSection(a_data, raceCatagory::Giant, ini, "Giant");
	loadRaceSection(a_data, raceCatagory::Bear, ini, "Bear");
	loadRaceSection(a_data, raceCatagory::SabreCat, ini, "SabreCat");
	loadRaceSection(a_data, raceCatagory::Wolf, ini, "Wolf");
	loadRaceSection(a_data, raceCatagory::Troll, ini, "Troll");
	loadRaceSection(a_data, raceCatagory::Hagraven, ini, "Hagraven");
	loadRaceSection(a_data, raceCatagory::Spriggan, ini, "Spriggan");
	loadRaceSection(a_data, raceCatagory::Boar, ini, "Boar");
	loadRaceSection(a_data, raceCatagory::Riekling, ini, "Riekling");
	loadRaceSection(a_data, raceCatagory::AshHopper, ini, "AshHopper");
	loadRaceSection(a_data, raceCatagory::SteamCenturion, ini, "SteamCenturion");
	loadRaceSection(a_data, raceCatagory::DwarvenBallista, ini, "DwarvenBallista");
	loadRaceSection(a_data, raceCatagory::ChaurusFlyer, ini, "ChaurusFlyer");
	loadRaceSection(a_data, raceCatagory::Lurker, ini, "Lurker");
	loadRaceSection(a_data, raceCatagory::Dragon, ini, "Dragon");
}

void data::loadExecutableRace(RE::TESDataHandler* a_data) {
	for (const auto& entry : std::filesystem::directory_iterator(kmRaceDir)) { //iterates through all .ini files
		std::string pathStr = entry.path().string();
		logger::info("Reading race config from {}", pathStr);
		const char* cstr = pathStr.c_str();
		loadExecutableRaceIni(a_data, cstr);
	}
}

void data::loadDifficultySettings(RE::GameSettingCollection* a_gameSettingCollection) {
	logger::info("Loading difficulty settings...");
	fDiffMultHPByPCVE = a_gameSettingCollection->GetSetting("fDiffMultHPByPCVE")->GetFloat();
	fDiffMultHPByPCE = a_gameSettingCollection->GetSetting("fDiffMultHPByPCE")->GetFloat();
	fDiffMultHPByPCN = a_gameSettingCollection->GetSetting("fDiffMultHPByPCN")->GetFloat();
	fDiffMultHPByPCH = a_gameSettingCollection->GetSetting("fDiffMultHPByPCH")->GetFloat();
	fDiffMultHPByPCVH = a_gameSettingCollection->GetSetting("fDiffMultHPByPCVH")->GetFloat();
	fDiffMultHPByPCL = a_gameSettingCollection->GetSetting("fDiffMultHPByPCL")->GetFloat();

	fDiffMultHPToPCVE = a_gameSettingCollection->GetSetting("fDiffMultHPToPCVE")->GetFloat();
	fDiffMultHPToPCE = a_gameSettingCollection->GetSetting("fDiffMultHPToPCE")->GetFloat();
	fDiffMultHPToPCN = a_gameSettingCollection->GetSetting("fDiffMultHPToPCN")->GetFloat();
	fDiffMultHPToPCH = a_gameSettingCollection->GetSetting("fDiffMultHPToPCH")->GetFloat();
	fDiffMultHPToPCVH = a_gameSettingCollection->GetSetting("fDiffMultHPToPCVH")->GetFloat();
	fDiffMultHPToPCL = a_gameSettingCollection->GetSetting("fDiffMultHPToPCL")->GetFloat();

	fCombatHitConeAngle = a_gameSettingCollection->GetSetting("fCombatHitConeAngle")->GetFloat();
	logger::info("...done");
}

bool data::isRaceType(RE::Actor* a_actor, raceCatagory a_catagory) {
	auto it = raceMapping.find(a_actor->GetRace());
	if (it != raceMapping.end()) {
		return it->second == a_catagory;
	}
	else {
		return false;
	}
}

void data::loadINISettings() {
	logger::info("Valhalla Combat: Loading static INI settings...");

	CSimpleIniA rawIni;
	rawIni.SetUnicode();
	if (rawIni.LoadFile(settingsFilePath) < 0) {
		logger::warn("Valhalla Combat: Settings INI not found or failed to load at path: {}", settingsFilePath);
	}

	CSimpleIniA poiseIni;
	poiseIni.SetUnicode();
	if (poiseIni.LoadFile(poiseFilePath) < 0) {
		logger::warn("Valhalla Combat: Poise INI not found or failed to load at path: {}", poiseFilePath);
	}

	CSimpleIniA animIni;
	animIni.SetUnicode();
	if (animIni.LoadFile(animationFilePath) < 0) {
		logger::warn("Valhalla Combat: Animation INI not found or failed to load at path: {}", animationFilePath);
	}

	auto dataHandler = RE::TESDataHandler::GetSingleton();
	if (!dataHandler) {
		logger::error("Valhalla Combat: TESDataHandler not available.");
		return;
	}

	// Helper to parse key format with strict load-order & existence verification
	auto parseFormIDKey = [dataHandler](const char* a_keyStr) -> RE::FormID {
		if (!a_keyStr || strlen(a_keyStr) == 0) return 0;

		std::vector<std::string> tokens = inlineUtils::tokenize("|", a_keyStr);
		if (tokens.size() == 2) {
			std::string plugin = tokens[0];

			// Verify plugin is loaded in active load order
			if (!dataHandler->LookupLoadedModByName(plugin)) {
				logger::warn("Valhalla Combat: Plugin [{}] is NOT loaded in current load order! (Key: {})", plugin, a_keyStr);
				return 0;
			}

			try {
				RE::FormID rawFormID = static_cast<RE::FormID>(std::stoul(tokens[1], nullptr, 16));
				RE::FormID resolvedFormID = dataHandler->LookupFormID(rawFormID & 0x00FFFFFF, plugin);

				// Verify form exists in game memory
				if (resolvedFormID == 0 || !RE::TESForm::LookupByID(resolvedFormID)) {
					logger::warn("Valhalla Combat: FormID [0x{:X}] does not exist in [{}]! (Key: {})", rawFormID & 0x00FFFFFF, plugin, a_keyStr);
					return 0;
				}

				return resolvedFormID;
			}
			catch (...) {
				logger::error("Valhalla Combat: Failed to parse hex offset [{}] (Key: {})", tokens[1], a_keyStr);
				return 0;
			}
		}
		else if (tokens.size() == 1) {
			// Fallback parsing for raw FormID strings (e.g., "0x000B7998")
			try {
				RE::FormID formID = static_cast<RE::FormID>(std::stoul(tokens[0], nullptr, 16));
				if (!RE::TESForm::LookupByID(formID)) {
					logger::warn("Valhalla Combat: Raw FormID [0x{:X}] does not exist in active game!", formID);
					return 0;
				}
				return formID;
			}
			catch (...) {
				logger::error("Valhalla Combat: Failed to parse legacy FormID key: {}", tokens[0]);
				return 0;
			}
		}
		return 0;
		};

	// Parse Poise Damage Mult Overrides
	CSimpleIniA::TNamesDepend itemKeys;
	poiseIni.GetAllKeys("PoiseDamageMultOverrides", itemKeys);
	for (const auto& key : itemKeys) {
		try {
			RE::FormID formId = parseFormIDKey(key.pItem);
			float value = static_cast<float>(poiseIni.GetDoubleValue("PoiseDamageMultOverrides", key.pItem, 0.0));

			if (formId > 0) {
				settings::poiseDamageMultOverrideMap[formId] = value;
				logger::info("Valhalla Poise: Registered Poise Damage Mult Override [{}] -> Value: {}", key.pItem, value);
			}
		}
		catch (...) {
			logger::error("Valhalla Poise: Failed to parse poise damage mult override key: {}", key.pItem);
		}
	}

	// Parse Actor/Race Specific Base Poise Damage Overrides
	CSimpleIniA::TNamesDepend baseDmgKeys;
	poiseIni.GetAllKeys("PoiseBaseDamageOverrides", baseDmgKeys);
	for (const auto& key : baseDmgKeys) {
		try {
			RE::FormID formId = parseFormIDKey(key.pItem);
			float baseDmgValue = static_cast<float>(poiseIni.GetDoubleValue("PoiseBaseDamageOverrides", key.pItem, 0.0));

			if (formId > 0) {
				settings::poiseBaseDamageOverrideMap[formId] = baseDmgValue;
				logger::info("Valhalla Poise: Registered Actor/Race Base Damage Override [{}] -> Value: {}", key.pItem, baseDmgValue);
			}
		}
		catch (...) {
			logger::error("Valhalla Poise: Failed to parse base damage override key: {}", key.pItem);
		}
	}

	// Parse Actor/Race Specific Poise Health Pools
	CSimpleIniA::TNamesDepend actorKeys;
	poiseIni.GetAllKeys("PoiseHealthOverrides", actorKeys);
	for (const auto& key : actorKeys) {
		try {
			RE::FormID formId = parseFormIDKey(key.pItem);
			float healthValue = static_cast<float>(poiseIni.GetDoubleValue("PoiseHealthOverrides", key.pItem, 0.0));

			if (formId > 0) {
				settings::poiseHealthOverrideMap[formId] = healthValue;
				logger::info("Valhalla Poise: Registered Actor/Race Health Override [{}] -> Value: {}", key.pItem, healthValue);
			}
		}
		catch (...) {
			logger::error("Valhalla Poise: Failed to parse actor/race override key: {}", key.pItem);
		}
	}

	// Parse Behavior Graph Health Overrides
	CSimpleIniA::TNamesDepend graphKeys;
	poiseIni.GetAllKeys("PoiseHealthGraphOverrides", graphKeys);
	for (const auto& key : graphKeys) {
		try {
			std::string graphName = inlineUtils::to_lower_string(key.pItem ? key.pItem : "");
			float healthValue = static_cast<float>(poiseIni.GetDoubleValue("PoiseHealthGraphOverrides", key.pItem, 0.0));

			if (!graphName.empty() && healthValue > 0.0f) {
				settings::poiseHealthGraphOverrideMap[graphName] = healthValue;
				logger::info("Valhalla Poise: Registered Behavior Graph Health Override [{}] -> Value: {}", key.pItem, healthValue);
			}
			else {
				logger::warn("Valhalla Poise: Invalid Behavior Graph entry or zero value for: {}", key.pItem);
			}
		}
		catch (...) {
			logger::error("Valhalla Poise: Failed to parse behavior graph override key: {}", key.pItem);
		}
	}

	// Parse Behavior Graph Base Damage Overrides
	CSimpleIniA::TNamesDepend graphDmgKeys;
	poiseIni.GetAllKeys("PoiseBaseDamageGraphOverrides", graphDmgKeys);
	for (const auto& key : graphDmgKeys) {
		try {
			std::string graphName = inlineUtils::to_lower_string(key.pItem ? key.pItem : "");
			float baseDmgValue = static_cast<float>(poiseIni.GetDoubleValue("PoiseBaseDamageGraphOverrides", key.pItem, 0.0));

			if (!graphName.empty() && baseDmgValue > 0.0f) {
				settings::poiseBaseDamageGraphOverrideMap[graphName] = baseDmgValue;
				logger::info("Valhalla Poise: Registered Graph Base Damage Override [{}] -> Value: {}", key.pItem, baseDmgValue);
			}
			else {
				logger::warn("Valhalla Poise: Invalid Graph Base Damage entry or zero value for: {}", key.pItem);
			}
		}
		catch (...) {
			logger::error("Valhalla Poise: Failed to parse graph base damage override key: {}", key.pItem);
		}
	}

	logger::info("Valhalla Poise: Successfully mapped {} poise damage mult, {} base damage overrides, {} actor/race health profiles, {} graph health overrides, and {} graph damage overrides.",
		settings::poiseDamageMultOverrideMap.size(),
		settings::poiseBaseDamageOverrideMap.size(),
		settings::poiseHealthOverrideMap.size(),
		settings::poiseHealthGraphOverrideMap.size(),
		settings::poiseBaseDamageGraphOverrideMap.size());

	// Reaction Type Values
	settings::fReactionTypeSmallTHLD = static_cast<float>(rawIni.GetDoubleValue("ReactionSettings", "fReactionTypeSmallTHLD", settings::fReactionTypeSmallTHLD));
	settings::fReactionTypeMediumTHLD = static_cast<float>(rawIni.GetDoubleValue("ReactionSettings", "fReactionTypeMediumTHLD", settings::fReactionTypeMediumTHLD));
	settings::fReactionTypeLargeTHLD = static_cast<float>(rawIni.GetDoubleValue("ReactionSettings", "fReactionTypeLargeTHLD", settings::fReactionTypeLargeTHLD));
	settings::fReactionTypeLargestTHLD = static_cast<float>(rawIni.GetDoubleValue("ReactionSettings", "fReactionTypeLargestTHLD", settings::fReactionTypeLargestTHLD));

	logger::info("Valhalla Poise: Loaded Reaction Settings -> Small: {}, Medium: {}, Large: {}, Largest: {}",
		settings::fReactionTypeSmallTHLD, settings::fReactionTypeMediumTHLD, settings::fReactionTypeLargeTHLD, settings::fReactionTypeLargestTHLD);

	// Dynamic Animation Event Mapping
	CSimpleIniA::TNamesDepend mappingKeys;
	if (animIni.GetAllKeys("AnimEventMapping", mappingKeys)) {
		auto trim = [](std::string& s) {
			s.erase(0, s.find_first_not_of(" \t\r\n"));
			s.erase(s.find_last_not_of(" \t\r\n") + 1);
			};

		for (const auto& key : mappingKeys) {
			std::string fullKey = key.pItem;
			size_t colonPos = fullKey.find(':');

			if (colonPos != std::string::npos) {
				std::string rawEventName = fullKey.substr(0, colonPos);
				std::string varName = fullKey.substr(colonPos + 1);

				trim(rawEventName);
				trim(varName);

				// 1. Lowercase using inline utility
				std::string lookupKey = inlineUtils::to_lower_string(rawEventName);

				// 2. Normalize spaces to dots (e.g., "pie $backpower" -> "pie.$backpower")
				std::replace(lookupKey.begin(), lookupKey.end(), ' ', '.');

				auto& eventData = settings::animEventDataMap[lookupKey];

				if (varName == "Val_PoiseBreaksBlock") {
					eventData.bPoiseBreaksBlock = animIni.GetBoolValue("AnimEventMapping", fullKey.c_str(), eventData.bPoiseBreaksBlock);
				}
				else if (varName == "Val_Unblockable") {
					eventData.bUnblockable = animIni.GetBoolValue("AnimEventMapping", fullKey.c_str(), eventData.bUnblockable);
				}
				else if (varName == "Val_StaminaMult") {
					eventData.fStaminaMult = static_cast<float>(animIni.GetDoubleValue("AnimEventMapping", fullKey.c_str(), eventData.fStaminaMult));
				}
				else if (varName == "Val_PoiseMult") {
					eventData.fPoiseMult = static_cast<float>(animIni.GetDoubleValue("AnimEventMapping", fullKey.c_str(), eventData.fPoiseMult));
				}

				logger::info("Valhalla Poise: Loaded rule -> Key: [{}] | Var: [{}]", lookupKey, varName);
			}
		}
		logger::info("Valhalla Poise: Successfully mapped {} dynamic animation events.", settings::animEventDataMap.size());
	}
	else {
		logger::info("Valhalla Poise: WARNING - No [AnimEventMapping] section found in INI.");
	}

	// Parse SpecialAttackPreset INI Section
	CSimpleIniA::TNamesDepend presetKeys;
	if (animIni.GetAllKeys("SpecialAttackPreset", presetKeys)) {
		for (const auto& key : presetKeys) {
			if (!key.pItem) continue;

			std::string lowerName = inlineUtils::to_lower_string(key.pItem);
			const char* presetVal = animIni.GetValue("SpecialAttackPreset", key.pItem, "");

			if (!lowerName.empty() && presetVal && presetVal[0] != '\0') {
				settings::specialAttackPresets[lowerName] = presetVal;
				logger::info("Valhalla Combat: Registered Special Attack Preset [{}] -> [{}]", key.pItem, presetVal);
			}
		}
		logger::info("Valhalla Combat: Successfully loaded {} special attack presets.", settings::specialAttackPresets.size());
	}
	else {
		logger::info("Valhalla Combat: No [SpecialAttackPreset] section found in INI.");
	}

	// Exhaustion Suppressed Events Mapping
	CSimpleIniA::TNamesDepend eventKeys;
	animIni.GetAllKeys("ExhaustionSuppressedEvents", eventKeys);
	for (const auto& key : eventKeys) {
		std::string eventName = key.pItem;
		if (!eventName.empty()) {
			settings::suppressedExhaustionAnimEvents.insert(RE::BSFixedString(eventName.c_str()));
			logger::info("Valhalla Combat: Registered suppressed exhaustion anim event -> {}", eventName);
		}
	}
	logger::info("Valhalla Combat: Loaded {} during exhaustion suppressed animation events.", settings::suppressedExhaustionAnimEvents.size());

	// Advanced Settings
	settings::bAdvancedLogging = rawIni.GetBoolValue("AdvancedSettings", "bAdvancedLogging", settings::bAdvancedLogging);
	settings::bApplySpellOnSuccess = rawIni.GetBoolValue("AdvancedSettings", "bApplySpellOnSuccess", settings::bApplySpellOnSuccess);
	//settings::bExecutionGraphReset = rawIni.GetBoolValue("AdvancedSettings", "bExecutionGraphReset", settings::bExecutionGraphReset);
	
	// Graph Cost Multipliers
	settings::fGraph_LightAttackCostMult = static_cast<float>(rawIni.GetDoubleValue("AdvancedSettings", "fGraph_LightAttackCostMult", settings::fGraph_LightAttackCostMult));
	settings::fGraph_PowerAttackCostMult = static_cast<float>(rawIni.GetDoubleValue("AdvancedSettings", "fGraph_PowerAttackCostMult", settings::fGraph_PowerAttackCostMult));
	settings::fGraph_LightBashCostMult = static_cast<float>(rawIni.GetDoubleValue("AdvancedSettings", "fGraph_LightBashCostMult", settings::fGraph_LightBashCostMult));
	settings::fGraph_PowerBashCostMult = static_cast<float>(rawIni.GetDoubleValue("AdvancedSettings", "fGraph_PowerBashCostMult", settings::fGraph_PowerBashCostMult));
	
	logger::info("Valhalla Combat: Advanced Logging set to {}", settings::bAdvancedLogging);
	logger::info("Valhalla Combat: Apply Spell On Success set to {}", settings::bApplySpellOnSuccess);
	//logger::info("Valhalla Combat: Execution Graph Reset set to {}", settings::bExecutionGraphReset);
	logger::info("Valhalla Combat: Graph Light Attack Cost Mult set to {}", settings::fGraph_LightAttackCostMult);
	logger::info("Valhalla Combat: Graph Power Attack Cost Mult set to {}", settings::fGraph_PowerAttackCostMult);
	logger::info("Valhalla Combat: Graph Light Bash Cost Mult set to {}", settings::fGraph_LightBashCostMult);
	logger::info("Valhalla Combat: Graph Power Bash Cost Mult set to {}", settings::fGraph_PowerBashCostMult);
}

void data::loadINIPerks()
{
	logger::info("Valhalla Combat: Loading Perks from INI...");

	CSimpleIniA ini;
	ini.SetUnicode();

	if (ini.LoadFile(perksFilePath) < 0) {
		logger::warn("Valhalla Combat: Perks.ini not found or failed to load at path: {}", perksFilePath);
		return;
	}

	auto dataHandler = RE::TESDataHandler::GetSingleton();
	if (!dataHandler) return;

	// Helper lambda to fetch the section cleanly, parse the string, and populate the struct with strict verification
	auto parsePerkFeature = [&](const char* a_section, const char* a_baseKey, FeatureSetting& a_outSetting) {
		std::string boolKey = std::string("b") + a_baseKey;
		std::string boolNPCKey = boolKey + "_NPC";

		// 1. Load Booleans
		a_outSetting.enabled = ini.GetBoolValue(a_section, boolKey.c_str(), false);
		a_outSetting.enabledNPC = ini.GetBoolValue(a_section, boolNPCKey.c_str(), false);

		// 2. Load Perk String (e.g., "Skyrim.esm|58F13" or "Skyrim.esm|0x00058F13")
		const char* perkStr = ini.GetValue(a_section, a_baseKey, "");
		if (perkStr && strlen(perkStr) > 0) {
			std::vector<std::string> tokens = inlineUtils::tokenize("|", perkStr);
			if (tokens.size() == 2) {
				std::string plugin = tokens[0];

				// Check if plugin is loaded
				if (!dataHandler->LookupLoadedModByName(plugin)) {
					logger::warn("Valhalla Combat: Plugin [{}] is NOT loaded! Cannot load perk [{}]", plugin, a_baseKey);
					return;
				}

				try {
					RE::FormID rawFormID = static_cast<RE::FormID>(std::stoul(tokens[1], nullptr, 16));
					a_outSetting.perk = dataHandler->LookupForm<RE::BGSPerk>(rawFormID & 0x00FFFFFF, plugin);

					if (a_outSetting.perk) {
						logger::info("Valhalla Combat: Successfully loaded perk [{}] from {} (FormID: {:X})", a_baseKey, plugin, a_outSetting.perk->GetFormID());
					}
					else {
						logger::warn("Valhalla Combat: FormID [0x{:X}] in [{}] does not exist or is not a Perk record for [{}]", rawFormID & 0x00FFFFFF, plugin, a_baseKey);
					}
				}
				catch (...) {
					logger::error("Valhalla Combat: Invalid hex FormID [{}] for perk {}", tokens[1], a_baseKey);
				}
			}
		}
		};

	// Execute lambda for each section
	parsePerkFeature("Timed Blocking", "TimedBlockingPerk", perks::timedBlocking);
	parsePerkFeature("Projectile Blocking", "ProjectileBlockingPerk", perks::projectileBlocking);
	parsePerkFeature("Timed Projectile Blocking", "TimedProjectileBlockingPerk", perks::timedProjectileBlocking);
	parsePerkFeature("Projectile Parrying", "ProjectileParryingPerk", perks::projectileParrying);
	parsePerkFeature("Execution", "ExecutionPerk", perks::execution);
	parsePerkFeature("Tackle", "TacklePerk", perks::tackle);
	parsePerkFeature("Perfect Blocking", "PerfectBlockingPerk", perks::perfectBlocking);
	parsePerkFeature("Perfect Blocking Stamina", "PerfectBlockingStaminaPerk", perks::perfectBlockingStamina);
	parsePerkFeature("Guard Bash", "GuardBashPerk", perks::guardBash);
	parsePerkFeature("Weapon Block Spell", "WeaponBlockSpellPerk", perks::weaponBlockSpell);
	parsePerkFeature("Weapon Block Arrow", "WeaponBlockArrowPerk", perks::weaponBlockArrow);
	parsePerkFeature("Shield Block Spell", "ShieldBlockSpellPerk", perks::shieldBlockSpell);
	parsePerkFeature("Shield Block Arrow", "ShieldBlockArrowPerk", perks::shieldBlockArrow);
	parsePerkFeature("Weapon Parry Spell", "WeaponParrySpellPerk", perks::weaponParrySpell);
	parsePerkFeature("Weapon Parry Arrow", "WeaponParryArrowPerk", perks::weaponParryArrow);
	parsePerkFeature("Shield Parry Spell", "ShieldParrySpellPerk", perks::shieldParrySpell);
	parsePerkFeature("Shield Parry Arrow", "ShieldParryArrowPerk", perks::shieldParryArrow);

	logger::info("Valhalla Combat: Finished loading Perks.ini");
}
