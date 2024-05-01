#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

using namespace std;

// Struct to represent N character information
struct NInfo {
    int start;
    int length;
};

// Struct to represent special character information
struct SpecialCharInfo {
    int position;
    char character;
};

// Struct to represent sequence information
struct SequenceInfo {
    string identifier;
    string sequence;
    vector<pair<int,int>> lowercaseInfo;
    vector<NInfo> nInfo;
    vector<SpecialCharInfo> specialCharInfo;
    int lineWidth;
};

//razdvoji sequence info, razdvoji funkciju za to be compressed i reference sequence

// Function to extract the sequence information
SequenceInfo extractSequenceInfo(string filename, SequenceInfo& seqInfo)
{
    ifstream file(filename);
    string line;
    string sequence;
    int lineCount = 0;
    while (getline(file, line)){
        if (lineCount == 0){
            seqInfo.identifier = line;
        } else if (lineCount == 1){
            seqInfo.lineWidth = line.size(); 
            sequence += line;
        }
        else {
            sequence += line;
        }
        ++lineCount;
    }
    file.close();
    cout << "Sequence: " << sequence << endl;
    int lowercaseStart = -1;
    int lowercaseLength = 0;
    int distanceFromLastLowercase = 0;
    int distanceFromLastN = 0;
    int distanceFromLastSpecialChar = 0;
    int nStart = -1;
    int nLength = 0;

    for (int i = 0; i < sequence.size(); ++i){
        if (islower(sequence[i])){
            if (lowercaseStart == -1){
                if (seqInfo.lowercaseInfo.size() > 0){
                    lowercaseStart = distanceFromLastLowercase;
                } else {    
                    lowercaseStart = i;
                }
            }
            sequence[i] = toupper(sequence[i]);
            ++lowercaseLength;
        } else {
            if (lowercaseStart != -1){
                seqInfo.lowercaseInfo.push_back(make_pair(lowercaseStart, lowercaseLength));
                lowercaseStart = -1;
                lowercaseLength = 0;
                distanceFromLastLowercase = 0;
            }
            distanceFromLastLowercase++;
        }
        if (sequence[i] == 'N'){
            if (nStart == -1){
                if (seqInfo.nInfo.size() > 0){
                    nStart = distanceFromLastN;
                } else {
                    nStart = i;
                }
            }
            ++nLength;
        } else {
            if (nStart != -1){
                seqInfo.nInfo.push_back({nStart, nLength});
                // delete the N characters
                sequence.erase(sequence.begin() + i - nLength, sequence.begin() + i);
                i -= nLength;
                nStart = -1;
                nLength = 0;
                distanceFromLastN = 0;

            }
            distanceFromLastN++;
        }
        if (sequence[i] == 'X' || sequence[i] == 'x' || sequence[i] == '-'){
            seqInfo.specialCharInfo.push_back({distanceFromLastSpecialChar, sequence[i]});
            distanceFromLastSpecialChar = 0;
            //delete the special character
            sequence.erase(sequence.begin() + i);
            --i;
        } else {
            distanceFromLastSpecialChar++;
        }
    }
    if (lowercaseStart != -1){
        seqInfo.lowercaseInfo.push_back(make_pair(lowercaseStart, lowercaseLength));
    }
    if (nStart != -1){
        seqInfo.nInfo.push_back({nStart, nLength});
    }

    cout << "Uppercase Sequence: " << sequence << endl;
    
    cout << "Lowercase Info: ";
    for (auto& info : seqInfo.lowercaseInfo){
        cout << "(" << info.first << "," << info.second << ") ";
    }
    cout << endl;
    cout << "N Info: ";
    for (auto& info : seqInfo.nInfo){
        cout << "(" << info.start << "," << info.length << ") ";
    }
    cout << endl;
    cout << "Special Char Info: ";
    for (auto& info : seqInfo.specialCharInfo){
        cout << "(" << info.position << "," << info.character << ") ";
    }
    cout << endl;
    seqInfo.sequence = sequence;    
    return seqInfo;
}

void originalSequenceFromSequenceInfo(SequenceInfo seqInfo){
    int nextNIndex, nextSpecialIndex;
    int lastNIndex = 0, lastSpecialIndex = 0;
    seqInfo.nInfo.size() > 0 ? nextNIndex = seqInfo.nInfo[0].start : nextNIndex = INT8_MAX;
    seqInfo.specialCharInfo.size() > 0 ? nextSpecialIndex = seqInfo.specialCharInfo[0].position : nextSpecialIndex = INT8_MAX;
    while (nextNIndex != INT8_MAX || nextSpecialIndex != INT8_MAX){
        if (nextSpecialIndex < nextNIndex){
            lastSpecialIndex += seqInfo.specialCharInfo[0].position;
            seqInfo.sequence.insert(nextSpecialIndex, 1, seqInfo.specialCharInfo[0].character);
            seqInfo.specialCharInfo.erase(seqInfo.specialCharInfo.begin());
            seqInfo.specialCharInfo.size() > 0 ? nextSpecialIndex = seqInfo.specialCharInfo[0].position + lastSpecialIndex : nextSpecialIndex = INT8_MAX;
        } else {
            lastNIndex += seqInfo.nInfo[0].start + seqInfo.nInfo[0].length;
            seqInfo.sequence.insert(nextNIndex, seqInfo.nInfo[0].length, 'N');
            seqInfo.nInfo.erase(seqInfo.nInfo.begin());
            seqInfo.nInfo.size() > 0 ? nextNIndex = seqInfo.nInfo[0].start + lastNIndex : nextNIndex = INT8_MAX;
            
        }
    }
    #include <cctype> // Include the <cctype> header for the correct overload of tolower function

    int nextLowercaseIndex = 0;
    for (auto& info : seqInfo.lowercaseInfo) {
        nextLowercaseIndex += info.first;
        std::string substring = seqInfo.sequence.substr(nextLowercaseIndex, info.second);
        for (char& c : substring) {
            c = std::tolower(c); // Use std::tolower instead of tolower
        }
        seqInfo.sequence.replace(nextLowercaseIndex, info.second, substring);
        nextLowercaseIndex += info.second;
    }
    cout << "Original Sequence: " << seqInfo.sequence << endl;
    // print the original sequence to a file which is named as the identifier and the reference to the file with respect to the original sequence in lines
    ofstream file(seqInfo.identifier + "_output.fa");
    file << seqInfo.identifier << endl;
    for (int i = 0; i < seqInfo.sequence.size(); i += seqInfo.lineWidth){
        file << seqInfo.sequence.substr(i, seqInfo.lineWidth) << endl;
    }
    file.close();
    
}

int main(){
    SequenceInfo seqInfo;
    seqInfo = SequenceInfo();
    seqInfo = extractSequenceInfo("Y_chr1.fa", seqInfo);
    string originalSeq = seqInfo.sequence;
    originalSequenceFromSequenceInfo(seqInfo);
    return 0;
}
    