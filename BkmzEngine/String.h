#pragma once
#include <string>

namespace bkmz::utl
{
	std::wstring ToWide(const std::string &narrow);
	std::string ToNarrow(const std::wstring &wide);
}