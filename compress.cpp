#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cctype> // Include the <cctype> header for the correct overload of tolower function

using namespace std;

// Struct for representing information about substrings, used for lowercase and N substrings
struct substringInfo {
    int startFromLastElement;
    int length;
};

// Struct for representing information about special characters, used for X, x and - characters
struct SpecialCharInfo {
    int position;
    char character;
};

// Struct for representing information about the sequence, to-be-compressed sequence in the original paper
struct SequenceInfo {
    string identifier;
    string sequence;
    vector<substringInfo> lowercaseInfo;
    vector<substringInfo> nInfo;
    vector<SpecialCharInfo> specialCharInfo;
    int lineWidth;
};

// Struct for representing information about the reference sequence
struct ReferenceSequenceInfo {
    string identifier;
    string sequence;
    vector<substringInfo> lowercaseInfo;
};

//Struct for representing information about the matched segment after first-level matching
struct MatchedInfo {
    int position;
    int length;
    string mismatched;
};



// Function for extracting information from the sequence file
// 3.1 Sequence information extraction for the to-be-compressed sequence
// Ivan Terzic
inline void extractSequenceInfo(string filename, SequenceInfo& seqInfo){
    // file opening and reading
    ifstream file(filename);
    string line, sequence = "";
    int lineCount = 0;
    while (getline(file, line)){
        if (lineCount == 0){
            seqInfo.identifier = line;
        } else {
            if (lineCount == 1){
                seqInfo.lineWidth = line.size();
            }
            sequence += line;
        }
        ++lineCount;
    }
    file.close();

	// initialization of variables for storing information about lowercase, N and special characters
    int lowercaseStart = -1;
    int lowercaseLength = 0;
    int distanceFromLastLowercase = 0;
    int distanceFromLastN = 0;
    int distanceFromLastSpecialChar = 0;
    int nStart = -1;
    int nLength = 0;

    // extraction of information about lowercase, N and special characters
    // all is done in one pass through the sequence so that the time complexity is lower
    // start and length of the substring are stored in the corresponding vectors, and the character is replaced in the sequence if it is lowercase
    for (unsigned int i = 0; i < sequence.size(); ++i){
        // if the character is lowercase, it is converted to uppercase and the length of the lowercase substring is increased
        if (islower(sequence[i])){
            if (lowercaseStart == -1){
                // if there is already a record, it is taken into account, otherwise the current index is taken
                if (seqInfo.lowercaseInfo.size() > 0){
                    lowercaseStart = distanceFromLastLowercase;
                } else {    
                    lowercaseStart = i;
                }
            }
            // convert to uppercase and increase the length
            sequence[i] = toupper(sequence[i]);
            ++lowercaseLength;
        } else {
            // if it is an uppercase letter, then if there is a current lowercase substring, it is added to the vector and the counters are restarted
            if (lowercaseStart != -1){
                seqInfo.lowercaseInfo.push_back({lowercaseStart, lowercaseLength});
                lowercaseStart = -1;
                lowercaseLength = 0;
                distanceFromLastLowercase = 0;
            }
            // if there is no current substring, the counter is increased
            distanceFromLastLowercase++;
        }
        // the same process is applied for N characters
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
                nStart = -1;
                nLength = 0;
                distanceFromLastN = 0;

            }
            distanceFromLastN++;
        }
        // the same process is applied for special characters
		// there is no length for special characters, only the position and the character itself because they are seen as separate characters
        if (sequence[i] == 'X' || sequence[i] == 'x' || sequence[i] == '-'){
            seqInfo.specialCharInfo.push_back({distanceFromLastSpecialChar, sequence[i]});
            distanceFromLastSpecialChar = 0;
        } else {
            distanceFromLastSpecialChar++;
        }
    }
    // adding the remaining substrings if there are any still open
    if (lowercaseStart != -1){
        seqInfo.lowercaseInfo.push_back({lowercaseStart, lowercaseLength});
    }
    if (nStart != -1){
        seqInfo.nInfo.push_back({nStart, nLength});
    }
    
    seqInfo.sequence = sequence; 

    // removing all N characters from the sequence and special characters
    for (unsigned int i = 0; i < seqInfo.sequence.size(); ++i){
        if (seqInfo.sequence[i] == 'N' || seqInfo.sequence[i] == 'X' || seqInfo.sequence[i] == 'x' || seqInfo.sequence[i] == '-'){
            seqInfo.sequence.erase(i, 1);
            --i;
        }
    }
}

// Function for extracting information from the reference sequence file
// 3.1 Sequence information extraction for the reference sequence
// Ivan Terzic
inline void extractReferenceSequenceInfo(string filename, ReferenceSequenceInfo& refSeqInfo){

    // file opening and reading
    ifstream file(filename);
    string line, sequence = "";
    int lineCount = 0;
    while (getline(file, line)){
        if (lineCount == 0){
            refSeqInfo.identifier = line;
        } else {
            sequence += line;
        }
        ++lineCount;
    }
    file.close();

    // removing all characters that are not A, C, G or T in uppercase or lowercase
    for (unsigned int i = 0; i < sequence.size(); ++i){
        if (!isalpha(sequence[i]) || 
            (sequence[i] != 'A' && sequence[i] != 'C' && sequence[i] != 'G' && sequence[i] != 'T' && 
            sequence[i] != 'a' && sequence[i] != 'c' && sequence[i] != 'g' && sequence[i] != 't')){
            sequence.erase(i, 1);
            --i;
        }
    }

    int lowercaseStart = -1;
    int lowercaseLength = 0;
    int distanceFromLastLowercase = 0;

    // extraction of information about lowercase characters is the same as for the to-be-compressed sequence
    for (unsigned int i = 0; i < sequence.size(); ++i){
        if (islower(sequence[i])){
            if (lowercaseStart == -1){
                if (refSeqInfo.lowercaseInfo.size() > 0){
                    lowercaseStart = distanceFromLastLowercase;
                } else {    
                    lowercaseStart = i;
                }
            }
            sequence[i] = toupper(sequence[i]);
            ++lowercaseLength;
        } else {
            if (lowercaseStart != -1){
                refSeqInfo.lowercaseInfo.push_back({lowercaseStart, lowercaseLength});
                lowercaseStart = -1;
                lowercaseLength = 0;
                distanceFromLastLowercase = 0;
            }
            distanceFromLastLowercase++;
        }
    }
    if (lowercaseStart != -1){
        refSeqInfo.lowercaseInfo.push_back({lowercaseStart, lowercaseLength});
    }
    refSeqInfo.sequence = sequence; 
}

/*3.2. Sequence Information Matching
At this stage, sequence information matching consists of two parts: one is B sequence matching, and the other is lowercase character information matching. B sequence matching is based on the reference B sequence to find matching segments in the to-be-compressed B sequence. If more than one sequences are to be compressed, HRCM employs the second-level matching automatically among the to-be-compressed sequences. The matching strategy of HRCM is selecting the longest match based on separate chaining.*/

/*
3.2.1. First-Level Matching

In the first step, we create the hash table for reference B sequence. This manuscript creates the hash table based on k-mer, that is, k-mer hashing. It is just like a sliding window with slides of length k over the reference sequence and a window containing a k-mer. We digitally encode the A, C, G, T to 0, 1, 2, 3, respectively. In this way, the k-mer is converted into an integer sequence. Then we calculate the hash value of the k-mer. The calculation method assures that different k-mer hash values represent different k-mers. HRCM uses array H and array L to store the positions of all k-mers. All initial values of H are −1. If the hash value of the i-th k-mer is expressed as , at the stage of hash table creating, i will be stored in the -th element of the array H, and the original value of the -th element in the array H will be stored into the array L. The equation can be expressed as , . Thus, when an identical k-mer appears in the reference sequence, it will be stored in the array H if it is the last one, and otherwise, it will be stored in the array L. After calculating a k-mer hashing, the sliding window slides forward one base character until the window cannot slide anymore. Thus, the hash table builds all k-mer indexes of the reference B sequence.

 is calculated by the same formula for the to-be-compressed B sequence, and then the array H is checked to see if the value exists or not. If it does not exist, it means that the k-mer does not exist in the reference sequence, and the base is recorded as a mismatched character; otherwise, it indicates that the k-mer exists, and then searches will traverse all identical k-mers based on the chain constituted by the array H and the array L. The longest matching segment will be found, the length of the segment is taken as the length value, and the position of the segment in the reference B sequence is used as the position value. The matched segment is represented as a (position, length) tuple and the to-be-compressed segment is replaced. 
*/

/*3.2.1 First-Level Matching
*/




                
        

    

