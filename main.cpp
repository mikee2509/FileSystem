#include <iostream>
#include <cstring>
#include "FileSystem.h"

using namespace std;

void printUsage(char *);

int main(int argc, char* argv[])
{
    try {
        if (argc < 3 || argc > 4 || argv[1][0] != '-') {
            printUsage(argv[0]);
            return -1;
        }

        FileSystem fs;

        if (strcmp("-c\0", argv[1]) == 0 && argc == 3) {
            fs.createContainer(argv[2]);
            cout << "Container " << argv[2] << " created" << endl;
        }
        else if (strcmp("-i", argv[1]) == 0 && argc == 4) {
            fs.insertFile(argv[2], argv[3]);
            cout << argv[3] << " inserted into container" << endl;
        }
        else if (strcmp("-rm", argv[1]) == 0 && argc == 4) {
            fs.removeFile(argv[2], argv[3]);
            cout << argv[3] << " removed from container" << endl;
        }
        else if (strcmp("-g", argv[1]) == 0 && argc == 4) {
            fs.getFile(argv[2], argv[3]);
            cout << argv[3] << " copied into current directory" << endl;
        }
        else if (strcmp("-d", argv[1]) == 0 && argc == 3) {
            fs.deleteContainer(argv[2]);
            cout << "Container " << argv[2] << " deleted" << endl;
        }
        else if (strcmp("-s", argv[1]) == 0 && argc == 3)
            fs.showBlocksTable(argv[2]);
        else if (strcmp("-ls", argv[1]) == 0) {
            if (argc == 3) fs.listFiles(argv[2], string().c_str());
            else if (argc == 4) fs.listFiles(argv[2], argv[3]);
        }
        else {
            printUsage(argv[0]);
            return -1;
        }
    } catch (regex_error e) {
        printUsage(argv[0]);
        return -1;
    } catch (runtime_error e) {
        cout << e.what() << endl;
        return -1;
    }
    return 0;
}

void printUsage(char* e) {
    cout << "Usage:\n";
    cout << "Create container\n";
    cout << e << " -c container_file\n\n";

    cout << "Insert file to container\n";
    cout << e << " -i container_file inserted_file\n\n";

    cout << "Get file from container\n";
    cout << e << " -g container_file requested_file\n\n";

    cout << "List files in container\n";
    cout << e << " -ls container_file\n";
    cout << e << " -ls container_file prefix+\n";
    cout << e << " -ls container_file +suffix\n\n";

    cout << "Show container usage map\n";
    cout << e << " -s container_file\n\n";

    cout << "Remove file from container\n";
    cout << e << " -rm container_file removed_file\n\n";

    cout << "Delete container\n";
    cout << e << " -d container_file\n\n";
}