#include "ElainePrecompiledHeader.h"
#include "ElaineVersion.h"
#include <charconv>

namespace Elaine
{
	static bool IsVersionFormat(const std::string& InString)
	{
		int dotCount = 0;
		if (InString.empty())
			return false;

		for (auto c : InString)
		{
			if (c == '.')
			{
				++dotCount;
			}
			else if (c < '0' || c > '9')
			{
				return false;
			}
		}

		return dotCount == 2;
	}

	static bool IsInt(const std::string& InString)
	{
		int value;
		auto [ptr, er] = std::from_chars(InString.data(), InString.data() + InString.size(), value);
		return er == std::errc() && ptr == InString.data() + InString.size();
	}

	static bool ParseVersion(const std::string& InString, int& major, int& minor, int& patch)
	{
		size_t p1 = InString.find('.');
		size_t p2 = InString.find('.', p1 + 1);
		if (p1 == std::string::npos || p2 == std::string::npos)
			return false;

		return
			std::from_chars(InString.data(), InString.data() + p1, major).ec == std::errc() &&
			std::from_chars(InString.data() + p1 + 1, InString.data() + p2, minor).ec == std::errc() &&
			std::from_chars(InString.data() + p2 + 1, InString.data() + InString.size(), patch).ec == std::errc();
	}

	Version::Version()
	{

	}

	Version::Version(const std::string& InString)
	{
		if (!ParseVersion(InString, mMajor, mMinor, mPatch))
		{
			LOG_ERROR("Version parse failed.");
		}
	}

	Version::Version(int InMajor, int InMinor, int InPatch)
	{
		mMajor = InMajor;
		mMinor = InMinor;
		mPatch = InPatch;
	}

	Version::~Version()
	{

	}

	bool Version::operator==(const Version& InOther)
	{
		return mMajor == InOther.mMajor && mMinor == InOther.mMinor && mPatch == InOther.mPatch;
	}

	bool Version::operator==(const std::string& InOther)
	{
		int Major = 0, Minor = 0, Patch = 0;
		if (!ParseVersion(InOther, Major, Minor, Patch))
		{
			return false;
		}

		return mMajor == Major && mMinor == Minor && mPatch == Patch;
	}

	Version& Version::operator=(const std::string& InOther)
	{
		int Major = 0, Minor = 0, Patch = 0;
		if (!ParseVersion(InOther, mMajor, mMinor, mPatch))
		{
			LOG_ERROR("Version parse failed.");
		}
		return *this;
	}
	bool Version::operator<(const Version& InVersion)
	{
		if (mMajor < InVersion.mMajor)
			return true;

		if (mMajor == InVersion.mMajor && mMinor < InVersion.mMinor)
			return true;

		if (mMajor == InVersion.mMajor && mMinor == InVersion.mMinor && mPatch < InVersion.mPatch)
			return true;

		return false;
	}
	bool Version::operator>(const Version& InVersion)
	{
		if (mMajor > InVersion.mMajor)
			return true;

		if (mMajor == InVersion.mMajor && mMinor > InVersion.mMinor)
			return true;

		if (mMajor == InVersion.mMajor && mMinor == InVersion.mMinor && mPatch > InVersion.mPatch)
			return true;

		return false;
	}
	bool Version::operator<=(const Version& InVersion)
	{
		if (mMajor <= InVersion.mMajor)
			return true;

		if (mMajor == InVersion.mMajor && mMinor <= InVersion.mMinor)
			return true;

		if (mMajor == InVersion.mMajor && mMinor == InVersion.mMinor && mPatch <= InVersion.mPatch)
			return true;

		return false;
	}
	bool Version::operator>=(const Version& InVersion)
	{
		if (mMajor >= InVersion.mMajor)
			return true;

		if (mMajor == InVersion.mMajor && mMinor >= InVersion.mMinor)
			return true;

		if (mMajor == InVersion.mMajor && mMinor == InVersion.mMinor && mPatch >= InVersion.mPatch)
			return true;

		return false;
	}
	std::string Version::ToString() const
	{
		return std::format("{}.{}.{}", mMajor, mMinor, mPatch);
	}
}