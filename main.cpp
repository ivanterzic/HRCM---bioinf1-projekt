#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include <cstdlib>
#include "compress.h"
#include "decompress.h"


using namespace std;

int main(int argc, char** argv) {
    // if the first argument is compress, run compress.cpp
    if (string(argv[1]) == "compress")
        compress();
    // if the first argument is decompress, run decompress.cpp
    else if (string(argv[1]) == "decompress")
        decompress();
    // if the first argument is not compress or decompress, print an error message
    else
        cout << "Invalid argument. Please enter 'compress' or 'decompress'." << endl;
}