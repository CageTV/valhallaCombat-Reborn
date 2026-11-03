#include "include/ModAPI.h"
#include "include/stunHandler.h"
#include "include/blockHandler.h"
#include "include/parryHandler.h"
#include "include/debuffHandler.h"
// V1
void ModAPI::VALInterface::processStunDamage(VAL_API::STUNSOURCE stunSource, RE::TESObjectWEAP* weapon, RE::Actor* aggressor, RE::Actor* victim, float baseDamage) noexcept
{
	stunHandler::GetSingleton()->processStunDamage(static_cast<stunHandler::STUNSOURCE>(stunSource), weapon, aggressor, victim, baseDamage);
}

// V2
bool ModAPI::VALInterface::getIsTimedBlocking(RE::Actor* a_actor) noexcept
{
	if (!a_actor) {
		return false;
	}
	return blockHandler::GetSingleton()->getIsTimedBlocking(a_actor);
}

bool ModAPI::VALInterface::getIsPerfectBlocking(RE::Actor* a_actor) noexcept
{
	if (!a_actor) {
		return false;
	}
	return blockHandler::GetSingleton()->getIsPerfectBlocking(a_actor);
}

void ModAPI::VALInterface::triggerTimedBlockSuccess(RE::Actor* a_actor) noexcept
{
	if (!a_actor) {
		return;
	}
	blockHandler::GetSingleton()->onSuccessfulTimedBlock(a_actor);
}

bool ModAPI::VALInterface::isActorStunned(RE::Actor* a_actor) noexcept
{
	if (!a_actor) {
		return false;
	}
	return stunHandler::GetSingleton()->getIsStunBroken(a_actor);
}

bool ModAPI::VALInterface::isActorExhausted(RE::Actor* a_actor) noexcept
{
	if (!a_actor) {
		return false;
	}
	return debuffHandler::GetSingleton()->isInDebuff(a_actor);
}

// V3
bool ModAPI::VALInterface::isEligibleForExecution(RE::Actor* a_actor) noexcept
{
	if (!a_actor) {
		return false;
	}
	return stunHandler::GetSingleton()->isEligibleForExecution(a_actor);
}

bool ModAPI::VALInterface::getIsTackling(RE::Actor* a_actor) noexcept
{
	if (!a_actor) {
		return false;
	}
	return blockHandler::GetSingleton()->getIsTackling(a_actor);
}

bool ModAPI::VALInterface::inParryState(RE::Actor* a_actor) noexcept
{
	if (!a_actor) {
		return false;
	}
	return parryHandler::GetSingleton()->inParryState(a_actor);
}

void ModAPI::VALInterface::applyMovementRestrictions(RE::Actor* a_actor) noexcept
{
	if (!a_actor) {
		return;
	}
	debuffHandler::GetSingleton()->applyMovementRestrictions(a_actor);
}

void ModAPI::VALInterface::clearMovementRestrictions(RE::Actor* a_actor) noexcept
{
	if (!a_actor) {
		return;
	}
	debuffHandler::GetSingleton()->clearMovementRestrictions(a_actor);
}

void ModAPI::VALInterface::initStaminaDebuff(RE::Actor* a_actor) noexcept
{
	if (!a_actor) {
		return;
	}
	debuffHandler::GetSingleton()->initStaminaDebuff(a_actor);
}

void ModAPI::VALInterface::stopDebuff(RE::Actor* a_actor) noexcept
{
	if (!a_actor) {
		return;
	}
	debuffHandler::GetSingleton()->stopDebuff(a_actor);
}
