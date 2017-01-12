#include <iostream>
#include "FileSystem.h"
using namespace std;

void FileSystem::create(char *container) {
    if(ifstream(container))
    {
        throw runtime_error("File already exists");
    }
    fstream fs;
    fs.open(container, fstream::out | fstream::binary | fstream::trunc);
    if(!fs) {
        throw runtime_error("File could not be created");
    }
    int nBytes = CONTAINER_SIZE * 1024;
    char disc[nBytes] = {0};
    fs.write((char*)&disc, nBytes * sizeof(char));

    //Write END_OF_BLOCKS_TABLE at the end of blocks' table
    fs.seekg(CONTAINER_SIZE / BLOCK_SIZE * sizeof(int));
    int endOfBlockTable = END_OF_BLOCKS_TABLE;
    fs.write((char*)&endOfBlockTable, sizeof(int));

    Directory main(Directory::LAST_DIRECTORY_BLOCK, 0);
    saveDirectory(main, fs);

    fs.close();
}

void FileSystem::insert(char *container, char *fileName) {
    fstream cfs, file;
    cfs.open(container, fstream::in | fstream::out | fstream::binary);
    if(!cfs) {
        throw runtime_error("Error opening container");
    }
    file.open(fileName, fstream::in | fstream::binary);
    if(!file) {
        throw runtime_error("Error opening file");
    }

    //Copy file to the container
    int fileSize = getFileSize(file);
    int nBlocks = getNumberOfBlocks(fileSize);
    vector<int> fileBlocks = getEmptyBlocks(nBlocks, cfs);
    File newFile(fileName, fileSize, fileBlocks[0]); //TODO: check for fileName length


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

    //Update Directory information
    Directory dir = getDirectory(cfs);
    if(dir.numOfFiles == 0) {
        saveFile(newFile, 0, cfs);
    }
    else {
        int slot = getEmptyFileMetadataSlot(cfs);
        saveFile(newFile, slot, cfs);
    }
    ++dir.numOfFiles;
    saveDirectory(dir, cfs);

    cfs.close();
    file.close();
}

int FileSystem::getFileSize(std::fstream &fs) {
    fs.seekg (0, fs.end);
    int length = fs.tellg();
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
        blocks[i] = fs.tellg()/sizeof(int)-1;
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

void FileSystem::saveFile(FileSystem::File file, int index, fstream &fs) {
    seekFileMetadata(index, fs);
    fs.write((char*)&file, sizeof(File));
}

FileSystem::File FileSystem::getFile(int index, fstream &fs) {
    File temp;
    seekFileMetadata(index, fs);
    fs.read((char*)&temp, sizeof(File));
    return temp;
}

int FileSystem::getEmptyFileMetadataSlot(std::fstream &fs) {
    int index = 0;
    File temp = getFile(index, fs);
    //TODO: check if it is not a bug- reading zeroes as File object should result in File.isActive set to 0
    while(temp.isActive) {
        ++index;
        temp = getFile(index, fs);
    }
    return index;
}

