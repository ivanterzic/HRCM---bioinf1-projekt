#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>
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

// the length of k-mer, 14 is the default value in the paper, so it is used here as well
const int kMerLength = 14; 
// the length of the hash table, H in the paper, 2^(2*kMerLength) because there are 4 possible values for each character so 4^kMerLength possible k-mers since 2 bits are enough to represent 4 values
const int hashTableLen =  pow(2, 2 * kMerLength);  


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

//3.2. Sequence Information Matching
//3.2.1 First-Level Matching*/

// translation of A, C, G, T to 0, 1, 2, 3
// Ivan Terzic
inline int charToNum(char c){
	switch (c) {
        case 'A': return 0;
        case 'C': return 1;
        case 'G': return 2;
        case 'T': return 3;
        default : return 4;
	}
}

// Function for creating the k-mer hash table for the reference sequence
// Ivan Terzic
inline void createKMerHashTable(ReferenceSequenceInfo& refSeqInfo, vector<int>& H, vector<int>& L){
	// the paper states: "All initial values of H are −1."
	for (int i = 0; i < hashTableLen; i++){
        H[i] = -1;
    }
	// calculatiing the value of the first k-mer, there isn't any shifting because it is the first k-mer
    int value = 0;
	for (int j = 0; j < kMerLength; j++) {
        // shifting the value by 2 bits to the left, since a char is 2 bits in this case, 00, 01, 10, 11 for A, C, G, T
		value *= 4;
        // adding the value of the current character, going backwards because the first character is the last one in the k-mer, we want the character at the position kMerLength - 1 to have the most weight in the hash
		value += charToNum(refSeqInfo.sequence[kMerLength - 1 - j]);
	}
    // as the paper states: "L[i] = H[valuei], H[valuei] = i.", "If the hash value of the i-th k-mer is expressed as valuei, at the stage of hash table creating, i will be stored in the valuei-th element of the array H, and the original value of the valuei-th element in the array H will be stored into the array L."
	L[0] = H[value];
	H[value] = 0;
    // now, the sliding window slides forward one base character until the window cannot slide anymore, so the hash table builds all k-mer indexes of the reference B sequence, since we're going to be shifting, it is necessary to determine our shift bit number and the end of the shift
    // the shiftSize is the number of bits we need to shift to the right to remove the last character from the k-mer
	int shiftSize = (kMerLength * 2 - 2);
    // the endGoal is the last possible position for the sliding window, so the last k-mer
    int endGoal = refSeqInfo.sequence.size() - kMerLength + 1;
    for (int i = 1; i < endGoal; i++) {
        // shifting the value by 2 bits to the right, since we're removing the last character from the k-mer, that is the equivalent of dividing by 4
        value /= 4;
        // adding the value of the next character, in this case, the character at the position i + kMerLength - 1, since we're going to be adding the character at the end of the k-mer, we need to shift the value by the shiftSize to the left
        // the i + kMerLength - 1 index is used because the k-mer is always kMerLength long, so the last character is at the position i + kMerLength - 1
        value += (charToNum(refSeqInfo.sequence[i + kMerLength - 1])) << shiftSize;
        // again, as stated in the paper, "L[i] = H[valuei], H[valuei] = i."
        L[i] = H[value];
        H[value] = i;
    }
}


/*
Input: reference B sequence: r seq; k-mer length: k; to-be-compressed B sequence: t seq; length of t seq: nt;
Initialize lmax = k; posmax = 0;
(1) Create hash table for reference B sequence;
(2) for i = 0 to nt − k do
(3) Calculate the hash value valuet
i of the k-mer which starts from i in t seq;
(4) pos = H[valueti]
(5) if pos = − 1 then
(6) t seq[i] is a mismatched character, recorded to the mismatched information;
(7) else
(8) while pos ≠ − 1 do
(9) set l = k, p = pos;
(10) while t seq[i + l] = r seq[p + l] do
(11) l = l + 1;
(12) end while
(13) if lmax < l then
(14) lmax = l, posmax = p;
(15) end if
(16) update pos = L[pos];
(17) end while
(18) end if
(19) record the mismatched string to the mismatched information
(20) record the matched string to the matched information
(21) end for
Output: matched entities (position, length, mismatched)*/
// First level matching function
    

inline void compress(){
    
    using namespace std;

    cout << "Started" << endl;

    ReferenceSequenceInfo refSeqInfo = ReferenceSequenceInfo();
    SequenceInfo seqInfo = SequenceInfo();

    extractReferenceSequenceInfo("../test_data/R1.fa", refSeqInfo);
    extractSequenceInfo("../test_data/T1.fa", seqInfo);

    cout << "Extracted reference sequence info: " << refSeqInfo.identifier << " " << refSeqInfo.sequence << endl;
    cout << "Extracted sequence info: " << seqInfo.identifier << " " << seqInfo.sequence << endl;

    vector<int> refBucket(hashTableLen);
    vector<int> refLoc(refSeqInfo.sequence.size() - kMerLength + 1);
    createKMerHashTable(refSeqInfo, refBucket, refLoc);

    for (int i = 0; i < refBucket.size(); i++){
        if (refBucket[i] != -1)
            cout << "refBucket[" << i << "] = " << refBucket[i] << endl;
    }
}
