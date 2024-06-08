#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <ctime>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>
//#include "include/helpers.hpp"

using namespace std;

// Function declarations
uint16_t getCurrentDate();
uint16_t getCurrentTime();
std::string displayDate(uint16_t date);
std::string displayTime(uint16_t time);
std::string fileSystemdat;
// Define the file system structures and functions
struct FileEntry {
    uint16_t permissions;      // 2 bytes
    char fileName[11];         // 11 bytes
    char password[9];          // 9 bytes
    uint8_t size;              // 1 byte
    uint8_t attribute;         // 1 byte
    uint16_t lastModifiedDate; // 2 bytes
    uint16_t lastModifiedTime; // 2 bytes
    uint16_t creationTime;     // 2 bytes
    uint16_t creationDate;     // 2 bytes

string toString() const {
    ostringstream oss;

    // Safely convert fileName to string
    std::string safeFileName(fileName, strnlen(fileName, sizeof(fileName)));
    
    // Safely convert password to string
    std::string safePassword(password, strnlen(password, sizeof(password)));

    oss << "File Name: " << safeFileName << "\n"
        << "Permissions: " << (permissions & 0x1 ? "R" : "-")
        << (permissions & 0x2 ? "W" : "-") << "\n"
        << "Password: " << safePassword << "\n"
        /*<< "Size: " << static_cast<int>(size) << " KB\n"*/
        << "Attribute: " << (attribute == 0 ? "Directory" : "File") << "\n"
        << "Last Modified Date: " << displayDate(lastModifiedDate) << "\n"
        << "Last Modified Time: " << displayTime(lastModifiedTime) << "\n"
        << "Creation Time: " << displayTime(creationTime) << "\n"
        << "Creation Date: " << displayDate(creationDate) << "\n" << "\n";
    return oss.str();
}

    string toString2() const {
        ostringstream oss;
        oss << "File Name: " << string(fileName, sizeof(fileName)) << " - ";
        for (int i = 0; i < sizeof(fileName); ++i) {
            oss << hex << setw(2) << setfill('0') << (int)(unsigned char)fileName[i];
        }
        oss << "\nPermissions: " << (permissions & 0x1 ? "R" : "-")
            << (permissions & 0x2 ? "W" : "-") << " - "
            << hex << setw(4) << setfill('0') << permissions
            << "\nPassword: " << string(password, sizeof(password)) << " - ";
        for (int i = 0; i < sizeof(password); ++i) {
            oss << hex << setw(2) << setfill('0') << (int)(unsigned char)password[i];
        }
        oss << "\nSize: " << static_cast<int>(size) << " KB - "
            << hex << setw(2) << setfill('0') << (int)(unsigned char)size
            << "\nAttribute: " << (attribute == 0 ? "Directory" : "File") << " - "
            << hex << setw(2) << setfill('0') << (int)(unsigned char)attribute
            << "\nLast Modified Date: " << displayDate(lastModifiedDate) << " - "
            << hex << setw(4) << setfill('0') << lastModifiedDate
            << "\nLast Modified Time: " << displayTime(lastModifiedTime) << " - "
            << hex << setw(4) << setfill('0') << lastModifiedTime
            << "\nCreation Time: " << displayTime(creationTime) << " - "
            << hex << setw(4) << setfill('0') << creationTime
            << "\nCreation Date: " << displayDate(creationDate) << " - "
            << hex << setw(4) << setfill('0') << creationDate
            << "\n\n";
        return oss.str();
    }
};

struct BitMap {
    uint8_t status;
};

struct FileSystem {
    char name[20];
    uint16_t blockSize;
    uint16_t blockCount;
    vector<BitMap> bitMap;
    vector<FileEntry> rootDirectory;
    vector<uint16_t> FAT;
    vector<char> emptyBlock;    // 515 bytes empty area
    vector<char> dataBlocks;
};

void addToRootDirectory(FileSystem& fs, const FileEntry& entry) {
    for (size_t i = 0; i < fs.rootDirectory.size(); ++i) {
        if (fs.rootDirectory[i].fileName[0] == '\0') { // Check for empty slot
            fs.rootDirectory[i] = entry;
            return;
        }
    }

    std::cerr << "Error: No empty slot available in the root directory." << std::endl;
}
FileEntry* findFileEntry(FileSystem &fs, const std::string &filePath, const std::string &password = "") {
    for (auto &entry : fs.rootDirectory) {
        if (entry.fileName[0] != '\0' && std::strncmp(entry.fileName, filePath.c_str(), sizeof(entry.fileName)) == 0) {
            if (password.empty() || std::strncmp(entry.password, password.c_str(), sizeof(entry.password)) == 0) {
                return &entry;
            } else {
                std::cerr << "Error: Incorrect password for file " << filePath << std::endl;
                return nullptr;
            }
        }
    }
    return nullptr;
}

bool loadFileSystem(const string& fileName, FileSystem& fs) {
    ifstream file(fileName, ios::binary);
    if (!file) {
        cerr << "Error: Cannot open file system." << endl;
        return false;
    }

    file.read(fs.name, sizeof(fs.name));
    file.read(reinterpret_cast<char*>(&fs.blockSize), sizeof(fs.blockSize));
    file.read(reinterpret_cast<char*>(&fs.blockCount), sizeof(fs.blockCount));
    
    fs.FAT.resize(fs.blockCount);
    file.read(reinterpret_cast<char*>(fs.FAT.data()), fs.FAT.size() * sizeof(uint16_t));

    fs.bitMap.resize(511);
    file.read(reinterpret_cast<char*>(fs.bitMap.data()), fs.bitMap.size() * sizeof(BitMap));

    fs.emptyBlock.resize(515);
    file.read(reinterpret_cast<char*>(fs.emptyBlock.data()), fs.emptyBlock.size() * sizeof(uint8_t));

    fs.rootDirectory.resize(128); // Assuming root directory has 128 entries
    for (auto& entry : fs.rootDirectory) {
        file.read(reinterpret_cast<char*>(&entry.permissions), sizeof(entry.permissions));
        file.read(entry.fileName, sizeof(entry.fileName));
        file.read(entry.password, sizeof(entry.password));
        file.read(reinterpret_cast<char*>(&entry.size), sizeof(entry.size));
        file.read(reinterpret_cast<char*>(&entry.attribute), sizeof(entry.attribute));
        file.read(reinterpret_cast<char*>(&entry.lastModifiedDate), sizeof(entry.lastModifiedDate));
        file.read(reinterpret_cast<char*>(&entry.lastModifiedTime), sizeof(entry.lastModifiedTime));
        file.read(reinterpret_cast<char*>(&entry.creationTime), sizeof(entry.creationTime));
        file.read(reinterpret_cast<char*>(&entry.creationDate), sizeof(entry.creationDate));
    }

    streampos currentPos = file.tellg();
    file.seekg(0, ios::end);
    streampos fileSize = file.tellg();
    streampos dataBlocksSize = fileSize - currentPos;
    fs.dataBlocks.resize(static_cast<size_t>(dataBlocksSize));

    file.seekg(currentPos, ios::beg);
    file.read(fs.dataBlocks.data(), fs.dataBlocks.size());

    file.close();
    return true;
}

void saveFileSystem(const string& fileName, const FileSystem& fs) {
    ofstream file(fileName, ios::binary | ios::trunc);
    if (!file) {
        cerr << "Error: Cannot open file system." << endl;
        return;
    }

    file.write(fs.name, sizeof(fs.name));
    file.write(reinterpret_cast<const char*>(&fs.blockSize), sizeof(fs.blockSize));
    file.write(reinterpret_cast<const char*>(&fs.blockCount), sizeof(fs.blockCount));

    for (const auto& entry : fs.FAT) {
        file.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
    }

    for (const auto& entry : fs.bitMap) {
        file.write(reinterpret_cast<const char*>(&entry.status), sizeof(entry.status));
    }

    file.write(fs.emptyBlock.data(), fs.emptyBlock.size());

    for (const auto& entry : fs.rootDirectory) {
        /*if (entry.fileName[0] != '\0') {
            cout << "entry.lastModifiedDate (hex): " << hex << setw(4) << setfill('0') << entry.lastModifiedDate << endl;
            cout << "actual date: " <<displayDate( getCurrentDate())<< endl;
            cout << "actual Time: " <<displayTime( getCurrentTime())<< endl;
            cout << "entry.lastModifiedTime (hex): " << hex << setw(4) << setfill('0') << entry.lastModifiedTime << endl;
            cout << "entry.creationDate (hex): " << hex << setw(4) << setfill('0') << entry.creationDate << endl;
            cout << "entry.creationTime (hex): " << hex << setw(4) << setfill('0') << entry.creationTime << endl;
        }*/

        file.write(reinterpret_cast<const char*>(&entry.permissions), sizeof(entry.permissions));
        file.write(entry.fileName, sizeof(entry.fileName));
        file.write(entry.password, sizeof(entry.password));
        file.write(reinterpret_cast<const char*>(&entry.size), sizeof(entry.size));
        file.write(reinterpret_cast<const char*>(&entry.attribute), sizeof(entry.attribute));
        file.write(reinterpret_cast<const char*>(&entry.lastModifiedDate), sizeof(entry.lastModifiedDate));
        file.write(reinterpret_cast<const char*>(&entry.lastModifiedTime), sizeof(entry.lastModifiedTime));
        file.write(reinterpret_cast<const char*>(&entry.creationTime), sizeof(entry.creationTime));
        file.write(reinterpret_cast<const char*>(&entry.creationDate), sizeof(entry.creationDate));
    }

    file.write(fs.dataBlocks.data(), fs.dataBlocks.size());

    file.close();
}
void dumpe2fs(const FileSystem& fs) {
    cout<<endl;
    cout << "Dumping file system information" << endl;
    cout << "File System Name: " << string(fs.name, sizeof(fs.name)) << endl;
    cout << "Block Size: " << fs.blockSize << " KB" << endl;
    cout << "Block Count: " << fs.blockCount << endl;

    // Count free blocks
    uint32_t freeBlocks = 0;
    for (const auto& block : fs.bitMap) {
        if (block.status == 0) {
            ++freeBlocks;
        }
    }
    cout << "Free Blocks: " << freeBlocks << endl;

    // Count number of files and directories
    uint32_t fileCount = 0, dirCount = 0;
    for (const auto& entry : fs.rootDirectory) {
        if (entry.fileName[0] != '\0') {
            if (entry.attribute == 0) { // Assuming 0 indicates a directory
                ++dirCount;
            } else {
                ++fileCount;
            }
        }
    }
    cout << "Number of Files: " << fileCount << endl;
    cout << "Number of Directories: " << dirCount << endl;

    // List all occupied blocks and file names
    cout << "Occupied Blocks:" << endl;
    for (size_t i = 0; i < fs.rootDirectory.size(); ++i) {
        const auto& entry = fs.rootDirectory[i];
        if (entry.fileName[0] != '\0') {
            cout << "File Name: " << string(entry.fileName, sizeof(entry.fileName)) << " -> Blocks: ";
            uint16_t block = i; // Assuming the index in rootDirectory corresponds to the start block
            while (block != 0 && block < fs.FAT.size()) { // Iterate through FAT chain
                cout << block << " ";
                block = fs.FAT[block];
            }
            cout << endl;
        }
    }
    cout<<endl;

}

bool writeRootElementByIndex(const string& fileName, FileSystem& fs, size_t index, const FileEntry& newEntry) {
    if (index >= fs.rootDirectory.size()) {
        cerr << "Error: Index out of bounds." << endl;
        return false;
    }

    fs.rootDirectory[index] = newEntry;

    fstream file(fileName, ios::binary | ios::in | ios::out);
    if (!file) {
        cerr << "Error: Cannot open file system." << endl;
        return false;
    }

    size_t superblockSize = sizeof(fs.name) + sizeof(fs.blockSize) + sizeof(fs.blockCount);
    size_t fatSize = fs.blockCount * sizeof(uint16_t);
    size_t bitMapSize = fs.bitMap.size() * sizeof(BitMap);
    size_t emptyBlockSize = 515;
    size_t rootDirStart = superblockSize + fatSize + bitMapSize + emptyBlockSize;
    size_t entrySize = sizeof(newEntry.permissions) + sizeof(newEntry.fileName) + sizeof(newEntry.password) +
                       sizeof(newEntry.size) + sizeof(newEntry.attribute) +
                       sizeof(newEntry.lastModifiedDate) + sizeof(newEntry.lastModifiedTime) +
                       sizeof(newEntry.creationTime) + sizeof(newEntry.creationDate);
    size_t entryPos = rootDirStart + index * entrySize;

    file.seekp(entryPos);
    file.write(reinterpret_cast<const char*>(&newEntry.permissions), sizeof(newEntry.permissions));
    file.write(newEntry.fileName, sizeof(newEntry.fileName));
    file.write(newEntry.password, sizeof(newEntry.password));
    file.write(reinterpret_cast<const char*>(&newEntry.size), sizeof(newEntry.size));
    file.write(reinterpret_cast<const char*>(&newEntry.attribute), sizeof(newEntry.attribute));
    file.write(reinterpret_cast<const char*>(&newEntry.lastModifiedDate), sizeof(newEntry.lastModifiedDate));
    file.write(reinterpret_cast<const char*>(&newEntry.lastModifiedTime), sizeof(newEntry.lastModifiedTime));
    file.write(reinterpret_cast<const char*>(&newEntry.creationTime), sizeof(newEntry.creationTime));
    file.write(reinterpret_cast<const char*>(&newEntry.creationDate), sizeof(newEntry.creationDate));

    file.close();
    return true;
}

void printOccupiedRootEntries(const FileSystem& fs) {
    cout << "Occupied Root Directory Entries:" << endl;
    for (const auto& entry : fs.rootDirectory) {
        if (entry.fileName[0] != '\0') {
            cout << "File Name: " << string(entry.fileName, sizeof(entry.fileName)) << endl;
            cout << "Permissions: " << (entry.permissions & 0x1 ? "R" : "-")
                 << (entry.permissions & 0x2 ? "W" : "-") << endl;
            cout << "Size: " << static_cast<int>(entry.size) << " KB" << endl;
            cout << "Attribute: " << (entry.attribute == 0 ? "Directory" : "File") << endl;
            cout << "Last Modified Date: " << entry.lastModifiedDate << " (hex): " << hex << setw(4) << setfill('0') << entry.lastModifiedDate << endl;
            cout << "Last Modified Time: " << entry.lastModifiedTime << " (hex): " << hex << setw(4) << setfill('0') << entry.lastModifiedTime << endl;
            cout << "Creation Date: " << entry.creationDate << " (hex): " << hex << setw(4) << setfill('0') << entry.creationDate << endl;
            cout << "Creation Time: " << entry.creationTime << " (hex): " << hex << setw(4) << setfill('0') << entry.creationTime << endl;
            if (!string(entry.password, sizeof(entry.password)).empty()) {
                cout << "Password Protected: Yes" << endl;
            } else {
                cout << "Password Protected: No" << endl;
            }
            cout << "----------------------------------------" << endl;
        }
    }
}

int findEmptyBlock(const FileSystem &fs) {
    for (size_t i = 0; i < fs.bitMap.size(); ++i) {
        if (fs.bitMap[i].status == 0) {
            return i;
        }
    }
    return -1; // No empty block found
}
void updateFATandBitmap(FileSystem &fs, int blockIndex) {
    fs.FAT[blockIndex] = 0; // Mark the end of the file
    fs.bitMap[blockIndex].status = 1; // Mark the block as used
}

void listDirectory(const FileSystem &fs, const std::string &directoryPath) {
    // For simplicity, we assume directoryPath is always root ("/")
    if (directoryPath != "/") {
        std::cerr << "Error: Only root directory (/) is supported." << std::endl;
        return;
    }

    std::cout << "Contents of " << directoryPath << ":" << std::endl;

    for (const auto &entry : fs.rootDirectory) {
        if (entry.fileName[0] != '\0') { // Check if the entry is valid
            // Print permissions
            std::string permissions;
            permissions += (entry.permissions & 0x1) ? 'r' : '-';
            permissions += (entry.permissions & 0x2) ? 'w' : '-';

            // Print file type
            std::string fileType = (entry.attribute == 0) ? "d" : "-";

            // Print password protection status
            std::string passwordProtected = (entry.password[0] != '\0') ? "Password Protected" : "No Password";

            // Print the entry details
            std::cout << fileType << permissions << "\t"
                      << displayDate(entry.creationDate) << " " << displayTime(entry.creationTime) << "\t"
                      << displayDate(entry.lastModifiedDate) << " " << displayTime(entry.lastModifiedTime) << "\t"
                      << passwordProtected << "\t"
                      << std::string(entry.fileName, sizeof(entry.fileName)) << std::endl;
        }
    }
}


void addFile(FileSystem &fs, const std::string &fileName, const std::vector<char> &fileData) {
    size_t blockSize = fs.blockSize; // Convert block size to bytes
    size_t requiredBlocks = (fileData.size() + blockSize - 1) / blockSize; // Calculate the number of blocks needed

    // Find contiguous empty blocks
    std::vector<int> blocks;
    for (size_t i = 0; i < fs.bitMap.size() && blocks.size() < requiredBlocks; ++i) {
        if (fs.bitMap[i].status == 0) {
            blocks.push_back(i);
        }
    }

    if (blocks.size() < requiredBlocks) {
        std::cerr << "Error: Not enough contiguous blocks available in the data segment." << std::endl;
        return;
    }

    // Step 2: Write data to the empty blocks
    size_t dataOffset = 0;
    for (size_t i = 0; i < blocks.size(); ++i) {
        size_t start = blocks[i] * blockSize;
        size_t bytesToWrite = std::min(blockSize, fileData.size() - dataOffset);
        std::copy(fileData.begin() + dataOffset, fileData.begin() + dataOffset + bytesToWrite, fs.dataBlocks.begin() + start);
        dataOffset += bytesToWrite;
    }

    // Step 3: Update the FAT and bitmap
    for (size_t i = 0; i < blocks.size(); ++i) {
        fs.bitMap[blocks[i]].status = 1;
        if (i == blocks.size() - 1) {
            fs.FAT[blocks[i]] = 0; // End of file
        } else {
            fs.FAT[blocks[i]] = blocks[i + 1];
        }
    }

    // Step 4: Add entry to the root directory
    FileEntry newEntry;
    std::strncpy(newEntry.fileName, fileName.c_str(), sizeof(newEntry.fileName) - 1);
    newEntry.fileName[sizeof(newEntry.fileName) - 1] = '\0';
    newEntry.size = (fileData.size() + 1023) / 1024; // Size in KB (1 block = 1 KB)
    newEntry.attribute = 1; // Mark as file
    newEntry.lastModifiedDate = getCurrentDate();
    newEntry.lastModifiedTime = getCurrentTime();
    newEntry.creationDate = getCurrentDate();
    newEntry.creationTime = getCurrentTime();
    newEntry.permissions = 0x3; // Default permissions: RW
    newEntry.password[0] = '\0'; // No password by default

    addToRootDirectory(fs, newEntry);

    // Print the details of the added file
    std::cout << "Added file details:\n" << newEntry.toString() << std::endl;

    // Save the file system after adding the file
    saveFileSystem(fileSystemdat, fs);
}

std::string readDataFromBlock(FileSystem &fs, int blockIndex, size_t fileSize) {
    cout << "fileSize: " << fileSize << endl;
    cout << "blockIndex: " << blockIndex << endl;
    size_t blockSize = fs.blockSize; // Block size in bytes
    cout << "blockSize: " << blockSize << endl;
    size_t totalBlocks = (fileSize + blockSize - 1) / blockSize; // Calculate total blocks needed
    cout << "totalBlocks: " << totalBlocks << endl;
    std::vector<char> data(fileSize);

    size_t dataIndex = 0;
    while (blockIndex != 0 && blockIndex < fs.FAT.size() && totalBlocks > 0) {
        size_t start = blockIndex * blockSize;
        size_t bytesToRead = std::min(blockSize, fileSize - dataIndex);
        std::copy(fs.dataBlocks.begin() + start, fs.dataBlocks.begin() + start + bytesToRead, data.begin() + dataIndex);
        dataIndex += bytesToRead;
        blockIndex = fs.FAT[blockIndex];
        --totalBlocks;
    }

    return std::string(data.begin(), data.end());
}
/*
FileEntry* findFileEntry(FileSystem &fs, const std::string &filePath) {
    for (auto &entry : fs.rootDirectory) {
        if (entry.fileName[0] != '\0' && std::strncmp(entry.fileName, filePath.c_str(), sizeof(entry.fileName)) == 0) {
            cout<<entry.toString();
            return &entry;
        }
    }
    cout<<"burda\n";
    return nullptr;
}*/
void writeToFile(const std::string& filename, const std::string& content) {
    // Create an ofstream object
    std::ofstream outFile;
    outFile.open(filename);

    if (!outFile) {
        std::cerr << "Error: Could not open the file " << filename << std::endl;
        return;
    }

    for (char ch : content) {
        if (ch != '\x00') {
//           outFile << ch;
            cout<<ch; //sill
        }
    }
//        cout<<"\n";
    outFile.close();

    if (outFile.fail()) {
        std::cerr << "Error: Could not close the file " << filename << std::endl;
    } else {
        std::cout << "Content successfully written to " << filename << std::endl;
    }
}
void readFile(FileSystem &fs, const std::string &filePath, const std::string &outputFileName, const std::string& password="") {
        

    // Step 1: Find the file entry
    FileEntry* entry = findFileEntry(fs, filePath);
    if (!entry) {
        std::cerr << "Error: File not found in the file system." << std::endl;
        return;
    }
    //check password
    if (entry->password[0] != '\0') { // Check if the password is set
        if (password.empty()) {
            std::cerr << "Error: This file is password protected. Please provide a password." << std::endl;
            return;
        }
        if (password != entry->password) {
            std::cerr << "Error: Invalid password." << std::endl;
            return;
        }
    }
    // Check read permission
    if (!(entry->permissions & 0x1)) { // Check if read permission is not set
        std::cerr << "Error: Read permission denied for file " << filePath << "." << std::endl;
        return;
    }

    // Step 2: Read data from the block
    std::string fileData = readDataFromBlock(fs, entry->attribute, entry->size * 1024); // Size in bytes

    // Print the data to the screen
    std::cout << "Read data from file " << filePath << ":\n" << fileData << std::endl;

    // Step 3: Write data to the output file
    writeToFile(outputFileName, fileData);
}

bool deleteFile(FileSystem &fs, const std::string &filePath) {
    // Step 1: Find the file entry
    FileEntry* entry = findFileEntry(fs, filePath);
    if (!entry) {
        std::cerr << "Error: File not found in the file system." << std::endl;
        return false;
    }

    // Step 2: Free the data blocks associated with the file
    int blockIndex = entry->attribute; // Starting block of the file
    while (blockIndex != 0 && blockIndex < fs.FAT.size()) {
        int nextBlock = fs.FAT[blockIndex];
        fs.FAT[blockIndex] = 0;
        fs.bitMap[blockIndex].status = 0; // Mark block as free

        // Clear the data block
        std::fill(fs.dataBlocks.begin() + (blockIndex * fs.blockSize), fs.dataBlocks.begin() + ((blockIndex + 1) * fs.blockSize), 0);

        blockIndex = nextBlock;
    }

    // Step 3: Remove the file entry from the root directory
    std::memset(entry, 0, sizeof(FileEntry)); // Clear the file entry

    return true;
}
void addPassword(FileSystem &fs, const std::string &filePath, const std::string &password,const std::string &passwordOld) {
    // Find the file entry
    FileEntry* entry = findFileEntry(fs, filePath);
    if (!entry) {
        std::cerr << "Error: File not found in the file system." << std::endl;
        return;
    }
    if (entry->password[0] != '\0') { // Check if the passwordOld is set
        if (passwordOld.empty()) {
            std::cerr << "Error: This file is password protected. Please provide a password." << std::endl;
            return;
        }
        if (passwordOld != entry->password) {
            std::cerr << "Error: Invalid password." << std::endl;
            return;
        }
    }
    // Update the password
    std::strncpy(entry->password, password.c_str(), sizeof(entry->password) - 1);
    entry->password[sizeof(entry->password) - 1] = '\0';

    // Print the updated file details
    std::cout << "Updated file details:\n" << entry->toString() << std::endl;

    // Save the file system after updating the password
    saveFileSystem(fileSystemdat, fs);
}

std::string getChmod(FileSystem &fs, const std::string &filePath) {
    FileEntry* entry = findFileEntry(fs, filePath);
    if (!entry) {
        std::cerr << "Error: File not found in the file system." << std::endl;
        return "";
    }

    std::string permissions;
    permissions += (entry->permissions & 0x1) ? 'r' : '-';
    permissions += (entry->permissions & 0x2) ? 'w' : '-';
    return permissions;
}


std::string getPassword(FileSystem &fs, const std::string &filePath) {
    FileEntry* entry = findFileEntry(fs, filePath);
    if (!entry) {
        std::cerr << "Error: File not found in the file system." << std::endl;
        return "";
    }

    return std::string(entry->password, sizeof(entry->password));
}


bool checkPassword(FileEntry *entry, const std::string &password) {
    return std::strncmp(entry->password, password.c_str(), sizeof(entry->password)) == 0;
}
void chmodFile(FileSystem &fs, const std::string &filePath, uint16_t permissions, const std::string &password="") {
    FileEntry* entry = findFileEntry(fs, filePath);
    if (!entry) {
        std::cerr << "Error: File not found in the file system." << std::endl;
        return;
    }
    //check password
    if (entry->password[0] != '\0') { // Check if the password is set
        if (password.empty()) {
            std::cerr << "Error: This file is password protected. Please provide a password." << std::endl;
            return;
        }
        if (password != entry->password) {
            std::cerr << "Error: Invalid password." << std::endl;
            return;
        }
    }
    entry->permissions = permissions;

    // Print the updated file details
    std::cout << "Updated file details:\n" << entry->toString() << std::endl;

    // Save the file system after updating the permissions
    saveFileSystem(fileSystemdat, fs);
}

void initializeFileSystem(FileSystem &fs) {
    // Initialize root directory
    FileEntry rootEntry;
    std::strncpy(rootEntry.fileName, "/", sizeof(rootEntry.fileName) - 1);
    rootEntry.fileName[sizeof(rootEntry.fileName) - 1] = '\0';
    rootEntry.size = 0;
    rootEntry.attribute = 0; // Directory
    rootEntry.lastModifiedDate = getCurrentDate();
    rootEntry.lastModifiedTime = getCurrentTime();
    rootEntry.creationDate = getCurrentDate();
    rootEntry.creationTime = getCurrentTime();
    rootEntry.permissions = 0x3; // Default permissions: RW
    rootEntry.password[0] = '\0'; // No password by default

    addToRootDirectory(fs, rootEntry);
/*
    // Initialize user directory
    FileEntry usrEntry;
    std::strncpy(usrEntry.fileName, "usr", sizeof(usrEntry.fileName) - 1);
    usrEntry.fileName[sizeof(usrEntry.fileName) - 1] = '\0';
    usrEntry.size = 0;
    usrEntry.attribute = 0; // Directory
    usrEntry.lastModifiedDate = getCurrentDate();
    usrEntry.lastModifiedTime = getCurrentTime();
    usrEntry.creationDate = getCurrentDate();
    usrEntry.creationTime = getCurrentTime();
    usrEntry.permissions = 0x3; // Default permissions: RW
    usrEntry.password[0] = '\0'; // No password by default

    addToRootDirectory(fs, usrEntry);
*/
    // Save the file system after initialization
    saveFileSystem(fileSystemdat, fs);
}

/*
    size_t pos = directoryPath.find_last_of('/');
    std::string parentPath = (pos == std::string::npos) ? "" : directoryPath.substr(0, pos);
    std::string dirName = (pos == std::string::npos) ? directoryPath : directoryPath.substr(pos + 1);

    // Check if the parent directory exists
    if (!parentPath.empty()) {
        FileEntry* parentEntry = findFileEntry(fs, parentPath);
        if (!parentEntry || parentEntry->attribute != 0) {
            std::cerr << "Error: Parent directory does not exist." << std::endl;
            return;
        }
    }

*/
void makeDirectory(FileSystem &fs, const std::string &directoryPath) {
    // Normalize the path to handle leading slashes
    std::string normalizedPath = directoryPath;
    if (!normalizedPath.empty() && normalizedPath[0] == '/') {
        normalizedPath = normalizedPath.substr(1);
    }

    // Split the directoryPath into parent path and directory name
    size_t pos = normalizedPath.find_last_of('/');
    std::string parentPath = (pos == std::string::npos) ? "" : normalizedPath.substr(0, pos);
    std::string dirName = (pos == std::string::npos) ? normalizedPath : normalizedPath.substr(pos + 1);

    // Check if the parent directory exists if the parentPath is not empty
    if (!parentPath.empty()) {
        bool parentExists = false;
        for (const auto &entry : fs.rootDirectory) {
            if (entry.fileName[0] != '\0' && std::strncmp(entry.fileName, parentPath.c_str(), sizeof(entry.fileName)) == 0 && entry.attribute == 0) {
                parentExists = true;
                break;
            }
        }
        if (!parentExists) {
            std::cerr << "Error: Parent directory does not exist." << std::endl;
            return;
        }
    }

    // Check if the directory already exists
    for (const auto &entry : fs.rootDirectory) {
        if (entry.fileName[0] != '\0' && std::strncmp(entry.fileName, normalizedPath.c_str(), sizeof(entry.fileName)) == 0) {
            std::cerr << "Error: Directory already exists." << std::endl;
            return;
        }
    }

    // Create the new directory entry
    FileEntry newEntry;
    std::strncpy(newEntry.fileName, normalizedPath.c_str(), sizeof(newEntry.fileName) - 1);
    newEntry.fileName[sizeof(newEntry.fileName) - 1] = '\0';
    newEntry.size = 0; // Directory size is 0
    newEntry.attribute = 0; // Mark as directory
    newEntry.lastModifiedDate = getCurrentDate();
    newEntry.lastModifiedTime = getCurrentTime();
    newEntry.creationDate = getCurrentDate();
    newEntry.creationTime = getCurrentTime();
    newEntry.permissions = 0x3; // Default permissions: RW
    newEntry.password[0] = '\0'; // No password by default

    addToRootDirectory(fs, newEntry);

    // Save the file system after adding the directory
    saveFileSystem(fileSystemdat, fs);
    std::cout << "Directory " << directoryPath << " created successfully." << std::endl;
}


bool removeDirectory(FileSystem &fs, const std::string &directoryPath) {
    // Find the directory entry
    FileEntry* entry = findFileEntry(fs, directoryPath);
    if (!entry) {
        std::cerr << "Error: Directory not found in the file system." << std::endl;
        return false;
    }

    // Check if the directory is empty
    for (const auto &subEntry : fs.rootDirectory) {
        if (subEntry.fileName[0] != '\0' && std::strncmp(subEntry.fileName, directoryPath.c_str(), sizeof(subEntry.fileName) - 1) == 0 && subEntry.attribute != 0) {
            std::cerr << "Error: Directory is not empty." << std::endl;
            return false;
        }
    }

    // Remove the directory entry from the root directory
    std::memset(entry, 0, sizeof(FileEntry)); // Clear the directory entry

    // Save the file system after removing the directory
    saveFileSystem(fileSystemdat, fs);
    std::cout << "Directory " << directoryPath << " removed successfully." << std::endl;
    return true;
}


int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: fileSystemOper fileSystem.data operation [parameters]" << std::endl;
        return 1;
    }
    std::string filesystemPath = argv[1];
    std::string operation = argv[2];
    fileSystemdat = filesystemPath;
    FileSystem fs;
    if (!loadFileSystem(filesystemPath, fs)) {
        std::cerr << "Failed to load file system" << std::endl;
        return 1;
    }
    //initializeFileSystem(fs);

    if (operation == "dumpe2fs") {
        dumpe2fs(fs);
    } else if (operation == "mkdir") {
        if (argc < 4) {
            std::cerr << "Usage: fileSystemOper fileSystem.data mkdir <path>" << std::endl;
            return 1;
        }
        std::string directoryPath = argv[3];
        makeDirectory(fs, directoryPath);
    } else if (operation == "rmdir") {
        if (argc < 4) {
            std::cerr << "Usage: fileSystemOper fileSystem.data rmdir <path>" << std::endl;
            return 1;
        }
        std::string directoryPath = argv[3];
        removeDirectory(fs, directoryPath);
    } else if (operation == "write") {
        if (argc < 5) {
            std::cerr << "Usage: fileSystemOper fileSystem.data write <path> <linuxFile>" << std::endl;
            return 1;
        }
        std::string filePath = argv[3];
        std::string linuxFile = argv[4];

        std::ifstream inFile(linuxFile, std::ios::binary);
        if (!inFile) {
            std::cerr << "Error: Cannot open the specified Linux file." << std::endl;
            return 1;
        }

        std::vector<char> fileData((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
        inFile.close();

        addFile(fs, filePath, fileData);
        saveFileSystem(filesystemPath, fs);
    } else if (operation == "read") {
        if (argc < 5) {
            std::cerr << "Usage: fileSystemOper fileSystem.data read <path> <outputFile>" << std::endl;
            return 1;
        }
        std::string filePath = argv[3];
        std::string outputFile = argv[4];
        std::string password ="";

        if (argc ==6) {
            password = argv[5];
        }

        readFile(fs, filePath, outputFile,password);
    } else if (operation == "del") {
        if (argc < 4) {
            std::cerr << "Usage: fileSystemOper fileSystem.data del <path>" << std::endl;
            return 1;
        }
        std::string filePath = argv[3];

        if (deleteFile(fs, filePath)) {
            std::cout << "File " << filePath << " deleted successfully." << std::endl;
            saveFileSystem(filesystemPath, fs);
        } else {
            std::cerr << "Failed to delete file " << filePath << "." << std::endl;
        }
    } else if (operation == "addpw") {
        if (argc < 5) {
            std::cerr << "Usage: fileSystemOper fileSystem.data addpw <path> <password>" << std::endl;
            return 1;
        }
        //cout<<"here_password\n";
        std::string filePath = argv[3];
        std::string password = argv[4];
        std::string passwordOld ="";

        if (argc ==6) {
            passwordOld = argv[5];
        }
        addPassword(fs, filePath, password,passwordOld);
        saveFileSystem(filesystemPath, fs);
    } else if (operation == "chmod") {
        if (argc < 5) {
            std::cerr << "Usage: fileSystemOper fileSystem.data chmod <path> <permissions>" << std::endl;
            return 1;
        }
        std::string filePath = argv[3];
        std::string permissionsStr = argv[4];

        std::string password="";

        if (argc ==6) {
            password = argv[5];
        }
        uint16_t permissions = 0;
        if (permissionsStr.find('r') != std::string::npos) permissions |= 0x1;
        if (permissionsStr.find('w') != std::string::npos) permissions |= 0x2;

        chmodFile(fs, filePath, permissions,password);
        saveFileSystem(filesystemPath, fs);
    } else if (operation == "dir") {
        if (argc < 4) {
            std::cerr << "Usage: fileSystemOper fileSystem.data dir <path>" << std::endl;
            return 1;
        }
        std::string directoryPath = argv[3];
        listDirectory(fs, directoryPath);
    }  
    else {
        std::cerr << "Unknown operation" << std::endl;
        return 1;
    }

    return 0;
}



uint16_t getCurrentDate() {
    time_t t = time(0);
    struct tm *now = localtime(&t);
    uint16_t year = now->tm_year + 1900;
    uint16_t month = now->tm_mon + 1;
    uint16_t day = now->tm_mday;
    return (year - 1980) << 9 | month << 5 | day; // FAT date format: YYYYYYYMMMMDDDDD
}

uint16_t getCurrentTime() {
    time_t t = time(0);
    struct tm *now = localtime(&t);
    uint16_t hour = now->tm_hour;
    uint16_t minute = now->tm_min;
    uint16_t second = now->tm_sec / 2; // FAT time format: HHHHHMMMMMMSSSSS (2-second increments)
    return hour << 11 | minute << 5 | second;
}

std::string displayDate(uint16_t date) {
    uint16_t year = 1980 + ((date >> 9) & 0x7F);
    uint16_t month = (date >> 5) & 0x0F;
    uint16_t day = date & 0x1F;
    
    std::ostringstream oss;
    oss << year << "-" 
        << std::setfill('0') << std::setw(2) << month << "-" 
        << std::setfill('0') << std::setw(2) << day;
    return oss.str();
}

std::string displayTime(uint16_t time) {
    uint16_t hour = (time >> 11) & 0x1F;
    uint16_t minute = (time >> 5) & 0x3F;
    uint16_t second = (time & 0x1F) * 2;
    
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(2) << hour << ":" 
        << std::setfill('0') << std::setw(2) << minute << ":" 
        << std::setfill('0') << std::setw(2) << second;
    return oss.str();
}
