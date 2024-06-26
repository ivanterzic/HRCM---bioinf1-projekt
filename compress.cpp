#include <iostream>
#include <fstream>
#include <sys/time.h>  //Linux
#include <unistd.h>   //linux
#include <string>
#include <vector>
#include <math.h>
#include <string>
#include <cctype> // Include the <cctype> header for the correct overload of tolower function
#include "common.h"

using namespace std;

// the length of k-mer, 14 is the default value in the original code, here the value has been changed for testing many times
const int kMerLength = 14; 
// the length of the hash table, H in the paper, 2^(2*kMerLength) because there are 4 possible values for each character so 4^kMerLength possible k-mers since 2 bits are enough to represent 4 values
const int hashTableLen =  pow(2, 2 * kMerLength);  
static int hash_table_size;
const int vec_size = 1 << 20;

// the hash tables for the reference sequence, H and L, H is the hash table, L is the list
int* H;
vector<int> L;

int sec_ref_seq_num;

vector<string> seq_file_names;
string ref_seq_file_name;
string zip_file_name;

vector<vector<int>> H_sec;
vector<vector<int>> L_sec;
vector<vector<MatchedInfo>> fst_lvl_res;

vector<substringInfo> mismatchedLowercase;
vector<int> matchedLowercase;

vector<int> line_width_vec;
vector<string> identifier_vec;

// Function for extracting information from the sequence file
// 3.1 Sequence information extraction for the to-be-compressed sequence
// Ivan Terzic
inline void extractSequenceInfo(string filename, SequenceInfo& seqInfo, int el){
    // initialization of variables for storing information about lowercase, N and special characters
    int lowercaseStart = -1;
    int lowercaseLength = 0;
    int distanceFromLastLowercase = 0;
    int distanceFromLastN = 0;
    int distanceFromLastSpecialChar = 0;
    int nStart = -1;
    int nLength = 0;

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
            identifier_vec[el] = line;
        } else {
            if (lineCount == 1){
                line_width_vec[el] = line.size();
            }
            sequence += line;
        }
        ++lineCount;
    }
    file.close();
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
    }
}

// First level matching function - this function performs the longest matching process, the pseudocode is taken from the paper and will be followed in this implementation
// Ivan Terzic
inline void firstLevelMatching(string &rSeq, string &tSeq, vector<MatchedInfo> &matchedInfo){
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
                while (i + length < tSeqLength && position + length < rSeqLength && tSeq[i + length] == rSeq[p + length]){
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
}

inline void save_matched_info(ofstream& of, MatchedInfo& info){
    of << "@ ";
    if(info.mismatched.empty()) of << info.position << " " << info.length << " . ";
    else of << info.position << " " << info.length << " " << info.mismatched << " ";
}

inline void save_matched_info_vector(ofstream& of, vector<MatchedInfo>& vec){
    for(MatchedInfo info : vec){
        save_matched_info(of, info);
    }

    of  << "<\n";
}

/*3.2.3. Lowercase Character Information Matching.*/
// Function for matching lowercase characters, the function is written according to the pseudocode from the paper
// Ivan Terzic
inline void matchLowercaseCharacters(ReferenceSequenceInfo &refSeqInfo, SequenceInfo &seqInfo){
    int lengthLowercase_t = seqInfo.lowercaseInfo.size(), lengthLowecase_r = refSeqInfo.lowercaseInfo.size();
    // initialize matched lowercase information
    matchedLowercase = vector<int>(lengthLowercase_t);
    // 0 means that the lowercase character object is not matched, the indexes will be shifted by 1
    for (int i = 0; i < lengthLowercase_t; i++){
        matchedLowercase[i] = 0;
    }

    // add the first element to the reference sequence lowercase information, to shift the indexes by 1
    vector<substringInfo> modifiedRefSeqLowercase = refSeqInfo.lowercaseInfo;
    modifiedRefSeqLowercase.insert(modifiedRefSeqLowercase.begin(), {0, 0});

    // start position of matching, index = 0
    int start_position = 1, i = 0, index = 0;
    // for i = 0 to l_t − 1 do
    for (i = 0; i < lengthLowercase_t; ++i){
        // for j = start_position to l_r do
        for (int j = start_position; j < lengthLowecase_r; ++j){
            // if t_lowercase[i] = r_lowercase[j] then
            if (seqInfo.lowercaseInfo[i].startFromLastElement == modifiedRefSeqLowercase[j].startFromLastElement && seqInfo.lowercaseInfo[i].length == modifiedRefSeqLowercase[j].length){
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
}

inline int get_closest_prime_value(int number){
    int num = number+1;
    bool prime = false;
    int i = 0;

    while(!prime){
        prime = true;

        for(i = 5; i * i <= num; i += 6){
            if (num % i == 0){
                prime = false;
                break;
            }
        }

        if(num % 2 == 0 || num % 3 == 0){
            prime = false;
        }

        num++;
    }

    return num;
}

inline int get_hash_value(MatchedInfo& info){
    int result = 0;
    for(int i = 0; i < info.mismatched.size(); i++){
        result += info.mismatched[i] * 92083;
    }
    result += info.position * 69061 + info.length * 51787;
    return result % hash_table_size;
}

void print_matched_entity(MatchedInfo info){
    cout << info.length << " " << info.mismatched << " " << info.position << " ";
}

inline void create_hash_for_ref(vector<MatchedInfo>& matched_info, int el) {
    vector<int> loc_L(vec_size);
    vector<int> loc_H(hash_table_size, -1);  // Initialize vector with -1

    int hash;
    
    for (int i = 0; i < matched_info.size(); i++) {
        hash = get_hash_value(matched_info[i]);
        loc_L[i] = loc_H[hash];
        loc_H[hash] = i;
    }

    H_sec[el] = loc_H;
    L_sec[el] = loc_L;
}

inline bool equal_info(MatchedInfo fst, MatchedInfo sec){
    if(fst.length == sec.length && fst.mismatched == sec.mismatched && fst.position == sec.position){
        return true;
    }

    return false;
}

inline int get_length(vector<MatchedInfo>& tested, vector<MatchedInfo>& tester, unsigned int tested_index, unsigned int tester_index){
    int length = 0;
    int idx = tested_index, idy = tester_index;
	while (idx < tested.size() && idy < tester.size() && equal_info(tested[idx], tester[idy])){
        idx++;
        idy++;
        length++;
    }

	return length;

}

inline void second_level_matching(ofstream& of, vector<MatchedInfo> matched_info, int el){
    int hashValue;
    int pre_seq_id = 1, pre_pos = 0;
    int max_pos = 0, max_length, length, seq_id, pos;
    int delta_length, delta_seq_id, delta_pos;
    int id;
    unsigned int j;

    for (j = 0; j < matched_info.size(); j++){
        hashValue = get_hash_value(matched_info[j]);
        max_length = 0;
        for(int k = 0; k < min(el - 1, sec_ref_seq_num); k++){
            id = H_sec[k][hashValue];
            if(id != -1){
                for(pos = id; pos != -1; pos = L_sec[k][pos]){
                    length = get_length(matched_info, fst_lvl_res[k], j, pos);
                    if(length > 1 && max_length < length){
                        max_length = length;
                        max_pos = pos;
                        seq_id = k;
                    }
                }
            }
        }

        if(max_length){
            of << "#" << seq_id << " " << max_pos << " " << max_length << " ";
            j = j + max_length - 1;
        } else {
            save_matched_info(of, matched_info[j]);
        }
    }

    of << "<\n";
}

inline void length_encoding(ofstream& of, vector<int>& vec, int tol){
    vector<int> code;
    if(vec.size() > 0){
        code.push_back(vec[0]);
        int cnt = 1;
        
        for(int i = 1; i < vec.size(); i++){
            if(vec[i] - vec[i-1] == tol){
                cnt++;
            } else {
                code.push_back(cnt);
                code.push_back(vec[i]);
            }
        }

        code.push_back(cnt);
    }
    of << code.size() << " ";
    for(int el : code){
        of << el << " ";
    }
}

inline void save_substring_info(ofstream& of, vector<substringInfo>& info){
    of << info.size() << " ";
    for(substringInfo sub : info){
        of << sub.startFromLastElement << " " << sub.length << " ";
    }
}

inline void save_special_charachter_data(ofstream& of, SequenceInfo &seqInfo){
    int size = seqInfo.specialCharInfo.size();
    of << size << " ";
    if(size == 0){
        return;
    }
    //for(SpecialCharInfo in : seqInfo.specialCharInfo){
    //    cout << in.position << " " << in.character << endl;
    //}
    char c;
    int bitCount = 0, arr_count = 0;
    unsigned int packedData = 0;
    const int bitsPerChar = 2;
    const int charsPerInt = sizeof(unsigned int) * 8 / bitsPerChar;
    const int arr_size = ceil((double)size/(double)charsPerInt);
    unsigned int* arr = new unsigned int[arr_size];
    
    for(int i = 0; i < size; i++){
        of << seqInfo.specialCharInfo[i].position << " ";
        unsigned int encodedChar;
        c = seqInfo.specialCharInfo[i].character;
        switch (seqInfo.specialCharInfo[i].character) {
            case '-':
                encodedChar = 0b00;
                break;
            case 'X':
                encodedChar = 0b01;
                break;
            default:
                encodedChar = 0b10;
                break;
        }
        packedData = (packedData << bitsPerChar) | encodedChar;
        bitCount += bitsPerChar;

        if (bitCount >= charsPerInt * bitsPerChar) {
            bitCount = 0;
            arr[arr_count] = packedData;
            arr_count++;
        }
    }

    if(bitCount > 0){
        arr[arr_count] = packedData;
    }

    for(int i = 0; i < arr_size; i++){
        of << arr[i] << " ";
    }
    delete[] arr;
    /*
    int temp;
    vector<int> flag(3, -1);
    vector<char> arr;

    for(int i = 0; i < seqInfo.specialCharInfo.size(); i++){
        of << seqInfo.specialCharInfo[i].position << " ";

        if(seqInfo.specialCharInfo[i].character == '-') temp = 0;
        else if(seqInfo.specialCharInfo[i].character == 'X') temp = 1;//seqInfo.specialCharInfo[i].character - 'A';
        else temp = 2;

        if(flag[temp] == -1){
            arr.push_back(temp);
            flag[temp] = arr.size() - 1;
        }
    }

    of << arr.size() << " ";

    for(int el : arr){
        of << el << " ";
    }

    if(arr.size() != 1){
        unsigned int bit_num = ceil(log(arr.size()) / log(2));
        unsigned int v_num = floor(32.0 / bit_num);
		for (int i = 0; i < seqInfo.specialCharInfo.size(); )
		{
			unsigned int v = 0;
			for (unsigned int j = 0; j < v_num && i < seqInfo.specialCharInfo.size(); j++, i++)
			{
				v <<= bit_num;

                if(seqInfo.specialCharInfo[i].character == '-') temp = 0;
                else if(seqInfo.specialCharInfo[i].character == 'X') temp = 1;//seqInfo.specialCharInfo[i].character - 'A';
                else temp = 2;

				v += flag[temp];
			}
            of << v << " ";
		}
    }*/
}

inline void save_identifier_data(ofstream& of){
    length_encoding(of, line_width_vec, 0);
    of << "\n";
    for(int i = 0; i < seq_file_names.size(); i++){
        of << identifier_vec[i]<<"\n";
    }
}

inline void save_n_charachter_data(ofstream& of, SequenceInfo& seqInfo){
    save_substring_info(of, seqInfo.nInfo);
}

inline void save_lowercase_charachter_data(ofstream& of, SequenceInfo& seqInfo, ReferenceSequenceInfo& refSeqInfo){
    bool flag = false;
    if(seqInfo.lowercaseInfo.size() > 0 && refSeqInfo.lowercaseInfo.size() > 0){
        matchLowercaseCharacters(refSeqInfo, seqInfo);
        if(2 * mismatchedLowercase.size() < seqInfo.lowercaseInfo.size()){
            flag = true;
            of << flag << " ";
            length_encoding(of, matchedLowercase, 1);
            save_substring_info(of, mismatchedLowercase);
        }
    }

    if (!flag){
        of << flag << " ";
        save_substring_info(of, seqInfo.lowercaseInfo);
    }
}

inline void save_all_other_data(ofstream& of, ReferenceSequenceInfo &refSeqInfo, SequenceInfo &seqInfo){
    save_n_charachter_data(of, seqInfo);
    save_lowercase_charachter_data(of, seqInfo, refSeqInfo);
    save_special_charachter_data(of, seqInfo);
    of << "\n";
}
    
inline void initilize(){
    H = new int[hashTableLen];
    fst_lvl_res = vector<vector<MatchedInfo>>(sec_ref_seq_num);
    H_sec = vector<vector<int>>(sec_ref_seq_num);
    L_sec = vector<vector<int>>(sec_ref_seq_num);
    line_width_vec = vector<int>(seq_file_names.size());
    identifier_vec = vector<string>(seq_file_names.size());
}

inline void clear(){
    delete[] H;
    fst_lvl_res.clear();
    H_sec.clear();
    L_sec.clear();
    line_width_vec.clear();
    identifier_vec.clear();
}

void compress(int percent){
    sec_ref_seq_num = ceil((double)percent * (double)seq_file_names.size() / 100.0);
    initilize();

    cout << "Compression started!" << endl;
    //cout << "Number of reference sequneces for second level matching: "<< sec_ref_seq_num << endl;
    ReferenceSequenceInfo refSeqInfo = ReferenceSequenceInfo();
    SequenceInfo seqInfo = SequenceInfo();

    extractReferenceSequenceInfo(ref_seq_file_name, refSeqInfo);
    L.resize(refSeqInfo.sequence.size() - kMerLength + 1);
    
    createKMerHashTable(refSeqInfo.sequence);
    string hrcm = "_storage_.hrcm";
    string desc = "_identifier_.desc";

    ofstream HRCM(hrcm);

    if(!HRCM.is_open()){
        cerr << "ERROR: An error has occured ...";
        exit(1);
    }

    hash_table_size = get_closest_prime_value(vec_size);

    for(int i = 0; i < seq_file_names.size(); i++){
        
        vector<MatchedInfo> matchedInfo;
        extractSequenceInfo(seq_file_names[i], seqInfo, i);
        firstLevelMatching(refSeqInfo.sequence, seqInfo.sequence, matchedInfo);
    
        save_all_other_data(HRCM, refSeqInfo, seqInfo);

        if(i < sec_ref_seq_num){
            create_hash_for_ref(matchedInfo, i);
            fst_lvl_res[i] = matchedInfo;
        }
        if(i == 0){
            save_matched_info_vector(HRCM, matchedInfo);
        } else {
            second_level_matching(HRCM, matchedInfo, i+1);
        }
    }

    HRCM.close();

    ofstream DESC(desc);
    if(!DESC.is_open()){
        cerr << "ERROR: An error has occured ...";
        exit(1);
    }

    save_identifier_data(DESC);

    DESC.close();
    clear();

    string cmd = "zip " + zip_file_name + ".zip " + hrcm + " " + desc;
    system(cmd.data());

    cmd = "rm -f " + hrcm + " " + desc;
    system(cmd.data());

    cout << "Compression ended!!!\n";
}
