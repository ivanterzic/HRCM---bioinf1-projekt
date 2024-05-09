#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <math.h>
#include <string>
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
const int kMerLength = 2; 
// the length of the hash table, H in the paper, 2^(2*kMerLength) because there are 4 possible values for each character so 4^kMerLength possible k-mers since 2 bits are enough to represent 4 values
const int hashTableLen =  pow(2, 2 * kMerLength);  
// the hash tables for the reference sequence, H and L, H is the hash table, L is the list
extern vector<int> H;
extern vector<int> L;



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
inline void createKMerHashTable(string& refSeqB){
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
		value += charToNum(refSeqB[kMerLength - 1 - j]);
	}
    cout << "Value: " << value << " of the first k-mer: " << refSeqB.substr(0, kMerLength) << endl;
    // as the paper states: "L[i] = H[valuei], H[valuei] = i.", "If the hash value of the i-th k-mer is expressed as valuei, at the stage of hash table creating, i will be stored in the valuei-th element of the array H, and the original value of the valuei-th element in the array H will be stored into the array L."
	L[0] = H[value];
	H[value] = 0;
    // now, the sliding window slides forward one base character until the window cannot slide anymore, so the hash table builds all k-mer indexes of the reference B sequence, since we're going to be shifting, it is necessary to determine our shift bit number and the end of the shift
    // the shiftSize is the number of bits we need to shift to the right to remove the last character from the k-mer
	int shiftSize = (kMerLength * 2 - 2);
    // the endGoal is the last possible position for the sliding window, so the last k-mer
    int endGoal = refSeqB.size() - kMerLength + 1;
    for (int i = 1; i < endGoal; i++) {
        // shifting the value by 2 bits to the right, since we're removing the last character from the k-mer, that is the equivalent of dividing by 4
        value /= 4;
        // adding the value of the next character, in this case, the character at the position i + kMerLength - 1, since we're going to be adding the character at the end of the k-mer, we need to shift the value by the shiftSize to the left
        // the i + kMerLength - 1 index is used because the k-mer is always kMerLength long, so the last character is at the position i + kMerLength - 1
        value += (charToNum(refSeqB[i + kMerLength - 1])) << shiftSize;
        // again, as stated in the paper, "L[i] = H[valuei], H[valuei] = i."
        L[i] = H[value];
        H[value] = i;
        cout << "Value: " << value << " of the k-mer: " << refSeqB.substr(i, kMerLength) << endl;
    }
}

// First level matching function - this function performs the longest matching process, the pseudocode is taken from the paper and will be followed in this implementation
// Ivan Terzic
inline void firstLevelMatching(string &rSeq, string &tSeq, vector<MatchedInfo> &matchedInfo){
    
    int length_max = kMerLength, position_max = 0;
    createKMerHashTable(rSeq);
    /*cout << "Hash table" << endl;
    for (int i = 0; i < hashTableLen; i++){
        cout << "H[" << i << "] = " << H[i] << endl;
    }
    cout << "List" << endl;
    for (int i = 0; i < L.size(); i++){
        cout << "L[" << i << "] = " << L[i] << endl;
    }*/
    int i, k, position, previos_position = 0, length;
    const int minimumReplaceLength = 0;
    string mismatched = "";
    for (i = 0; i < tSeq.size() - kMerLength + 1; i++){
        cout << "----------------------------------" << endl;
        int value = 0;
        for (int j = 0; j < kMerLength; j++){
            value /= 4;
            value += charToNum(tSeq[i + j]) << (kMerLength * 2 - 2);
        }
        cout << "Value: " << value << " of the k-mer: " << tSeq.substr(i, kMerLength) << endl;
        position = H[value];
        if (position > -1) {
            cout << "Matched string: " << tSeq.substr(i, kMerLength) << endl;
            cout << "Position: " << position << endl;
            cout << "Going further..." << endl;

            position_max = -1, length_max = -1;

            while (position != -1){
                length = kMerLength; int p = position;
                cout << "Chars compared : refSeq[" << position + length << "] = " << rSeq[position] << " tSeq[" << i + length << "] = " << tSeq[i + length] << endl;
                cout << "Length: " << length << endl;
                while (i + length < tSeq.size() && 
                        position + length < rSeq.size() && 
                        tSeq[i + length] == rSeq[p + length]){
                    cout << "Matching char: " << tSeq[i + length] << " " << rSeq[p + length] << endl;
                    ++length;
                }
                if (length_max < length && length >= minimumReplaceLength){
                    length_max = length;
                    position_max = p;
                }
                position = L[position];
                cout << "Changed position of the reference sequence: " << position << endl;
            }

            if (length_max != -1){
                MatchedInfo matchedSegment;
                matchedSegment.position = position_max - previos_position;
                matchedSegment.length = length_max - minimumReplaceLength;
                matchedSegment.mismatched = mismatched;
                matchedInfo.push_back(matchedSegment);

                i += length_max;
                previos_position = position_max + length_max;
                mismatched = "";
                if (i < tSeq.size()){
                    mismatched += tSeq[i];
                }
                continue;
            }
        }
        mismatched += tSeq[i];
    }
    if (i < tSeq.size()){
        for (; i < tSeq.size(); i++){
            mismatched += tSeq[i];
        }
        MatchedInfo matchedSegment;
        matchedSegment.position = 0;
        matchedSegment.length = -minimumReplaceLength;
        matchedSegment.mismatched = mismatched;
        matchedInfo.push_back(matchedSegment);
    }
    

    cout << "First level matching finished" << endl;
    for (int i = 0; i < matchedInfo.size(); i++){
        cout << matchedInfo[i].position << " " << matchedInfo[i].length << " " << matchedInfo[i].mismatched << endl;
    }


}
    

inline void compress(){
    
    using namespace std;

    cout << "Started" << endl;

    ReferenceSequenceInfo refSeqInfo = ReferenceSequenceInfo();
    SequenceInfo seqInfo = SequenceInfo();

    extractReferenceSequenceInfo("../test_data/R1.fa", refSeqInfo);
    extractSequenceInfo("../test_data/T1.fa", seqInfo);

    cout << "Extracted reference sequence info: " << refSeqInfo.identifier << " " << refSeqInfo.sequence << endl;
    cout << "Extracted sequence info: " << seqInfo.identifier << " " << seqInfo.sequence << endl;

    L.resize(refSeqInfo.sequence.size() - kMerLength + 1);

    vector<MatchedInfo> matchedInfo;
    firstLevelMatching(refSeqInfo.sequence, seqInfo.sequence, matchedInfo);

    
}
