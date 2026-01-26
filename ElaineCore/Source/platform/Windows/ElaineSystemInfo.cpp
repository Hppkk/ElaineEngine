#include "ElainePrecompiledHeader.h"
#include "platform/ElaineSystemInfo.h"
#include <intrin.h>
#include <iomanip>

namespace Elaine
{
    void SystemInfo::LogSystemInfo()
    {
        // CPU Info
        int cpuInfo[4] = { -1 };
        char cpuBrandString[0x40];
        __cpuid(cpuInfo, 0x80000000);
        unsigned int nExIds = cpuInfo[0];

        memset(cpuBrandString, 0, sizeof(cpuBrandString));

        for (unsigned int i = 0x80000000; i <= nExIds; ++i)
        {
            __cpuid(cpuInfo, i);
            if (i == 0x80000002)
                memcpy(cpuBrandString, cpuInfo, sizeof(cpuInfo));
            else if (i == 0x80000003)
                memcpy(cpuBrandString + 16, cpuInfo, sizeof(cpuInfo));
            else if (i == 0x80000004)
                memcpy(cpuBrandString + 32, cpuInfo, sizeof(cpuInfo));
        }

        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);

        LOG_INFO("Hardware Info:");
        LOG_INFO("  CPU: {}", cpuBrandString);
        LOG_INFO("  Logical Processors: {}", sysInfo.dwNumberOfProcessors);

        // Memory Info
        MEMORYSTATUSEX memInfo;
        memInfo.dwLength = sizeof(MEMORYSTATUSEX);
        GlobalMemoryStatusEx(&memInfo);

        double totalPhysMemGB = static_cast<double>(memInfo.ullTotalPhys) / (1024 * 1024 * 1024);
        double availPhysMemGB = static_cast<double>(memInfo.ullAvailPhys) / (1024 * 1024 * 1024);

        LOG_INFO("  Total System Memory: {:.2f} GB", totalPhysMemGB);
        LOG_INFO("  Available System Memory: {:.2f} GB", availPhysMemGB);
    }
}
