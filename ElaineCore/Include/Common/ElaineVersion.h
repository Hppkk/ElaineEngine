#pragma once
#include "ElaineCorePrerequirements.h"

namespace Elaine
{
	class ElaineCoreExport Version
	{
	public:
		Version();
		Version(const std::string& InString);
		Version(int InMajor, int InMinor, int InPatch);
		~Version();
		bool operator==(const Version& InOther);
		bool operator==(const std::string& InOther);
		Version& operator=(const std::string& InOther);
		bool operator<(const Version& InVersion);
		bool operator>(const Version& InVersion);
		bool operator<=(const Version& InVersion);
		bool operator>=(const Version& InVersion);

		std::string ToString() const;
	private:
		int mMajor = 0;
		int mMinor = 0;
		int mPatch = 0;
	};
}