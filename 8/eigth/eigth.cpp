#define _CRT_SECURE_NO_WARNINGS
#include "windows.h"
#include "winbase.h"
#include "tchar.h" //_tcslen()
#include "stdio.h" //printf
#include "conio.h"

#ifdef _WIN32
#include <limits.h>
#include <intrin.h>
typedef unsigned __int32  uint32_t;
#else
#include <stdint.h>
#endif

#include <algorithm>
#include <iostream>
#include <string>

using namespace std;
 
 
#define DRIVE_UNKNOWN     0   // Неправильное имя
#define DRIVE_NO_ROOT_DIR 1   // Неправильное имя: отсутствует диск
#define DRIVE_REMOVABLE   2   // Съёмное устройство (магнито-оптика, zip)
#define DRIVE_FIXED       3   // жёсткий диск (обычный винчестер)
#define DRIVE_REMOTE      4   // сетевой диск
#define DRIVE_CDROM       5   // CD-ROM
#define DRIVE_RAMDISK     6   // RAM диск
#define DIV 1024
#define WIDTH 7
#define MAX_INTEL_TOP_LVL 4

char* divisor = (char *)"K";
 
void GetVolumeInfo(char* Volume)
{
    char Name[MAX_PATH];
    char FileSysName[256];
    DWORD SerialNumber;
    DWORD MaxLength;
    DWORD FileSysFlags;
    DWORD SectorsPerCluster;
    DWORD BytesPerSector;
    DWORD NumberOfFreeClusters;
    DWORD TotalNumberOfClusters;

    char str[MAX_PATH];
    if (GetVolumeInformation(
        (LPCWSTR)Volume,
        (LPWSTR)Name,
        256,
        &SerialNumber,
        &MaxLength,
        &FileSysFlags,
        (LPWSTR)FileSysName,
        256))
    {
        GetDiskFreeSpace(
            (LPCWSTR)Volume,
            &SectorsPerCluster,
            &BytesPerSector,
            &NumberOfFreeClusters,
            &TotalNumberOfClusters);

        sprintf(str,
            "\t## VOLUME %s INFO\n Name : %s\n Serial number : %p\n File system : %s\n",
            Volume, Name, SerialNumber, FileSysName);
        sprintf(str,
            "%s SectorsPerCluster :\t%d\n BytesPerSector :\t%d\n NumberOfFreeClusters :\t%d\n TotalNumberOfClusters : \t%d\n",
            str, SectorsPerCluster, BytesPerSector, NumberOfFreeClusters, TotalNumberOfClusters);
    }
    else
        sprintf(str, "\t## WRONG VOLUME NAME OR NOT READY!");
    printf(str);
}
 
struct {
   UINT type;        // возвращаемый код из GetDriveType
   LPCSTR name;      // ascii имя
} DriveTypeFlags [] = {
   { DRIVE_UNKNOWN,     "Unknown" },
   { DRIVE_NO_ROOT_DIR, "Invalid path" },
   { DRIVE_REMOVABLE,   "Removable" },
   { DRIVE_FIXED,       "Fixed" },
   { DRIVE_REMOTE,      "Network drive" },
   { DRIVE_CDROM,       "CD-ROM" },
   { DRIVE_RAMDISK,     "RAM disk" },
   { 0, NULL},
};

class CPUID {
    uint32_t regs[4];

public:
    explicit CPUID(unsigned funcId, unsigned subFuncId) {
#ifdef _WIN32
        __cpuidex((int*)regs, (int)funcId, (int)subFuncId);

#else
        asm volatile
            ("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
                : "a" (funcId), "c" (subFuncId));
        // ECX is set to zero for CPUID function 4
#endif
    }

    const uint32_t& EAX() const { return regs[0]; }
    const uint32_t& EBX() const { return regs[1]; }
    const uint32_t& ECX() const { return regs[2]; }
    const uint32_t& EDX() const { return regs[3]; }
};


class CPUInfo {
public:
    CPUInfo();
    string  vendor()            const { return mVendorId; }
    string  model()             const { return mModelName; }
    int     cores()             const { return mNumCores; }
    float   cpuSpeedInMHz()     const { return mCPUMHz; }
    bool    isSSE()             const { return mIsSSE; }
    bool    isSSE2()            const { return mIsSSE2; }
    bool    isSSE3()            const { return mIsSSE3; }
    bool    isSSE41()           const { return mIsSSE41; }
    bool    isSSE42()           const { return mIsSSE42; }
    bool    isAVX()             const { return mIsAVX; }
    bool    isAVX2()            const { return mIsAVX2; }
    bool    isHyperThreaded()   const { return mIsHTT; }
    int     logicalCpus()       const { return mNumLogCpus; }

private:
    static const uint32_t SSE_POS = 0x02000000;
    static const uint32_t SSE2_POS = 0x04000000;
    static const uint32_t SSE3_POS = 0x00000001;
    static const uint32_t SSE41_POS = 0x00080000;
    static const uint32_t SSE42_POS = 0x00100000;
    static const uint32_t AVX_POS = 0x10000000;
    static const uint32_t AVX2_POS = 0x00000020;
    static const uint32_t LVL_NUM = 0x000000FF;
    static const uint32_t LVL_TYPE = 0x0000FF00;
    static const uint32_t LVL_CORES = 0x0000FFFF;

    string mVendorId;
    string mModelName;
    int    mNumSMT;
    int    mNumCores;
    int    mNumLogCpus;
    float  mCPUMHz;
    bool   mIsHTT;
    bool   mIsSSE;
    bool   mIsSSE2;
    bool   mIsSSE3;
    bool   mIsSSE41;
    bool   mIsSSE42;
    bool   mIsAVX;
    bool   mIsAVX2;
};


CPUInfo::CPUInfo()
{
    // Get vendor name EAX=0
    CPUID cpuID0(0, 0);
    uint32_t HFS = cpuID0.EAX();
    mVendorId += string((const char*)&cpuID0.EBX(), 4);
    mVendorId += string((const char*)&cpuID0.EDX(), 4);
    mVendorId += string((const char*)&cpuID0.ECX(), 4);
    // Get SSE instructions availability
    CPUID cpuID1(1, 0);
    mIsHTT = cpuID1.EDX() & AVX_POS;
    mIsSSE = cpuID1.EDX() & SSE_POS;
    mIsSSE2 = cpuID1.EDX() & SSE2_POS;
    mIsSSE3 = cpuID1.ECX() & SSE3_POS;
    mIsSSE41 = cpuID1.ECX() & SSE41_POS;
    mIsSSE42 = cpuID1.ECX() & SSE41_POS;
    mIsAVX = cpuID1.ECX() & AVX_POS;
    // Get AVX2 instructions availability
    CPUID cpuID7(7, 0);
    mIsAVX2 = cpuID7.EBX() & AVX2_POS;

    string upVId = mVendorId;
    for_each(upVId.begin(), upVId.end(), [](char& in) { in = ::toupper(in); });
    // Get num of cores
    if (upVId.find("INTEL") != std::string::npos) {
        if (HFS >= 11) {
            for (int lvl = 0; lvl < MAX_INTEL_TOP_LVL; ++lvl) {
                CPUID cpuID4(0x0B, lvl);
                uint32_t currLevel = (LVL_TYPE & cpuID4.ECX()) >> 8;
                switch (currLevel) {
                case 0x01: mNumSMT = LVL_CORES & cpuID4.EBX(); break;
                case 0x02: mNumLogCpus = LVL_CORES & cpuID4.EBX(); break;
                default: break;
                }
            }
            mNumCores = mNumLogCpus / mNumSMT;
        }
        else {
            if (HFS >= 1) {
                mNumLogCpus = (cpuID1.EBX() >> 16) & 0xFF;
                if (HFS >= 4) {
                    mNumCores = 1 + (CPUID(4, 0).EAX() >> 26) & 0x3F;
                }
            }
            if (mIsHTT) {
                if (!(mNumCores > 1)) {
                    mNumCores = 1;
                    mNumLogCpus = (mNumLogCpus >= 2 ? mNumLogCpus : 2);
                }
            }
            else {
                mNumCores = mNumLogCpus = 1;
            }
        }
    }
    else if (upVId.find("AMD") != std::string::npos) {
        if (HFS >= 1) {
            mNumLogCpus = (cpuID1.EBX() >> 16) & 0xFF;
            if (CPUID(0x80000000, 0).EAX() >= 8) {
                mNumCores = 1 + (CPUID(0x80000008, 0).ECX() & 0xFF);
            }
        }
        if (mIsHTT) {
            if (!(mNumCores > 1)) {
                mNumCores = 1;
                mNumLogCpus = (mNumLogCpus >= 2 ? mNumLogCpus : 2);
            }
        }
        else {
            mNumCores = mNumLogCpus = 1;
        }
    }
    else {
        cout << "Unexpected vendor id" << endl;
    }
    // Get processor brand string
    // This seems to be working for both Intel & AMD vendors
    for (int i = 0x80000002; i < 0x80000005; ++i) {
        CPUID cpuID(i, 0);
        mModelName += string((const char*)&cpuID.EAX(), 4);
        mModelName += string((const char*)&cpuID.EBX(), 4);
        mModelName += string((const char*)&cpuID.ECX(), 4);
        mModelName += string((const char*)&cpuID.EDX(), 4);
    }
}




 
void main()
{
    TCHAR buf[100];
    DWORD len = GetLogicalDriveStrings(sizeof(buf)/sizeof(TCHAR),buf);
 
   
    LPCTSTR msg  = (LPCTSTR)"Logical Drives:\n";  // строка STL
    printf((const char*)msg);
    for (TCHAR* s=buf; *s; s+=_tcslen(s)+1) 
    {
        LPCTSTR sDrivePath = s;
        printf((const char*)sDrivePath);
        printf(" ");
        
        UINT uDriveType = GetDriveType(sDrivePath);
 
        for (int i=0; DriveTypeFlags[i].name; i++) 
        {
            if (uDriveType == DriveTypeFlags[i].type) 
            {
                printf(DriveTypeFlags[i].name);
                strcpy((char*)buf, (const char*)sDrivePath);
                GetVolumeInfo((char*)buf);
                break;
             }
        }
        printf("\n");
    }


    MEMORYSTATUS stat;
    GlobalMemoryStatus(&stat);

    printf("The MemoryStatus structure is %ld bytes long.\n",
        stat.dwLength);
    printf("%ld percent of memory is in use.\n",
        stat.dwMemoryLoad);
    printf("There are %*d total %sbytes of physical memory.\n",
        WIDTH, stat.dwTotalPhys / DIV, divisor);
    printf("There are %*d free %sbytes of physical memory.\n",
        WIDTH, stat.dwAvailPhys / DIV, divisor);
    printf("There are %*d total %sbytes of paging file.\n",
        WIDTH, stat.dwTotalPageFile / DIV, divisor);
    printf("There are %*d free %sbytes of paging file.\n",
        WIDTH, stat.dwAvailPageFile / DIV, divisor);
    printf("There are %*d total %sbytes of virtual memory.\n",
        WIDTH, stat.dwTotalVirtual / DIV, divisor);
    printf("There are %*d free %sbytes of virtual memory.\n",
        WIDTH, stat.dwAvailVirtual / DIV, divisor);
    
    CPUInfo cinfo;

    cout << "CPU vendor = " << cinfo.vendor() << endl;
    cout << "CPU Brand String = " << cinfo.model() << endl;
    cout << "# of cores = " << cinfo.cores() << endl;
    cout << "# of logical cores = " << cinfo.logicalCpus() << endl;
    cout << "Is CPU Hyper threaded = " << cinfo.isHyperThreaded() << endl;
    cout << "CPU SSE = " << cinfo.isSSE() << endl;
    cout << "CPU SSE2 = " << cinfo.isSSE2() << endl;
    cout << "CPU SSE3 = " << cinfo.isSSE3() << endl;
    cout << "CPU SSE41 = " << cinfo.isSSE41() << endl;
    cout << "CPU SSE42 = " << cinfo.isSSE42() << endl;
    cout << "CPU AVX = " << cinfo.isAVX() << endl;
    cout << "CPU AVX2 = " << cinfo.isAVX2() << endl;

}
 