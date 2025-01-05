#include <windows.h>
#include <stdio.h>

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
                    printf("%d|%s|%s|%llu|%s\n", i, (char*)(buffer + deviceDescriptor->ProductIdOffset), busType, diskSize, partitionStyle);
                } 
            }
        }
        CloseHandle(hDevice);
        i++;
    }
    return 0;
}