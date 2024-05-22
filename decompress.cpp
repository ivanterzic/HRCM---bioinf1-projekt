#include <iostream>
#include <fstream>
#include <string>
#include <vector>

using namespace std;

// struct for substring information
struct substringInfo {
    int startFromLastElement;
    int length;
};

// struct for special character information
struct SpecialCharInfo {
    int position;
    char character;
};

// struct for sequence information
struct SequenceInfo {
    string identifier;
    string sequence;
    vector<substringInfo> lowercaseInfo;
    vector<substringInfo> nInfo;
    vector<SpecialCharInfo> specialCharInfo;
    int lineWidth;
};

//Struct for representing information about the matched segment after first-level matching
struct MatchedInfo {
    int position;
    int length;
    string mismatched;
};

// reconstructing original sequence from sequence information
// 3.1 Sequence information extraction for to-be-decompressed sequence
// Ivan Terzic
void originalSequenceFromSequenceInfo(string outputFileName, SequenceInfo& seqInfo){
    // Firstly, the N characters and special characters are inserted into the sequence according to the information extracted from the sequence information.
    // Because the N characters and special characters are inserted into the sequence in the order they appear in the sequence information, the order of the sequence information is important.
    // Also lowercase includes n, so that needs to be done last
    int nextNIndex, nextSpecialIndex;
    // setting up initial indexes
    // if there are any n characters or special characters for reconstruction: set the next index to the first element of the vector
    seqInfo.nInfo.size() > 0 ? nextNIndex = seqInfo.nInfo[0].startFromLastElement : nextNIndex = -1;
    seqInfo.specialCharInfo.size() > 0 ? nextSpecialIndex = seqInfo.specialCharInfo[0].position : nextSpecialIndex = -1;
    // while there are still n characters or special characters to insert
    while (nextNIndex != -1 || nextSpecialIndex != -1){
        // if the index for n characters is smaller than the index for special characters or if there are no more indexes for n characters
        if (nextNIndex < nextSpecialIndex || nextSpecialIndex == -1){
            // insert N character at the index
            seqInfo.sequence.insert(nextNIndex, seqInfo.nInfo[0].length, 'N'); 
            // increase the index for insertion by the length of N
            nextNIndex += seqInfo.nInfo[0].length; 
            // remove the first element from the vector
            seqInfo.nInfo.erase(seqInfo.nInfo.begin()); 
            // if there are more elements in the vector, increase the index for insertion by the length of the last element
            if (seqInfo.nInfo.size() > 0){
                nextNIndex += seqInfo.nInfo[0].startFromLastElement;
            } else { 
                // if there are no more elements, set the index to -1
                nextNIndex = -1;
            }
        // if the index for special characters is smaller than the index for n characters or if there are no more indexes for n characters
        } else if (nextSpecialIndex <= nextNIndex || nextNIndex == -1){
            // insert special character at the index
            seqInfo.sequence.insert(nextSpecialIndex, 1, seqInfo.specialCharInfo[0].character);
            // increase the index for insertion by 1
            nextSpecialIndex += 1;
            // remove the first element from the vector
            seqInfo.specialCharInfo.erase(seqInfo.specialCharInfo.begin()); 
            if (seqInfo.specialCharInfo.size() > 0){
                // if there are more elements in the vector, increase the index for insertion by the position of the last element
                nextSpecialIndex += seqInfo.specialCharInfo[0].position;
            } else {
                // if there are no more elements, set the index to -1
                nextSpecialIndex = -1;
            }
        } 
    }
    // setting up the index for lowercase characters
    int nextLowercaseIndex = 0;
    // for each substring in the vector of lowercase characters
    for (auto& info : seqInfo.lowercaseInfo) {
        // increase the index for insertion by the position of the last element
        nextLowercaseIndex += info.startFromLastElement;
        // extract the substring from the sequence
        std::string substring = seqInfo.sequence.substr(nextLowercaseIndex, info.length);
        // convert the substring to lowercase
        for (char& c : substring) {
            c = std::tolower(c);
        }
        // replace the substring in the sequence with the lowercase substring
        seqInfo.sequence.replace(nextLowercaseIndex, info.length, substring);
        nextLowercaseIndex += info.length;
    }
    
    // Finally, the sequence is written to the output file.
    ofstream outputFile(outputFileName);
        outputFile << seqInfo.identifier << endl;
        for (unsigned int i = 0; i < seqInfo.sequence.size(); i += seqInfo.lineWidth){
            if (i != 0) { outputFile << endl; }
        outputFile << seqInfo.sequence.substr(i, seqInfo.lineWidth);
    }
}

// Function restoring original base string from first level matching
// Ivan Terzic
inline void reverseFirstLevelMatching(string &rSeq, vector<MatchedInfo> &matchedInfo, string &originalSequence){
    //relative indexes are used to insert the mismatched characters from the last match
    int relativeIndex = 0;
    //for each matched segment
    for (auto &info : matchedInfo){
        originalSequence += info.mismatched;
        relativeIndex += info.position;
        originalSequence += rSeq.substr(relativeIndex, info.length);
        relativeIndex += info.length;
    }
}

void decompress(){
        // The sequence information is extracted from the compressed file.
        string compressedFileName;
        /*Matched info for test :
        0 19 
        -2 11 T
        4 9 T*/
        string rseq = "AGCTGGGCCCTTAAGGTTTCCCGGGAAAAAATTTCCCTTTG";
        string tseq = "AGCTGGGCCCTTAAGGTTTTTTCCCGGGAAATTTCCCTTTG";

        vector<MatchedInfo> matchedInfo;
        MatchedInfo m1 = {0, 19, ""};
        MatchedInfo m2 = {-2, 11, "T"};
        MatchedInfo m3 = {4, 9, "T"};
        matchedInfo.push_back(m1);
        matchedInfo.push_back(m2);
        matchedInfo.push_back(m3);
        
        string originalBaseString = "";
        reverseFirstLevelMatching(rseq, matchedInfo, originalBaseString);
        cout << originalBaseString << endl;
        cout << tseq << endl;
        return;
}

