#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <stdio.h>
#include <winioctl.h>
#include <string.h>

// 修改函数：使用调用者提供的缓冲区
void get_device_string(const STORAGE_DEVICE_DESCRIPTOR* desc, DWORD offset, char* buffer, size_t buffer_size) {
    if (offset == 0 || offset >= desc->Size || buffer_size < 1) {
        buffer[0] = '\0';
        return;
    }
    const char* str = (const char*)desc + offset;
    size_t i = 0;
    for (; i < buffer_size - 1 && str[i] != '\0'; ++i) {
        buffer[i] = str[i];
    }
    buffer[i] = '\0';
}

const char* GetHardDiskInfo(int disk_index, char* result, size_t result_size) {
    char device_path[50];
    snprintf(device_path, sizeof(device_path), "\\\\.\\PhysicalDrive%d", disk_index);

    HANDLE h_device = CreateFileA(device_path, GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (h_device == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        fprintf(stderr, "Failed to open device (Error %lu)\n", err);
        snprintf(result, result_size, "Error: Cannot access device");
        return result;
    }

    STORAGE_PROPERTY_QUERY spq = {0};
    spq.PropertyId = StorageDeviceProperty;
    spq.QueryType = PropertyStandardQuery;

    BYTE buffer[1024];
    STORAGE_DEVICE_DESCRIPTOR* device_desc = (STORAGE_DEVICE_DESCRIPTOR*)buffer;

    DWORD bytes_returned = 0;
    BOOL success = DeviceIoControl(
        h_device,
        IOCTL_STORAGE_QUERY_PROPERTY,
        &spq,
        sizeof(spq),
        device_desc,
        (DWORD)sizeof(buffer),
        &bytes_returned,
        NULL
    );

    CloseHandle(h_device);

    if (!success || bytes_returned < sizeof(STORAGE_DEVICE_DESCRIPTOR)) {
        DWORD err = GetLastError();
        fprintf(stderr, "DeviceIoControl failed (Error %lu)\n", err);
        snprintf(result, result_size, "Error: Communication failed");
        return result;
    }

    // 为每个字段分配独立的缓冲区
    char vendorStr[51], productStr[51], serialStr[51], revisionStr[51];
    get_device_string(device_desc, device_desc->VendorIdOffset, vendorStr, sizeof(vendorStr));
    get_device_string(device_desc, device_desc->ProductIdOffset, productStr, sizeof(productStr));
    get_device_string(device_desc, device_desc->SerialNumberOffset, serialStr, sizeof(serialStr));
    get_device_string(device_desc, device_desc->ProductRevisionOffset, revisionStr, sizeof(revisionStr));



    snprintf(result, result_size, ">>%s|%s|%s|%s|396|16777216|40|49|%d<<",
        productStr,
        revisionStr,
        serialStr,
        vendorStr,
        (int)device_desc->BusType);

    return result;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <DiskIndex>\n", argv[0]);
        return 1;
    }

    int index = 0;
    char result[1024];
    if (sscanf(argv[1], "%d", &index) != 1) {
        fprintf(stderr, "Invalid disk index.\n");
        return 1;
    }

    printf("%s\n", GetHardDiskInfo(index, result, sizeof(result)));
    return 0;
}
