#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>
#include "compress.h"
#include "decompress.h"


using namespace std;

int main(){ 

    auto start = chrono::high_resolution_clock::now();


    string testFileName = "test-copy.fna";
    string outputFileName = "test-output.fna";
    SequenceInfo seqInfo = SequenceInfo();
    extractSequenceInfo(testFileName, seqInfo);
    
    originalSequenceFromSequenceInfo(outputFileName, seqInfo);

    cout << "The original sequence has been written to a file" << endl;
    
    bool areFilesTheSame = false;
    ifstream file1(testFileName);
    ifstream file2(outputFileName);
    string line1, line2;
    while (getline(file1, line1) && getline(file2, line2)){
        if (line1.compare(line2) != 0){
            areFilesTheSame = false;
            break;
        } else {
            areFilesTheSame = true;
        }
    }

    if (areFilesTheSame){
        cout << "The original sequence has been successfully reconstructed" << endl;
    } else {
        cout << "The original sequence has not been successfully reconstructed" << endl;
    }
    

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double> elapsed = end - start;
    cout << "Time taken to compress: " << elapsed.count() << "s" << endl;
    return 0;

    

}