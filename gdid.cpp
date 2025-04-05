#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <iostream>
#include <winioctl.h>
#include <string>

// https://bbs.kanxue.com/thread-252767.htm

typedef struct _HARDDISKINFO2
{
    ULONG of_name1;        // 名称1偏移
    ULONG unknown1[3];     // 未知数据
    ULONG of_name2;        // 名称2偏移
    ULONG of_FirmwareRev;  // 固件版本偏移
    ULONG of_SerialNumber; // 序列号偏移
    ULONG unknown2;        // 未知数据
    char outdata[520];     // 硬盘数据
    PCHAR get_FirmwareRev()
    {
        if (of_FirmwareRev < sizeof(_HARDDISKINFO2))
        {
            return (PCHAR)this + of_FirmwareRev;
        }
        return const_cast<PCHAR>("");
    }
    PCHAR get_name1()
    {
        if (((PCHAR)this)[of_name1] == 0)
            return (PCHAR)this + of_name1 + 8;
        else
            return (PCHAR)this + of_name1;
        return NULL;
    }
    PCHAR get_name2()
    {
        if (of_name2 < sizeof(_HARDDISKINFO2))
        {
            return (PCHAR)this + of_name2;
        }
        return const_cast<PCHAR>("");
    }
    PCHAR get_SerialNumber()
    {
        if (of_SerialNumber < sizeof(_HARDDISKINFO2))
        {
            return (PCHAR)this + of_SerialNumber;
        }
        return const_cast<PCHAR>("");
    }
    PCHAR get_outdata()
    {
        return outdata;
    }

} HARDDISKINFO2, *PHARDDISKINFO2;

std::string get_HardDiskUID(int harddiskindex)
{
    char objName[50] = {0};
    wsprintfA(objName, "\\\\.\\PhysicalDrive%d", harddiskindex);
    HANDLE hDevice = ::CreateFileA(objName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (hDevice == INVALID_HANDLE_VALUE)
        return "error -1";

    unsigned char inputdata[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0x00, 0x00};

    HARDDISKINFO2 hdinfo = {0};
    DWORD dwBytesReturned = 0;
    if (!DeviceIoControl(hDevice, 0x2D1400, inputdata, sizeof(inputdata), &hdinfo, sizeof(HARDDISKINFO2), &dwBytesReturned, nullptr))
    {
        ::CloseHandle(hDevice);
        return "error -2";
    }
    ::CloseHandle(hDevice);

    return std::string(hdinfo.get_FirmwareRev()) + "|" + hdinfo.get_name1() + "|" + std::string(hdinfo.get_name2()) + "|" + std::string(hdinfo.get_SerialNumber());
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        std::cout << "Usage: " << argv[0] << " <harddiskindex>" << std::endl;
        return 1;
    }
    int harddiskindex = std::stoi(argv[1]);
    std::cout << get_HardDiskUID(harddiskindex) << std::endl;

    return 0;
}