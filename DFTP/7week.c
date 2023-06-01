#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

void securelyDeleteFile(char* fileName) {
    HANDLE fileHandle = CreateFile(fileName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (fileHandle == INVALID_HANDLE_VALUE) {
        return;
    }

    LARGE_INTEGER fileSize;
    GetFileSizeEx(fileHandle, &fileSize);

    DWORD clusterSize;
    GetDiskFreeSpace(NULL, NULL, &clusterSize, NULL, NULL);

    DWORD sectorsPerCluster;
    DWORD bytesPerSector;
    DWORD numberOfFreeClusters;
    DWORD totalNumberOfClusters;
    GetDiskFreeSpace(NULL, &sectorsPerCluster, &bytesPerSector, &numberOfFreeClusters, &totalNumberOfClusters);

    DWORD bytesToWipe = fileSize.QuadPart;
    DWORD bytesPerCluster = clusterSize * sectorsPerCluster;

    DWORD clusterCount = bytesToWipe / bytesPerCluster;
    if (bytesToWipe % bytesPerCluster) {
        ++clusterCount;
    }

    DWORD* clusters = malloc(sizeof(DWORD) * clusterCount);
    if (clusters == NULL) {
        return;
    }

    for (DWORD i = 0; i < clusterCount; i++) {
        DWORD randomCluster;
        do {
            if (!CryptAcquireContext(NULL, NULL, NULL, PROV_RSA_FULL, 0)) {
                break;
            }
            if (!CryptGenRandom(NULL, sizeof(DWORD), (BYTE*)&randomCluster)) {
                break;
            }
            if (!CryptReleaseContext(NULL, 0)) {
                break;
            }

            randomCluster = randomCluster % totalNumberOfClusters;
        } while (randomCluster < 2);

        clusters[i] = randomCluster;
    }

    for (DWORD i = 0; i < clusterCount; i++) {
        DWORD sectorCount = bytesPerCluster / bytesPerSector;
        BYTE* buffer = malloc(bytesPerCluster);
        if (buffer == NULL) {
            continue;
        }

        DWORD sectorNumber = clusters[i] * sectorsPerCluster;
        SetFilePointer(fileHandle, sectorNumber * bytesPerSector, NULL, FILE_BEGIN);

        for (DWORD j = 0; j < sectorCount; j++) {
            DWORD bytesWritten;
            WriteFile(fileHandle, buffer, bytesPerSector, &bytesWritten, NULL);
        }

        free(buffer);
    }

    free(clusters);

    CloseHandle(fileHandle);
    DeleteFile(fileName);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename>", argv[0]);
        return 1;
    }

    securelyDeleteFile(argv[1]);

    return 0;
}