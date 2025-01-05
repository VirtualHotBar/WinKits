#include <windows.h>
#include <stdio.h>

BOOL GetDiskNumberByDriveLetter(char driveLetter, DWORD *diskNumber)
{ // 需要管理员权限
    BOOL result = FALSE;
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    DWORD bytesReturned = 0;
    STORAGE_DEVICE_NUMBER deviceNumber;

    char devicePath[MAX_PATH];
    sprintf(devicePath, "\\\\.\\%c:", driveLetter);

    hDevice = CreateFileA(devicePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hDevice == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
        return FALSE;
    }

    result = DeviceIoControl(hDevice, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &deviceNumber, sizeof(deviceNumber), &bytesReturned, NULL);
    if (result)
    {
        *diskNumber = deviceNumber.DeviceNumber;
    }
    CloseHandle(hDevice);
    return result;
}

char *GetLetterByHD(int hdIndex)
{
    static char driveLetters[256] = {0};
    driveLetters[0] = '\0';
    DWORD drives = GetLogicalDrives();
    for (char letter = 'A'; letter <= 'Z'; letter++)
    {
        if (drives & (1 << (letter - 'A')))
        {
            char rootPath[4] = {letter, ':', '\\', '\0'};
            if (GetDriveType(rootPath) == DRIVE_FIXED)
            {

                DWORD diskNumber;

                if (GetDiskNumberByDriveLetter(letter, &diskNumber) && diskNumber == hdIndex)
                {
                    char volumePath[8];
                    sprintf(volumePath, "%c:", letter);
                    strcat(driveLetters, volumePath);
                    strcat(driveLetters, " ");
                }
            }
        }
    }
    return driveLetters;
}

int main() {
    HANDLE hDevice = INVALID_HANDLE_VALUE;
    DWORD bytesReturned = 0;
    STORAGE_PROPERTY_QUERY query;
    BYTE buffer[1024] = {0};
    PSTORAGE_DEVICE_DESCRIPTOR deviceDescriptor = (PSTORAGE_DEVICE_DESCRIPTOR)buffer;
    int i = 0;

    while (1) {
        char devicePath[16];
        sprintf(devicePath, "\\\\.\\PhysicalDrive%d", i);

        hDevice = CreateFileA(devicePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        if (hDevice == INVALID_HANDLE_VALUE) {
            break;
        }

        query.PropertyId = StorageDeviceProperty;
        query.QueryType = PropertyStandardQuery;

        if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), buffer, sizeof(buffer), &bytesReturned, NULL)) {
            const char* busType;
            switch (deviceDescriptor->BusType) {
                case BusTypeAta: busType = "ATA"; break;
                case BusTypeSata: busType = "SATA"; break;
                case BusTypeScsi: busType = "SCSI"; break;
                case BusTypeNvme: busType = "NVMe"; break;
                case BusTypeUsb: busType = "USB"; break;
                default: busType = "Unknown"; break;
            }

            DISK_GEOMETRY_EX diskGeometry;
            if (DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &diskGeometry, sizeof(diskGeometry), &bytesReturned, NULL)) {
                ULONGLONG diskSize = diskGeometry.DiskSize.QuadPart;

                // 获取分区信息
                PARTITION_INFORMATION_EX partitionInfo;
                if (DeviceIoControl(hDevice, IOCTL_DISK_GET_PARTITION_INFO_EX, NULL, 0, &partitionInfo, sizeof(partitionInfo), &bytesReturned, NULL)) {
                    const char* partitionStyle;
                    switch (partitionInfo.PartitionStyle) {
                        case PARTITION_STYLE_MBR: partitionStyle = "MBR"; break;
                        case PARTITION_STYLE_GPT: partitionStyle = "GPT"; break;
                        default: partitionStyle = "RAW"; break;
                    }


                    printf("%d|%s|%s|%llu|%s|%s\n", i, (char*)(buffer + deviceDescriptor->ProductIdOffset), busType, diskSize, partitionStyle, GetLetterByHD(i));
                } 
            }
        }
        CloseHandle(hDevice);
        i++;
    }
    return 0;
}