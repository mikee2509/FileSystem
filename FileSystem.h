#ifndef FILESYSTEM_FILESYSTEM_H
#define FILESYSTEM_FILESYSTEM_H

#include <fstream>
#include <cstring>

class FileSystem {
public:
    const static int BLOCK_SIZE = 4; //in kilobytes
    const static int CONTAINER_SIZE = 128; //in kilobytes
    void create(char *container);
    void insert(char *container, char *fileName);
    class File;

    int getFileSize(std::fstream &fs); //in bytes
};

class FileSystem::File {
public:
    char fileName[15];
    bool active;
    int fileSize;
    int firstDataBlock;
    int nextFile;

    File(char *name, int size) : active(true), fileSize(size),
                                 firstDataBlock(0), nextFile(0)
    {
        strncpy(fileName, name, 15);
    }
};



#endif //FILESYSTEM_FILESYSTEM_H
