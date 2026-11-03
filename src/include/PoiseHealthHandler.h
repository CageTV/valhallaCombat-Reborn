#pragma once


class PoiseHealthHandler
{
public:
	static float GetTotalPoiseHealth(RE::Actor* a_target);

private:
	static float GetTotalArmorPoiseHealth(RE::Actor* a_target);

	// Deleted constructors to treat this strictly as a static utility class
	PoiseHealthHandler() = delete;
	~PoiseHealthHandler() = delete;
};