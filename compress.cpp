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

// the length of k-mer, 14 is the default value in the original code, here the value has been changed for testing many times
const int kMerLength = 2; 
// the length of the hash table, H in the paper, 2^(2*kMerLength) because there are 4 possible values for each character so 4^kMerLength possible k-mers since 2 bits are enough to represent 4 values
const int hashTableLen =  pow(2, 2 * kMerLength);  
// the hash tables for the reference sequence, H and L, H is the hash table, L is the list
extern vector<int> H;
extern vector<int> L;
extern vector<string> seq_names;
extern string ref_seq;

// Function for extracting information from the sequence file
// 3.1 Sequence information extraction for the to-be-compressed sequence
// Ivan Terzic
inline void extractSequenceInfo(string filename, SequenceInfo& seqInfo){
    // file opening and reading
    ifstream file(filename);
    string line, sequence = "";
    int lineCount = 0;
    while (getline(file, line)){
        // skip the comment lines
        if (line[0] == ','){
            continue;
        }
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
inline void extractReferenceSequenceInfo(string filename, ReferenceSequenceInfo& seqInfo){
    
    // file opening and reading
    ifstream file(filename);
    string line, sequence = "";
    int lineCount = 0;
    while (getline(file, line)){
        // skip the comment lines
        if (line[0] == ','){
            continue;
        }
        if (lineCount == 0){
            seqInfo.identifier = line;
        } else {
            if (lineCount == 1){
                //seqInfo.lineWidth = line.size();
            }
            sequence += line;
        }
        ++lineCount;
    }
    file.close();

	// initialization of variables for storing information about lowercase
    int lowercaseStart = -1;
    int lowercaseLength = 0;
    int distanceFromLastLowercase = 0;

    // Lowercase character information extraction from the reference sequence, the logic is the same as in the previous function
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
       
    }

    if (lowercaseStart != -1){
        seqInfo.lowercaseInfo.push_back({lowercaseStart, lowercaseLength});
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
    /*cout << "Value: " << value << " of the first k-mer: " << refSeqB.substr(0, kMerLength) << endl;*/
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
        /*cout << "Value: " << value << " of the k-mer: " << refSeqB.substr(i, kMerLength) << endl;*/
    }
}

// First level matching function - this function performs the longest matching process, the pseudocode is taken from the paper and will be followed in this implementation
// Ivan Terzic
inline void firstLevelMatching(string &rSeq, string &tSeq, vector<MatchedInfo> &matchedInfo){
    cout << "rseq: " << rSeq << endl;
    cout << "tseq: " << tSeq << endl;
    int tSeqLength = tSeq.size(), rSeqLength = rSeq.size();
    // initialite l_max = k, pos_max = 0
    int length_max = kMerLength, position_max = 0;
    // loop variables and l and pos variables
    int i, k, position, length;
    int previos_position = 0;
    //const int minimumReplaceLength = 0;
    // initalization of the mismatchedInfo string
    string mismatchedInfo = "";
    // for i = 0 to n_t - k
    for (i = 0; i < tSeqLength - kMerLength + 1; i++){
        // Calculate the hash value value_t_T  of the k-mer which starts from i in t_seq
        int kMerHashValue = 0;
        for (int j = 0; j < kMerLength; j++){
            // the logic is the same as in hash table creation, but we're not storing the values in the hash table, we're just calculating the hash value
            kMerHashValue /= 4;
            kMerHashValue += charToNum(tSeq[i + j]) << (kMerLength * 2 - 2);
        }
        // pos = H[value_t_i]
        position = H[kMerHashValue];
        // if pos = -1 then
        if (position == -1){
            // t_seq[i] is a mismatched character, recorded to the mismatched information;
            mismatchedInfo += tSeq[i];
            continue;
        }
        else {
            // variables used to determine the parameters of the longest matching segment
            position_max = -1, length_max = -1;
            // while pos != -1 do
            while (position != -1){
                // set l = k, p = pos
                length = kMerLength; int p = position;
                // while t_seq[i + l] = r_seq[p + l] do
                // the resuts are the same with or without the length check, should we keep it or not?
                while (tSeq[i + length] == rSeq[p + length] && i + length < tSeqLength && position + length < rSeqLength){
                    // l = l + 1
                    ++length;
                }
                // if l_max < l then
                if (length_max < length /*&& length >= minimumReplaceLength*/){
                    // l_max = l, pos_max = p
                    length_max = length;
                    position_max = p;
                }
                // update p = L[p]
                position = L[position];
            }
            // record the mismatched string to the mismatched information
            // record the matched string to the matched information
            if (length_max != -1){
                MatchedInfo matchedSegment = {
                    position = position_max - previos_position,
                    length = length_max /*- minimumReplaceLength*/,
                };
                matchedSegment.mismatched = mismatchedInfo;                
                matchedInfo.push_back(matchedSegment);

                i += length_max;
                previos_position = position_max + length_max;
                mismatchedInfo = "";
                if (i < tSeqLength){
                    mismatchedInfo += tSeq[i];
                }
                continue;
            }
        }
        
    }
    // record all the remaining mismatched characters to the mismatched information, these are not in any k-mer
    if (i < tSeqLength){
        for (i; i < tSeqLength; i++)
            mismatchedInfo += tSeq[i];
        
        MatchedInfo matchedSegment = {
            position = 0,
            length = 0, //-minimumReplaceLength,
        };
        matchedSegment.mismatched = mismatchedInfo;
        matchedInfo.push_back(matchedSegment);
    }
    cout << "First level matching finished" << endl;
    for (int i = 0; i < matchedInfo.size(); i++){
        cout << matchedInfo[i].position << " " << matchedInfo[i].length << " " << matchedInfo[i].mismatched << endl;
    }
}

inline void save_matched_info(ofstream& of, MatchedInfo& info){
    of << info.position << " " << info.length << " " << info.mismatched << "\n";
}

inline void save_matched_info_vector(ofstream& of, vector<MatchedInfo>& vec){
    for(MatchedInfo info : vec){
        save_matched_info(of, info);
    }
}

/*3.2.3. Lowercase Character Information Matching.*/
// Function for matching lowercase characters, the function is written according to the pseudocode from the paper
// Ivan Terzic
inline void matchLowercaseCharacters(ReferenceSequenceInfo &refSeqInfo, SequenceInfo &seqInfo){

    /*cout << "Lowercase info reference sequence: " << endl;
    for (int i = 0; i < refSeqInfo.lowercaseInfo.size(); i++){
        cout << refSeqInfo.lowercaseInfo[i].startFromLastElement << " " << refSeqInfo.lowercaseInfo[i].length << endl;
    }
    cout << "Lowercase info sequence: " << endl;
    for (int i = 0; i < seqInfo.lowercaseInfo.size(); i++){
        cout << seqInfo.lowercaseInfo[i].startFromLastElement << " " << seqInfo.lowercaseInfo[i].length << endl;
    }*/

    int lengthLowercase_t = seqInfo.lowercaseInfo.size(), lengthLowecase_r = refSeqInfo.lowercaseInfo.size();
    // initialize matched lowercase information
    vector<int> matchedLowercase = vector<int>(lengthLowercase_t);
    // 0 means that the lowercase character object is not matched, the indexes will be shifted by 1
    for (int i = 0; i < lengthLowercase_t; i++){
        matchedLowercase[i] = 0;
    }
    vector<substringInfo> mismatchedLowercase;
    // add the first element to the reference sequence lowercase information, to shift the indexes by 1
    vector<substringInfo> modifiedRefSeqLowercase = refSeqInfo.lowercaseInfo;
    modifiedRefSeqLowercase.insert(modifiedRefSeqLowercase.begin(), {0, 0});

    // start position of matching, index = 0
    int start_position = 1, i = 0, index = 0;
    // for i = 0 to l_t − 1 do
    //cout << "start " << endl;
    for (i = 0; i < lengthLowercase_t; ++i){
        //cout << "i: " << i << endl;
        // for j = start_position to l_r do
        for (int j = start_position; j < lengthLowecase_r; ++j){
            // if t_lowercase[i] = r_lowercase[j] then
            if (seqInfo.lowercaseInfo[i].startFromLastElement == modifiedRefSeqLowercase[j].startFromLastElement && seqInfo.lowercaseInfo[i].length == modifiedRefSeqLowercase[j].length){
                //cout << "       Matched" << endl;
                // matched lowercase[i] = j, update start_position = j + 1
                matchedLowercase[i] = j;
                start_position = j + 1;
                break;
            }
            // end if
        }
        // end for
        // if matched_lowercase[i] = 0 then
        if (matchedLowercase[i] == 0){
            // for j = start_position − 1 to 1 do
            for (int j = start_position - 1; j > 0; --j){
                // if t_lowercase[i] = r_lowercase[j] then
                if (seqInfo.lowercaseInfo[i].startFromLastElement == modifiedRefSeqLowercase[j].startFromLastElement && seqInfo.lowercaseInfo[i].length == modifiedRefSeqLowercase[j].length){
                    //cout << "           Matched" << endl;
                    // matched_lowercase[i] = j;
                    matchedLowercase[i] = j;
                    // update start position = j + 1;
                    start_position = j + 1;
                    break;
                }
                // end if
            }
            // end for
        }
        // end if
        // if matched_lowercase[i] = 0 then
        if (matchedLowercase[i] == 0){
            // mismatched_lowercase[index] = t_lowercase[i];
            mismatchedLowercase.push_back({
                seqInfo.lowercaseInfo[i].startFromLastElement,
                seqInfo.lowercaseInfo[i].length
            });
            // update index = index + 1;
            
        }

    }
    /*cout << "end" << endl;
    cout << "Lowercase matching done, printing mismatched lowercase information" << endl;
    for (int i = 0; i < mismatchedLowercase.size(); i++){
        cout << mismatchedLowercase[i].startFromLastElement << " " << mismatchedLowercase[i].length << endl;
    }
    cout << "Printing matched lowercase information" << endl;
    for (int i = 0; i < matchedLowercase.size(); i++){
        cout << matchedLowercase[i] << endl;
    }
    cout << "--------------------------------" << endl;*/
}
    

inline void compress(){
    
    using namespace std;

    cout << "Started" << endl;

    ReferenceSequenceInfo refSeqInfo = ReferenceSequenceInfo();
    SequenceInfo seqInfo = SequenceInfo();
    vector<MatchedInfo> matchedInfo;

    string refFile = "../test_data/X_chr1.fa";
    string seqFile = "../test_data/Y_chr1.fa";

    //check if files exist, if not print an error message
    //ifstream fileCheck();
    /* for (string file : {refFile, seqFile}){
        ifstream fileCheck(file);
        if (!fileCheck){
            cout << "File " << file << " does not exist." << endl;
            return;
        }
    }  
     */
    extractReferenceSequenceInfo(ref_seq, refSeqInfo);
    L.resize(refSeqInfo.sequence.size() - kMerLength + 1);
    createKMerHashTable(ref_seq);

    ofstream file("_stored_.hrcm");

    if(!file.is_open()){
        cerr << "ERROR: An error has occured ...";
        exit(1);
    }

    for(string seq : seq_names){
        extractSequenceInfo(seq, seqInfo);
        firstLevelMatching(refSeqInfo.sequence, seqInfo.sequence, matchedInfo);
        matchLowercaseCharacters(refSeqInfo, seqInfo);

        save_matched_info_vector(file, matchedInfo);
    }

    file.close();
}
