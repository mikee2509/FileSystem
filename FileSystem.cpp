#include <iostream>
#include <bitset>
#include <vector>
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
    char disc[CONTAINER_SIZE*1000] = {0};
    disc[0] |= 0b10000000; //First block is marked as taken for bitmap
    fs.write((char*)&disc, CONTAINER_SIZE*1000*sizeof(char));
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

    int fileSize = getFileSize(file);
    File newFile(fileName, fileSize);

    int bitmapSize = CONTAINER_SIZE / BLOCK_SIZE / 8; //in bytes
    char bitmap[bitmapSize]; //each char holds information about 8 blocks
    cfs.read(bitmap, bitmapSize*sizeof(char));
    vector<bitset<8>> bits;
    for (int i = 0; i < bitmapSize; ++i) {
        bits.push_back(bitset<8>(bitmap[i]));
    }

    bool isContainerEmpty = true;
    for (int i = 0; i < 7; ++i) {
        if(bits[0][i] == 1) isContainerEmpty = false;
    }
    for (int i = 1; i < bitmapSize; ++i) {
        for (int j = 0; j < 8; ++j) {
            if(bits[i][j] == 1) isContainerEmpty = false;
        }
    }

    if(isContainerEmpty)
    {
        int firstBlockPosition = bitmapSize + 1;
        cfs.write((char*)&firstBlockPosition, sizeof(int));
    }
    //TODO: Finish this function
}

int FileSystem::getFileSize(std::fstream &fs) {
    fs.seekg (0, fs.end);
    int length = fs.tellg();
    fs.seekg (0, fs.beg);
    return length;
}


