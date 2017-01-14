#include <iostream>
#include <cstring>
#include "FileSystem.h"

void printUsage();

using namespace std;

int main(int argc, char **argv)
{
    try {
        if (argc < 3 || argc > 4 || argv[1][0] != '-') {
            printUsage();
            return -1;
        }

        FileSystem fs;

        if (strcmp("-c\0", argv[1]) == 0 && argc == 3)
            fs.create(argv[2]);
        else if (strcmp("-i", argv[1]) == 0 && argc == 4)
            fs.insert(argv[2], argv[3]);
        else if (strcmp("-rm", argv[1]) == 0 && argc == 4)
            fs.remove(argv[2], argv[3]);
        else if (strcmp("-g", argv[1]) == 0 && argc == 4)
            fs.get(argv[2], argv[3]);
//    else if (strcmp("-d", argv[1]) == 0 && argc == 3)
//        exitCode = deleteContainer(argv[2]);
//    else if (strcmp("-ls", argv[1]) == 0 && (argc == 3 || argc == 4))
//        exitCode = argc == 3 ? listFiles(argv[2], "") : listFiles(argv[2], argv[3]);
//    else if (strcmp("-s", argv[1]) == 0 && argc == 3)
//        exitCode = showMap(argv[2]);
        else {
            printUsage();
            return -1;
        }
    } catch (runtime_error e) {
        cout << e.what() << endl;
        return -1;
    }
    return 0;
}

void printUsage() {
    cout << "You're doing it wrong" << endl;
}