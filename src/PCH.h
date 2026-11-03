#pragma once

#pragma warning(push)
#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>

#pragma warning(disable: 4702)
#include <SimpleIni.h>

#include <spdlog/sinks/basic_file_sink.h>

#pragma warning(pop)

using namespace std::literals;

namespace logger = SKSE::log;

namespace util
{
	using SKSE::stl::report_and_fail;
}

namespace std
{
	template <class T>
	struct hash<RE::BSPointerHandle<T>>
	{
		uint32_t operator()(const RE::BSPointerHandle<T>& a_handle) const
		{
			uint32_t nativeHandle = const_cast<RE::BSPointerHandle<T>*>(&a_handle)->native_handle();  // ugh
			return nativeHandle;
		}
	};
}

#define DLLEXPORT __declspec(dllexport)

#define RELOCATION_OFFSET(SE, AE) REL::VariantOffset(SE, AE, 0).offset()

#include "Plugin.h"

/*template <>
struct fmt::formatter<RE::BSFixedString> : fmt::formatter<std::string_view> {
  auto format(const RE::BSFixedString &str, fmt::format_context &ctx) const {
    return fmt::formatter<std::string_view>::format(str.c_str(), ctx);
  }
};*/

template <>
struct std::hash<RE::BSFixedString> {
	std::size_t operator()(const RE::BSFixedString& a_str) const noexcept {
		return std::hash<std::string_view>{}(a_str.c_str());
	}
};
