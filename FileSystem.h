#ifndef FILESYSTEM_FILESYSTEM_H
#define FILESYSTEM_FILESYSTEM_H

#include <fstream>
#include <cstring>
#include <bitset>
#include <vector>
#include <math.h>

class FileSystem {
public:
    const static int BLOCK_SIZE = 4; //in kilobytes
    const static int CONTAINER_SIZE = 128; //in kilobytes
    const static int LAST_BLOCK_OF_FILE = -1;
    const static int END_OF_BLOCKS_TABLE = -2;
    void create(char *container);
    void insert(char *container, char *fileName);
    class File;
    class Directory;

    int getFileSize(std::fstream &fs); //in bytes

    void seekBlock(int block, std::fstream &fs); //move file cursor to a given block

    int getNumberOfBlocks(int size); //get number of blocks needed to hold *size* bytes

    //Move file cursor to a file with given *fileIndex* in Directory block
    void seekFileMetadata(int fileIndex, std::fstream &fs);

    //Get index numbers of blocks that can be filled with data
    std::vector<int> getEmptyBlocks(int nBlocks, std::fstream &fs);

    //Move file cursor to *index* position in blocks' table
    void seekBlocksTableElement(int index, std::fstream &fs);

    Directory getDirectory(std::fstream &fs); //get Directory from a file stream

    void saveDirectory(Directory dir, std::fstream &fs); //save Directory to a file

    //Save File metadata object at position *index* in a file
    void saveFile(File file, int index, std::fstream &fs);

    File getFile(int index, std::fstream &fs); //get File metadata object of given *index* from a file stream

    //Get index of an empty position in Dictionary block
    int getEmptyFileMetadataSlot(std::fstream &fs);
};


class FileSystem::Directory {
public:
    const static int LAST_DIRECTORY_BLOCK = -3;
    int nextDirectoryBlock; //number of the next Directory block
    int numOfFiles; //number of files in this Directory block
    Directory(): nextDirectoryBlock(LAST_DIRECTORY_BLOCK), numOfFiles(0) {}
    Directory(int ndBlock, int nFiles): nextDirectoryBlock(ndBlock), numOfFiles(nFiles) {}
};

class FileSystem::File {
public:
    char fileName[15];
    bool isActive; //can be replaced by a new file if false
    int fileSize; //in bytes
    int firstDataBlock; //index in the blocks' table

    File() : isActive(false), fileSize(0), firstDataBlock(0) {}
    File(char *name, int size, int block) : isActive(true), fileSize(size), firstDataBlock(block)
    {
        strncpy(fileName, name, 15);
    }
};


#endif //FILESYSTEM_FILESYSTEM_H
