#include <iostream>
#include "FileSystem.h"
using namespace std;

void FileSystem::create(char *container) {
    //Create container file
    string c(container);
    if(ifstream(container))
        throw runtime_error(c + " already exists");
    fstream fs;
    fs.open(container, fstream::out | fstream::binary | fstream::trunc);
    if(!fs) throw runtime_error(c + " could not be created");

    //Fill container with zeroes
    int nBytes = CONTAINER_SIZE * 1024;
    char disc[nBytes] = {0};
    fs.write((char*)&disc, nBytes * sizeof(char));

    //Create blocks' table
    fs.seekg(CONTAINER_SIZE / BLOCK_SIZE * sizeof(int));
    int endOfBlockTable = END_OF_BLOCKS_TABLE;
    fs.write((char*)&endOfBlockTable, sizeof(int));

    //Create Directory block
    Directory main(Directory::LAST_DIRECTORY_BLOCK, 0);
    saveDirectory(main, fs);

    fs.close();
}

void FileSystem::insert(char *container, char *fileName) {
    checkFileName(fileName);

    //Open files
    fstream cfs = openContainer(container), file;
    file.open(fileName, fstream::in | fstream::binary);
    if(!file) throw runtime_error("Error opening file");

    //Check if *fileName* is unique
    if(findFileByName(fileName, cfs) >= 0) {
        string error(fileName);
        error += string(" already exists in ") + container;
        throw runtime_error(error);
    }

    //Copy file to the container
    long long int fileSize = getFileSize(file);
    int nBlocks = getNumberOfBlocks(fileSize);
    vector<int> fileBlocks = getEmptyBlocks(nBlocks, cfs);
    File newFile(fileName, (int) fileSize-(nBlocks-1)*BLOCK_SIZE*1024, fileBlocks[0]);

    int bufferSize = BLOCK_SIZE * 1024;
    char buffer[bufferSize];
    file.seekg(0, file.beg);
    for(int block : fileBlocks) {
        cout << "Copying data to block: " << block << endl;
        file.read(buffer, bufferSize * sizeof(char));
        seekBlock(block, cfs);
        cfs.write(buffer, (fileSize < bufferSize ? fileSize : bufferSize) * sizeof(char));
        fileSize -= bufferSize;
    }

    //Update Directory block
    Directory dir = getDirectory(cfs);
    if(dir.numOfFiles == 0) {
        saveFileMetadata(newFile, 0, cfs);
    }
    else {
        int slot = getEmptyFileMetadataSlot(cfs);
        saveFileMetadata(newFile, slot, cfs);
    }
    ++dir.numOfFiles;
    saveDirectory(dir, cfs);

    cfs.close();
    file.close();
}

void FileSystem::remove(char *container, char *fileName) {
    checkFileName(fileName);
    fstream fs = openContainer(container);

    int fileIndex = findFileByName(fileName, fs);
    if(fileIndex < 0) throw runtime_error(string(fileName) + " not found");

    File removedFile = getFileMetadata(fileIndex, fs);
    removedFile.isActive = false;
    int block = removedFile.firstDataBlock;
    int temp, zero = 0;
    do {
        seekBlocksTableElement(block, fs);
        fs.read((char*)&temp, sizeof(int));
        seekBlocksTableElement(block, fs);
        fs.write((char*)&zero, sizeof(int));
        block = temp;
    } while (block != LAST_BLOCK_OF_FILE);
    saveFileMetadata(removedFile, fileIndex, fs);

    //Update Directory block
    Directory dir = getDirectory(fs);
    --dir.numOfFiles;
    saveDirectory(dir, fs);

    fs.close();
}

void FileSystem::get(char *container, char *fileName) {
    checkFileName(fileName);
    fstream fs = openContainer(container);

    string fileNameStr(fileName);
    if(ifstream(fileName))
        throw runtime_error(fileNameStr + " already exists in current directory");
    fstream file;
    file.open(fileName, fstream::out | fstream::binary | fstream::trunc);
    if(!file) throw runtime_error("Output file could not be created");

    int fileIndex = findFileByName(fileName, fs);
    if(fileIndex < 0) throw runtime_error(fileNameStr + " not found");

    File requestedFile = getFileMetadata(fileIndex, fs);
    int block = requestedFile.firstDataBlock;
    long long int fileSize = getFileSize(block, requestedFile.lastBlockSize, fs);
    int bufferSize = BLOCK_SIZE * 1024;
    int nBytes;
    char buffer[bufferSize];
    file.seekg(0, file.beg);
    do {
        nBytes = (int) (fileSize > bufferSize ? bufferSize : fileSize);
        seekBlock(block, fs);
        fs.read((char*)&buffer, nBytes * sizeof(char));
        file.write((char*)&buffer, nBytes * sizeof(char));
        seekBlocksTableElement(block, fs);
        fs.read((char*)&block, sizeof(int));
        fileSize -= nBytes;
    } while (block != LAST_BLOCK_OF_FILE);

    fs.close();
    file.close();
}

long long int FileSystem::getFileSize(std::fstream &fs) {
    fs.seekg (0, fs.end);
    long long length = fs.tellg();
    fs.seekg (0, fs.beg);
    return length;
}

void FileSystem::seekBlock(int block, std::fstream &fs) {
    fs.seekg(block*BLOCK_SIZE*1024, fs.beg);
}

int FileSystem::getNumberOfBlocks(int size) {
    float blocks = (float) size/(BLOCK_SIZE*1024);
    return (int) ceil(blocks);
}

void FileSystem::seekFileMetadata(int fileIndex, std::fstream &fs) {
    fs.seekg(BLOCK_SIZE*1024 + sizeof(Directory) + fileIndex * sizeof(File), fs.beg);
}

vector<int> FileSystem::getEmptyBlocks(int nBlocks, std::fstream &fs) {
    fs.seekg(2*sizeof(int), fs.beg);
    vector<int> blocks((unsigned) nBlocks);
    for (int i = 0; i < nBlocks; ++i) {
        do {
            fs.read((char*)&blocks[i], sizeof(int));
            cout << "position: " << fs.tellg()/sizeof(int)-1 << "\tval: " << blocks[i] << endl;
        } while (blocks[i] != 0 || blocks[i] == END_OF_BLOCKS_TABLE);

        if(blocks[i] == END_OF_BLOCKS_TABLE) {
            throw runtime_error("Not enough space available");
        }
        blocks[i] = (int) fs.tellg()/sizeof(int)-1;
    }
    for (int i = 0; i < nBlocks-1; ++i) {
        seekBlocksTableElement(blocks[i], fs);
        fs.write((char*)&blocks[i+1], sizeof(int));
    }
    seekBlocksTableElement(blocks[nBlocks - 1], fs);
    int lastBlock = LAST_BLOCK_OF_FILE;
    fs.write((char*)&lastBlock, sizeof(int));
    return blocks;
}

void FileSystem::seekBlocksTableElement(int index, std::fstream &fs) {
    fs.seekg(index*sizeof(int), fs.beg);
}

FileSystem::Directory FileSystem::getDirectory(std::fstream &fs) {
    Directory dir;
    seekBlock(1, fs);
    fs.read((char*)&dir, sizeof(Directory));
    return dir;
}

void FileSystem::saveDirectory(FileSystem::Directory dir, fstream &fs) {
    seekBlock(1, fs);
    fs.write((char*)&dir, sizeof(Directory));
}

void FileSystem::saveFileMetadata(FileSystem::File file, int index, fstream &fs) {
    seekFileMetadata(index, fs);
    fs.write((char*)&file, sizeof(File));
}

FileSystem::File FileSystem::getFileMetadata(int index, fstream &fs) {
    File temp;
    seekFileMetadata(index, fs);
    fs.read((char*)&temp, sizeof(File));
    return temp;
}

int FileSystem::getEmptyFileMetadataSlot(std::fstream &fs) {
    int index = 0;
    File temp = getFileMetadata(index, fs);
    //TODO: check if it is not a bug- reading zeroes as File object should result in File.isActive set to 0
    while(temp.isActive) {
        ++index;
        temp = getFileMetadata(index, fs);
    }
    return index;
}

int FileSystem::findFileByName(char *fileName, std::fstream &fs) {
    File temp;
    float x = (float) (BLOCK_SIZE * 1024 - sizeof(Directory)) / sizeof(File);
    int nOfFilesInDirectory = (int) floorf(x);
    int searchedFileIndex = -1;
    for (int i = 0; i < nOfFilesInDirectory; ++i) {
        temp = getFileMetadata(i, fs);
        if(strcmp(temp.fileName, fileName) == 0 && temp.isActive) {
            searchedFileIndex = i;
            break;
        }
    }
    return searchedFileIndex;
}

void FileSystem::checkFileName(char *fileName) {
    size_t length = strlen(fileName);
    if (length > 15) throw runtime_error("Filename is too long");
    for (int i = 0; i < length; i++) {
        if (isalnum(fileName[i]) == 0)
            throw runtime_error("Filename can only consist of alphanumeric characters");
    }
}

std::fstream FileSystem::openContainer(char *container) {
    fstream fs;
    fs.open(container, fstream::in | fstream::out | fstream::binary);
    string c(container);
    if(!fs) throw runtime_error("Cannot open " + c);
    if(getFileSize(fs) != CONTAINER_SIZE*1024)
        throw runtime_error(c + " is not a container file or the file is corrupted");
    return fs;
}

long long int FileSystem::getFileSize(int firstBlock, int lastBlockSize, std::fstream &fs) {
    int nBlocks = 0; //TODO: Check if passing fileIndex to this function would not be a better idea
    int block = firstBlock;
    do {
        ++nBlocks;
        seekBlocksTableElement(block, fs);
        fs.read((char*)&block, sizeof(int));
    } while (block != LAST_BLOCK_OF_FILE);
    return (nBlocks-1)*BLOCK_SIZE*1024 + lastBlockSize;
}
