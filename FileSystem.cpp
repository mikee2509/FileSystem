#include <iostream>
#include "FileSystem.h"
using namespace std;

void FileSystem::create(char *container) {
    //Create container file
    string c(container);
    if(ifstream(container))
        throw runtime_error(c + " already exists");

    ContainerStream cs;
    cs.open(container, ContainerStream::out | ContainerStream::binary);
    if(!cs) throw runtime_error(c + " could not be created");

    //Fill container with zeroes
    int nBytes = CONTAINER_SIZE * 1024;
    char disc[nBytes] = {0};
    cs.write((char*)&disc, nBytes * sizeof(char));

    //Create blocks' table
    cs.seekg(CONTAINER_SIZE / BLOCK_SIZE * sizeof(int));
    int endOfBlockTable = END_OF_BLOCKS_TABLE;
    cs.write((char*)&endOfBlockTable, sizeof(int));

    //Create Directory block
    Directory main(Directory::LAST_DIRECTORY_BLOCK, 0);
    cs.saveDirectory(main);

    cs.close();
}

void FileSystem::insert(char *container, char *fileName) {
    checkFileName(fileName);

    //Open files
    ContainerStream cs = openContainer(container);
    fstream file;
    file.open(fileName, fstream::in | fstream::binary);
    if(!file) throw runtime_error("Error opening file");

    //Check if *fileName* is unique
    if(cs.findFileByName(fileName) >= 0) {
        string error(fileName);
        error += string(" already exists in ") + container;
        throw runtime_error(error);
    }

    //Copy file to the container
    long long int fileSize = getFileSize(file);
    int nBlocks = getNumberOfBlocks(fileSize);
    vector<int> fileBlocks = cs.getEmptyBlocks(nBlocks);
    File newFile(fileName, (int) fileSize-(nBlocks-1)*BLOCK_SIZE*1024, fileBlocks[0]);

    int bufferSize = BLOCK_SIZE * 1024;
    char buffer[bufferSize];
    file.seekg(0, file.beg);
    for(int block : fileBlocks) {
        cout << "Copying data to block: " << block << endl;
        file.read(buffer, bufferSize * sizeof(char));
        cs.seekBlock(block);
        cs.write(buffer, (fileSize < bufferSize ? fileSize : bufferSize) * sizeof(char));
        fileSize -= bufferSize;
    }

    //Update Directory block
    Directory dir = cs.getDirectory();
    if(dir.numOfFiles == 0) {
        cs.saveFileMetadata(newFile, 0);
    }
    else {
        int slot = cs.getEmptyFileMetadataSlot();
        cs.saveFileMetadata(newFile, slot);
    }
    ++dir.numOfFiles;
    cs.saveDirectory(dir);

    cs.close();
    file.close();
}

void FileSystem::remove(char *container, char *fileName) {
    checkFileName(fileName);
    ContainerStream cs = openContainer(container);

    int fileIndex = cs.findFileByName(fileName);
    if(fileIndex < 0) throw runtime_error(string(fileName) + " not found");

    File removedFile = cs.getFileMetadata(fileIndex);
    removedFile.isActive = false;
    int block = removedFile.firstDataBlock;
    int temp, zero = 0;
    do {
        cs.seekBlocksTableElement(block);
        cs.read((char*)&temp, sizeof(int));
        cs.seekBlocksTableElement(block);
        cs.write((char*)&zero, sizeof(int));
        block = temp;
    } while (block != LAST_BLOCK_OF_FILE);
    cs.saveFileMetadata(removedFile, fileIndex);

    //Update Directory block
    Directory dir = cs.getDirectory();
    --dir.numOfFiles;
    cs.saveDirectory(dir);

    cs.close();
}

void FileSystem::get(char *container, char *fileName) {
    checkFileName(fileName);
    ContainerStream cs = openContainer(container);

    string fileNameStr(fileName);
    if(ifstream(fileName))
        throw runtime_error(fileNameStr + " already exists in current directory");
    fstream file;
    file.open(fileName, fstream::out | fstream::binary | fstream::trunc);
    if(!file) throw runtime_error("Output file could not be created");

    int fileIndex = cs.findFileByName(fileName);
    if(fileIndex < 0) throw runtime_error(fileNameStr + " not found");

    File requestedFile = cs.getFileMetadata(fileIndex);
    int block = requestedFile.firstDataBlock;
    long long int fileSize = cs.getFileSize(block, requestedFile.lastBlockSize);
    int bufferSize = BLOCK_SIZE * 1024;
    int nBytes;
    char buffer[bufferSize];
    file.seekg(0, file.beg);
    do {
        nBytes = (int) (fileSize > bufferSize ? bufferSize : fileSize);
        cs.seekBlock(block);
        cs.read((char*)&buffer, nBytes * sizeof(char));
        file.write((char*)&buffer, nBytes * sizeof(char));
        cs.seekBlocksTableElement(block);
        cs.read((char*)&block, sizeof(int));
        fileSize -= nBytes;
    } while (block != LAST_BLOCK_OF_FILE);

    cs.close();
    file.close();
}

FileSystem::ContainerStream FileSystem::openContainer(char *container) {
    ContainerStream fs;
    fs.open(container, ContainerStream::in | ContainerStream::out | ContainerStream::binary);
    string c(container);
    if(!fs) throw runtime_error("Cannot open " + c);
    if(getFileSize(fs) != CONTAINER_SIZE*1024)
        throw runtime_error(c + " is not a container file or the file is corrupted");
    return fs;
}

long long int FileSystem::getFileSize(std::fstream &fs) {
    fs.seekg (0, fs.end);
    long long length = fs.tellg();
    fs.seekg (0, fs.beg);
    return length;
}

int FileSystem::getNumberOfBlocks(long long int size) {
    float blocks = (float) size/(BLOCK_SIZE*1024);
    return (int) ceil(blocks);
}

void FileSystem::checkFileName(char *fileName) {
    size_t length = strlen(fileName);
    if (length > 15) throw runtime_error("Filename is too long");
    for (int i = 0; i < length; i++) {
        if (isalnum(fileName[i]) == 0)
            throw runtime_error("Filename can only consist of alphanumeric characters");
    }
}


void FileSystem::ContainerStream::seekBlock(int block) {
    seekg(block*BLOCK_SIZE*1024, beg);
}

void FileSystem::ContainerStream::seekBlocksTableElement(int index) {
    seekg(index*sizeof(int), beg);
}

void FileSystem::ContainerStream::seekFileMetadata(int fileIndex) {
    seekg(BLOCK_SIZE*1024 + sizeof(Directory) + fileIndex * sizeof(File), beg);
}

vector<int> FileSystem::ContainerStream::getEmptyBlocks(int nBlocks) {
    seekg(2*sizeof(int), beg);
    vector<int> blocks((unsigned) nBlocks);
    for (int i = 0; i < nBlocks; ++i) {
        do {
            read((char*)&blocks[i], sizeof(int));
            cout << "position: " << tellg()/sizeof(int)-1 << "\tval: " << blocks[i] << endl;
        } while (blocks[i] != 0 || blocks[i] == END_OF_BLOCKS_TABLE);

        if(blocks[i] == END_OF_BLOCKS_TABLE) {
            throw runtime_error("Not enough space available");
        }
        blocks[i] = (int) tellg()/sizeof(int)-1;
    }
    for (int i = 0; i < nBlocks-1; ++i) {
        seekBlocksTableElement(blocks[i]);
        write((char*)&blocks[i+1], sizeof(int));
    }
    seekBlocksTableElement(blocks[nBlocks - 1]);
    int lastBlock = LAST_BLOCK_OF_FILE;
    write((char*)&lastBlock, sizeof(int));
    return blocks;
}

FileSystem::Directory FileSystem::ContainerStream::getDirectory() {
    Directory dir;
    seekBlock(1);
    read((char*)&dir, sizeof(Directory));
    return dir;
}

void FileSystem::ContainerStream::saveDirectory(FileSystem::Directory dir) {
    seekBlock(1);
    write((char*)&dir, sizeof(Directory));
}

FileSystem::File FileSystem::ContainerStream::getFileMetadata(int index) {
    File temp;
    seekFileMetadata(index);
    read((char*)&temp, sizeof(File));
    return temp;
}

void FileSystem::ContainerStream::saveFileMetadata(FileSystem::File file, int index) {
    seekFileMetadata(index);
    write((char*)&file, sizeof(File));
}

int FileSystem::ContainerStream::getEmptyFileMetadataSlot() {
    int index = 0;
    File temp = getFileMetadata(index);
    //TODO: check if it is not a bug- reading zeroes as File object should result in File.isActive set to 0
    while(temp.isActive) {
        ++index;
        temp = getFileMetadata(index);
    }
    return index;
}

int FileSystem::ContainerStream::findFileByName(char *fileName) {
    File temp;
    float x = (float) (BLOCK_SIZE * 1024 - sizeof(Directory)) / sizeof(File);
    int nOfFilesInDirectory = (int) floorf(x);
    int searchedFileIndex = -1;
    for (int i = 0; i < nOfFilesInDirectory; ++i) {
        temp = getFileMetadata(i);
        if(strcmp(temp.fileName, fileName) == 0 && temp.isActive) {
            searchedFileIndex = i;
            break;
        }
    }
    return searchedFileIndex;
}

long long int FileSystem::ContainerStream::getFileSize(int firstBlock, int lastBlockSize) {
    int nBlocks = 0; //TODO: Check if passing fileIndex to this function would not be a better idea
    int block = firstBlock;
    do {
        ++nBlocks;
        seekBlocksTableElement(block);
        read((char*)&block, sizeof(int));
    } while (block != LAST_BLOCK_OF_FILE);
    return (nBlocks-1)*BLOCK_SIZE*1024 + lastBlockSize;
}
