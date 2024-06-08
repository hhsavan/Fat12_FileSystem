#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <ctime>

void FileManager()
{
}

struct Superblock
{
    char name[20];      // 20 bytes
    uint16_t blockSize; // 2 bytes
    uint16_t maxBlocks; // 2 bytes
};

struct FATEntry
{
    uint16_t next;
};

struct BitMap
{
    uint8_t status;
};

// toplam 32 byte
struct FileEntry
{
    uint16_t permissions;      // 2byte first byte R read permission, second byte W write permission
    char fileName[11];         // 11byte 8+3 extension yok
    char password[9];          // 9byte
    uint8_t size;              // 1byte bu dosyanın size'ı
    uint8_t attribute;         // 1byte 0 means directory 1 means file
    uint16_t lastModifiedDate; // 2byte
    uint16_t lastModifiedTime; // 2byte
    uint16_t creationTime;     // 2byte
    uint16_t creationDate;     // 2byte
};
uint16_t getCurrentDate()
{
    time_t t = time(0);
    struct tm *now = localtime(&t);
    uint16_t year = now->tm_year + 1900;
    uint16_t month = now->tm_mon + 1;
    uint16_t day = now->tm_mday;
    return (year - 1980) << 9 | month << 5 | day; // FAT date format: YYYYYYYMMMMDDDDD
}

uint16_t getCurrentTime()
{
    time_t t = time(0);
    struct tm *now = localtime(&t);
    uint16_t hour = now->tm_hour;
    uint16_t minute = now->tm_min;
    uint16_t second = now->tm_sec / 2; // FAT time format: HHHHHMMMMMMSSSSS (2-second increments)
    return hour << 11 | minute << 5 | second;
}

void displayDate(uint16_t date)
{
    uint16_t year = 1980 + ((date >> 9) & 0x7F);
    uint16_t month = (date >> 5) & 0x0F;
    uint16_t day = date & 0x1F;
    std::cout << "Date: " << year << "-" << month << "-" << day << std::endl;
}

void displayTime(uint16_t time)
{
    uint16_t hour = (time >> 11) & 0x1F;
    uint16_t minute = (time >> 5) & 0x3F;
    uint16_t second = (time & 0x1F) * 2;
    std::cout << "Time: " << hour << ":" << minute << ":" << second << std::endl;
}
class FileSystem
{
private:
    Superblock superblock;
    std::vector<FATEntry> fat;
    std::vector<BitMap> bitMap;
    std::vector<FileEntry> root;
    std::vector<char> emptyBlock;
    std::vector<char> dataBlocks;
    bool block_1kb = true; // true 1kb false 512byte

public:
    FileSystem(const char *name, uint16_t blockSize, uint16_t maxBlocks)
    {
        if (blockSize == 512)
            block_1kb = false;
        // supperblock initialization
        strncpy(superblock.name, name, sizeof(superblock.name));
        superblock.blockSize = blockSize;
        superblock.maxBlocks = maxBlocks;

        //! 1- fat
        fat.resize(maxBlocks);
        // Initialize FAT
        for (auto &entry : fat)
        {
            entry.next = 0x00; // All blocks are free initially
        }

        //! 2- initialize bitmap table
        if (block_1kb)
            bitMap.resize(511); // block 1kb
        else
            bitMap.resize(509); // block 512byte

        for (auto &entry : bitMap)
        {
            entry.status = 0x00; // Bitmap 0 unused, 1 used
        }

        //! 3- empty block
        if (block_1kb)
            emptyBlock.resize(515); // block 1kb
        else
            emptyBlock.resize(541); // block 512byte

        for (auto &entry : emptyBlock)
        {
            entry = 0x00;
        }

        //! 4- root
        std::cout << sizeof(FileEntry) << std::endl;
        root.resize(128); // 128* 32 byte root
        for (auto &entry : root)
        {
            entry.permissions = 0x00;
            entry.creationTime = 0x00;
            entry.creationDate = 0x00;
            entry.lastModifiedTime = 0x00;
            entry.lastModifiedDate = 0x00;
        }

        // Set the current date and time for the first root entry
        root[0].creationDate = getCurrentDate();
        root[0].creationTime = getCurrentTime();
        root[0].lastModifiedDate = root[0].creationDate;
        root[0].lastModifiedTime = root[0].creationTime;

//        std::cout << "Root[0] - creationDate: " << root[0].creationDate
//                  << ", creationTime: " << root[0].creationTime
//                  << ", lastModifiedDate: " << root[0].lastModifiedDate
//                  << ", lastModifiedTime: " << root[0].lastModifiedTime << std::endl;

        //! 5- data block
        dataBlocks.resize(maxBlocks * blockSize);
        for (auto &entry : dataBlocks)
        {
            entry = 0x00;
        }
    }

    void createFileSystem(const char *filePath)
    {
        std::ofstream fs(filePath, std::ios::binary);
        if (!fs)
        {
            std::cerr << "Failed to create file system file.\n";
            return;
        }

        //! 1- Write Superblock 24Byte
        fs.write(reinterpret_cast<char *>(&superblock), sizeof(superblock));

        //! 2- Write FAT
        // fat: n*2byte = 4083*2byte
        for (const auto &entry : fat)
        {
            fs.write(reinterpret_cast<const char *>(&entry.next), sizeof(entry.next));
        }

        //! 3- Bitmap: 512b lık blocklarda 1019 byte, 1kblık blocklarda 511 byte
        for (const auto &entry : bitMap)
        {
            fs.write(reinterpret_cast<const char *>(&entry.status), sizeof(entry.status));
        }

        //! 4- boş block koymak
        fs.write(emptyBlock.data(), emptyBlock.size());

        //! 5- Rootu koymak
        for (const auto &fileEntry : root)
        {
            fs.write(reinterpret_cast<const char *>(&fileEntry), sizeof(fileEntry));
        }

        // Write Data Blocks
        fs.write(dataBlocks.data(), dataBlocks.size());

        fs.close();
    }

};

int main(int argc, char *argv[])
{
    if (argc < 3)
    {
        std::cerr << "Usage: " << argv[0] << " <block size> <file system name>" << std::endl;
        return 1;
    }
    displayDate(getCurrentDate());
    displayTime(getCurrentTime());
    int maxBlockSize = 0;

    std::string argv1 = argv[1];
    int blockbyte;
    if (argv1 == "1")
    {
        maxBlockSize = 4083;
        blockbyte=1024;
    }
    else if (argv1 == "0.5" || argv1 == "0,5")
    {
        maxBlockSize = 4071;
        blockbyte=512;
    }
    else
    {
        std::cout << "Error!" << std::endl;
        return 1;
    }

    FileSystem fs("linuxFile", blockbyte, maxBlockSize);
    fs.createFileSystem(argv[2]);

    return 0;
}
