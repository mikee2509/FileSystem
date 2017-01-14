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
    const static int MAX_FILENAME_LENGTH = 15;
    const static int LAST_BLOCK_OF_FILE = -1;
    const static int END_OF_BLOCKS_TABLE = -2;

    class File;
    class Directory;
    class ContainerStream;

    void create(char *container);
    void insert(char *container, char *fileName);
    void remove(char *container, char *fileName);
    void get(char *container, char *fileName);

private:
    ContainerStream openContainer(char *container);

    long long int getFileSize(std::fstream &fs); //in bytes

    int getNumberOfBlocks(long long int size); //Get number of blocks needed to hold *size* bytes

    void checkFileName(char *fileName);
};


class FileSystem::ContainerStream : public std::fstream {
    friend class FileSystem;

    ContainerStream() : std::fstream() {}
    ContainerStream(const ContainerStream &other) : std::fstream() {}

    void seekBlock(int block); //Move file cursor to a given block

    void seekBlocksTableElement(int index); //Move file cursor to *index* position in blocks' table

    void seekFileMetadata(int fileIndex); //Move file cursor to a file with given *fileIndex* in Directory block

    std::vector<int> getEmptyBlocks(int nBlocks); //Get index numbers of blocks that can be filled with data

    Directory getDirectory(); //Get Directory from a file stream

    void saveDirectory(Directory dir); //Save Directory to a file

    File getFileMetadata(int index); //Get File metadata object of given *index* from a file stream

    void saveFileMetadata(File file, int index); //Save File metadata object at position *index* in a file

    int getEmptyFileMetadataSlot(); //Get index of an empty position in Dictionary block

    int findFileByName(char *fileName);

    long long int getFileSize(int firstBlock, int lastBlockSize);
};


class FileSystem::Directory {
    friend class FileSystem;

    const static int LAST_DIRECTORY_BLOCK = -3;
    int nextDirectoryBlock; //number of the next Directory block
    int numOfFiles; //number of files in this Directory block

    Directory(): nextDirectoryBlock(LAST_DIRECTORY_BLOCK), numOfFiles(0) {}
    Directory(int ndBlock, int nFiles): nextDirectoryBlock(ndBlock), numOfFiles(nFiles) {}
};


class FileSystem::File {
    friend class FileSystem;

    char fileName[MAX_FILENAME_LENGTH];
    bool isActive; //can be replaced by a new file if false
    int lastBlockSize; //number of bytes held in the last data block
    int firstDataBlock; //index in the blocks' table

    File() : isActive(false), lastBlockSize(0), firstDataBlock(0) {}
    File(char *name, int size, int block) : isActive(true), lastBlockSize(size), firstDataBlock(block) {
        strncpy(fileName, name, 15);
    }
};


#endif //FILESYSTEM_FILESYSTEM_H
