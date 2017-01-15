#include "FileSystem.h"
using namespace std;

void FileSystem::createContainer(char *container) {
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

void FileSystem::insertFile(char *container, char *fileName) {
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

void FileSystem::removeFile(char *container, char *fileName) {
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

void FileSystem::getFile(char *container, char *fileName) {
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

void FileSystem::deleteContainer(char *container) {
#ifdef __linux__
    remove(container);
#endif // __linux__

#ifdef WIN32
    DeleteFile(container);
#endif // WIN32
}

void FileSystem::listFiles(char *container, const char *filter) {
    ContainerStream cs = openContainer(container);
    Directory dir = cs.getDirectory();
    if(dir.numOfFiles == 0) {
        cout << "Container is empty" << endl;
        return;
    }

    string f(filter);
    regex ex("[+]");
    function<bool(char*, regex)> regex_function = [](char* a, regex b)->bool { return regex_search(a,b); };

    if(f.empty()) {
        ex.assign("[:alnum:]+");
    }
    else if(!regex_search(f, ex)) {
        ex.assign(f);
        regex_function = [](char* a, regex b)->bool { return regex_match(a,b); };
    }
    else if(f.front() == '+') {
        ex.assign(f.substr(1, f.length()-1) + "$");
    }
    else if(f.back() == '+') {
        ex.assign("^" + f.substr(0, f.length()-1));
    }

    stringstream message;
    File temp;
    int maxNumOfFiles = getNumOfFilesPerDirectoryBlock();
    for (int i = 0; i < maxNumOfFiles; ++i) {
        temp = cs.getFileMetadata(i);
        if(temp.isActive && regex_function(temp.fileName, ex)) {
            message << left << setw(MAX_FILENAME_LENGTH+3) << temp.fileName
                    << cs.getFileSize(temp.firstDataBlock, temp.lastBlockSize) << endl;
        }
    }

    if(message.str().empty()) {
        cout << "No files matching given filter found" << endl;
    } else {
        cout << left << setw(MAX_FILENAME_LENGTH+3) << "FILENAME" << "SIZE [bytes]\n";
        cout << message.str();
    }

    cs.close();
}

void FileSystem::showBlocksTable(char *container) {
    ContainerStream cs = openContainer(container);
    cout << "Block size [kB]: " << BLOCK_SIZE << endl;
    cout << "Container size [kB]: " << CONTAINER_SIZE << endl;
    cout << "Types: table - blocks' table\n       dir - directory block\n       data - data block\n\n";
    cout << left << setw(5) << "BLOCK" << "  " << setw(10) << "ADDRESS"
         << setw(8) << "TYPE" << "FILE" << endl;

    unordered_map<int, string> map;
    File temp;
    int maxNumOfFiles = getNumOfFilesPerDirectoryBlock();
    for (int i = 0; i < maxNumOfFiles; ++i) {
        temp = cs.getFileMetadata(i);
        if(temp.isActive) {
            int j = 1;
            int block = temp.firstDataBlock;
            stringstream s;
            do {
                s << "(" << j++ << ") " << temp.fileName;
                map[block] = s.str();
                s.str(string());
                cs.seekBlocksTableElement(block);
                cs.read((char*)&block, sizeof(int));
            } while (block != LAST_BLOCK_OF_FILE);
        }
    }

    cout << left << setw(5) << 1 << "  " << setw(10) << 0 << "table" << endl;
    cout << left << setw(5) << 2 << "  " << setw(10) << BLOCK_SIZE*1024 << "dir" << endl;

    int nBlocks = CONTAINER_SIZE / BLOCK_SIZE;
    int state;
    for (int b = 2; b < nBlocks; ++b) {
        cout << left << setw(5) << b+1 << "  " << setw(10) << b*BLOCK_SIZE*1024 << setw(8);
        cs.seekBlocksTableElement(b);
        cs.read((char*)&state, sizeof(int));
        if(state == 0) {
            cout << "empty" << endl;
        } else {
            cout << "data" << map[b] << endl;
        }
    }
    
    cs.close();
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

long long int FileSystem::getFileSize(fstream &fs) {
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
    if (length > MAX_FILENAME_LENGTH) throw runtime_error("Filename is too long");
    for (int i = 0; i < length; i++) {
        if (isalnum(fileName[i]) == 0)
            throw runtime_error("Filename can only consist of alphanumeric characters");
    }
}

int FileSystem::getNumOfFilesPerDirectoryBlock() {
    float x = (float) (BLOCK_SIZE * 1024 - sizeof(Directory)) / sizeof(File);
    return (int) floorf(x);
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
    int maxNumOfFiles = getNumOfFilesPerDirectoryBlock();
    int searchedFileIndex = -1;
    for (int i = 0; i < maxNumOfFiles; ++i) {
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
