#pragma once
#include "include/debuffHandler.h"
#include "include/blockHandler.h"
#include "include/parryHandler.h"
#include "include/stunHandler.h"
#include "include/poiseHandler.h"
#include "include/AI.h"
#include "include/AILearning.h"
#include "include/lib/TrueHUDAPI.h"
#include "include/lib/PrecisionAPI.h"
#include "include/settings.h"
#include "include/specialAttackHandler.h"
/*Combat tweaks to make Skyrim's melee combat feel like AC:Valhalla.*/
class ValhallaCombat
{
public:

	TRUEHUD_API::IVTrueHUD3* ersh_TrueHUD = nullptr;
	PRECISION_API::IVPrecision1* ersh_Precision = nullptr;
	
	static ValhallaCombat* GetSingleton()
	{
		static ValhallaCombat singleton;
		return  std::addressof(singleton);
	}


	enum HANDLER {
		debuffHandler = 0,
		blockHandler,
		stunHandler,
		parryHandler,
		blockCommitment,
		poiseHandler,
		aiHandler,
		aiLearningHandler,
		specialAttackHandler
	};

	/*Start updating a handler per tick.
	@param handlerToActivate: the handler which will start updating per tick.*/
	void activateUpdate(HANDLER a_ValhallaHandler) {
		switch (a_ValhallaHandler)
		{
		case ValhallaCombat::debuffHandler: update_DebuffHandler = true;
			break;
		case ValhallaCombat::blockHandler: update_BlockHandler = true;
			break;
		case ValhallaCombat::stunHandler: update_StunHandler = true;
			break;
		case ValhallaCombat::parryHandler: update_ParryHandler = true;
			break;
		case ValhallaCombat::blockCommitment: update_BlockCommitment = true;
			break;
		case ValhallaCombat::poiseHandler: update_PoiseHandler = true;
			break;
		case ValhallaCombat::aiHandler: update_AIHandler = true;
			break;
		case ValhallaCombat::aiLearningHandler: update_AILearningHandler = true;
			break;
		case ValhallaCombat::specialAttackHandler: update_SpecialAttackHandler = true;
			break;
		}
	}
	/*Stop updating a handler per tick.
	@param handlerToDeactivate: the handler which will stop updating per tick.*/
	void deactivateUpdate(HANDLER a_ValallaHandler) {
		switch (a_ValallaHandler) {
		case ValhallaCombat::debuffHandler: update_DebuffHandler = false;
			break;
		case ValhallaCombat::blockHandler: update_BlockHandler = false;
			break;
		case ValhallaCombat::stunHandler: update_StunHandler = false;
			break;
		case ValhallaCombat::parryHandler: update_ParryHandler = false;
			break;
		case ValhallaCombat::blockCommitment: update_BlockCommitment = false;
			break;
		case ValhallaCombat::poiseHandler: update_PoiseHandler = false;
			break;
		case ValhallaCombat::aiHandler: update_AIHandler = false;
			break;
		case ValhallaCombat::aiLearningHandler: update_AILearningHandler = false;
			break;
		case ValhallaCombat::specialAttackHandler: update_SpecialAttackHandler = false;
			break;
		}
	}

	/*Runs every frame when game is not paused.*/
	void update() {
		if (RE::UI::GetSingleton()->GameIsPaused()) {
			return;
		}
		if (update_DebuffHandler) {
			debuffHandler::GetSingleton()->update();
		}
		if (update_BlockHandler) {
			blockHandler::GetSingleton()->update();
		}
		if (update_StunHandler) {
			stunHandler::GetSingleton()->update();
		}
		if (update_ParryHandler) {
			parryHandler::GetSingleton()->update();
		}
		if (update_BlockCommitment) {
			BlockCommitment::GetSingleton()->update();
		}
		if (update_PoiseHandler) {
			poiseHandler::GetSingleton()->update();
		}
		if (update_AIHandler) {
			AI::GetSingleton()->update();
		}
		if (update_AILearningHandler) {
			AILearning::GetSingleton()->update();
		}
		if (update_SpecialAttackHandler) {
			SpecialAttackHandler::GetSingleton()->update();
		}
	}


	/*Request special bar control from truehud API. 
	If successful, set the truehud specialmeter global value to true.*/

	void requestTrueHudSpecialBarControl() {
		logger::info("Requesting trueHUD API special bar control...");
		if (!ersh_TrueHUD) {
			logger::info("...aborted: TrueHUD API not acquired.");
			return;
		}

		auto res = ersh_TrueHUD->RequestSpecialResourceBarsControl(SKSE::GetPluginHandle());
		switch (res) {
		case TRUEHUD_API::APIResult::OK:
		case TRUEHUD_API::APIResult::AlreadyGiven:

			// Register callbacks based on who owns the special bar
			if (settings::facts::TrueHudAPI_PoiseControlsSpecialBar) {
				ersh_TrueHUD->RegisterSpecialResourceFunctions(
					SKSE::GetPluginHandle(),
					poiseHandler::getCurrentPoise_static,
					poiseHandler::getTotalPoise_static,
					true, true
				);
			}
			else {
				ersh_TrueHUD->RegisterSpecialResourceFunctions(
					SKSE::GetPluginHandle(),
					stunHandler::getCurrentStun_static,
					stunHandler::getMaxStun_static,
					true, true
				);
			}

			settings::facts::TrueHudAPI_HasSpecialBarControl = true;
			logger::info("...Success");
			settings::updateGlobals();
			break;
		case TRUEHUD_API::APIResult::AlreadyTaken:
			logger::info("...Failure: TrueHUD API already taken by another plugin");
			break;
		}
	}
	void releaseTrueHudSpecialBarControl() {
		logger::info("Release trueHUD API special bar control...");
		if (!ersh_TrueHUD) {
			logger::info("...aborted: TrueHUD API not acquired.");
			return;
		}
		auto res = ersh_TrueHUD->ReleaseSpecialResourceBarControl(SKSE::GetPluginHandle());
		if (res == TRUEHUD_API::APIResult::OK) {
			settings::facts::TrueHudAPI_HasSpecialBarControl = false;
			settings::updateGlobals();
			logger::info("...Success");
		}
		else {
			logger::info("...Failure");
		}
	}

private:
	bool update_DebuffHandler = false;
	bool update_BlockHandler = false;
	bool update_StunHandler = false;
	bool update_ParryHandler = false;
	bool update_BlockCommitment = false;
	bool update_PoiseHandler = false;
	bool update_AIHandler = false;
	bool update_AILearningHandler = false;
	bool update_SpecialAttackHandler = false;
};
