#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <iostream>
#include <winioctl.h>
#include <string>
#include <sstream>

#pragma pack(push, 1) // 确保结构体紧凑对齐
typedef struct _HARDDISKINFO2
{
    ULONG of_name1;
    ULONG unknown1[3];
    ULONG of_name2;
    ULONG of_FirmwareRev;
    ULONG of_SerialNumber;
    ULONG unknown2;
    char outdata[520];

    const char *get_field(ULONG offset, size_t max_len = 50) const
    {
        if (offset >= sizeof(_HARDDISKINFO2))
            return "";
        const char *field = reinterpret_cast<const char *>(this) + offset;
        // 防止非终止字符串，手动截断
        for (size_t i = 0; i < max_len; ++i)
        {
            if (field[i] == '\0')
                return field;
        }
        static char buffer[51];
        strncpy_s(buffer, field, 50);
        buffer[50] = '\0';
        return buffer;
    }
} HARDDISKINFO2, *PHARDDISKINFO2;
#pragma pack(pop) // 恢复默认对齐

std::string GetHardDiskInfo(int disk_index)
{
    char device_path[50];
    snprintf(device_path, sizeof(device_path), "\\\\.\\PhysicalDrive%d", disk_index);

    HANDLE h_device = CreateFileA(device_path, GENERIC_READ | GENERIC_WRITE,
                                  FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (h_device == INVALID_HANDLE_VALUE)
    {
        DWORD err = GetLastError();
        std::cerr << "Failed to open device (Error " << err << ")" << std::endl;
        return "Error: Cannot access device";
    }

    BYTE input_data[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0x00, 0x00};
    HARDDISKINFO2 hd_info = {0};
    DWORD bytes_returned = 0;

    if (!DeviceIoControl(h_device, 0x2D1400, input_data, sizeof(input_data),
                         &hd_info, sizeof(HARDDISKINFO2), &bytes_returned, nullptr))
    {
        DWORD err = GetLastError();
        CloseHandle(h_device);
        std::cerr << "DeviceIoControl failed (Error " << err << ")" << std::endl;
        return "Error: Communication failed";
    }
    CloseHandle(h_device);

    // 安全获取字段
    std::stringstream ss;
    ss << ">>"
       << hd_info.get_field(hd_info.of_name2) << "|"
       << hd_info.get_field(hd_info.of_FirmwareRev) << "|"
       << hd_info.get_field(hd_info.of_SerialNumber) << "|"
       << hd_info.get_field(hd_info.of_name1) << "|"
       << hd_info.unknown1[0] << "|"
       << hd_info.unknown1[1] << "|"
       << hd_info.unknown1[2] << "|"
       << hd_info.unknown1[3] << "|"
       << hd_info.unknown2 << "<<";
    return ss.str();
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " <DiskIndex>" << std::endl;
        return 1;
    }

    try
    {
        int index = std::stoi(argv[1]);
        std::cout << GetHardDiskInfo(index) << std::endl;
    }
    catch (...)
    {
        std::cerr << "Invalid disk index." << std::endl;
        return 1;
    }
    return 0;
}
