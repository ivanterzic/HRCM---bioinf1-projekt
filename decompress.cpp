#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "common.h"

using namespace std;

// reconstructing original sequence from sequence information
// 3.1 Sequence information extraction for to-be-decompressed sequence
// Ivan Terzic
inline void originalSequenceFromSequenceInfo(string outputFileName, SequenceInfo& seqInfo){
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

inline void reverse_lowercase_match(SequenceInfo& info, ReferenceSequenceInfo& ref_info){
    int loc;
    for(int i = 0, j = 0; i < matchedLowercase.size(); i++){
        if(matchedLowercase[i] == 0){
            info.lowercaseInfo[i].startFromLastElement = mismatchedLowercase[i].startFromLastElement;
            info.lowercaseInfo[i].length = mismatchedLowercase[j].startFromLastElement;
            j++;
        } else {
            info.lowercaseInfo[i].startFromLastElement = ref_info.lowercaseInfo[matchedLowercase[i]].startFromLastElement;
            info.lowercaseInfo[i].length = ref_info.lowercaseInfo[matchedLowercase[i]].length;
        }
    }
}

inline void extract_matched_info(ifstream& is, vector<MatchedInfo>& info){
    char test;
    int pos, len, id;
    string str;

    is >> test;
    while(test != '<'){
        //cout << test << " " << endl;
        if(test == '@'){
            is >> pos >> len >> str;
            if(strcmp(str.data(), ".") == 0){
                str = "";
            }

            info.push_back({
                pos, len, str
            });
        }

        if(test == '#'){
            is >> id >> pos >> len;

            for(int j = 0; j < len; j++){
                info.push_back(fst_lvl_res[id][pos + j]);
            }
        }

        is >> test;
    }
}

inline void read_matched_info(string hrcm_line, vector<MatchedInfo>& matched_info){
    int count = 0;
    bool was_blank_space = false;

    vector<string> code;
    code.reserve(hrcm_line.size());
    string to_push = "";
    int i;

    for(i = 0; i < hrcm_line.size(); i++){
        if(hrcm_line[i] == ' '){
            if(was_blank_space){
                code.push_back("");
                continue;
            }
            code.push_back(to_push);
            to_push = "";
            was_blank_space = true;
            continue;
        }

        to_push+=hrcm_line[i];
    }

    for(i = 0; i < code.size(); i+=3){
        matched_info.push_back({stoi(code[i]), stoi(code[i+1]), code[i+2]});
    }

    code.clear();
}

void extract_file_name(string path, string& filename){
    string name = "";
    bool was_first_dot = false;
    int i;
    for(i = path.size() - 1; i >= 0; i--){
        if(path[i] == '.' && !was_first_dot){
            name = "";
            was_first_dot = true;
            continue;
        }

        if(path[i] == '/'){
            break;
        }

        name += path[i];
    }

    filename = "";
    for(i = name.size() - 1; i >= 0; i--){
        filename += name[i];
    }
}

inline void length_decoding(ifstream& is, vector<int>& vec, int tol){
    int len, el;
    is >> len;
    
    vector<int> code = vector<int>(len);

    for(int i = 0; i < len; i++){
        is >> el;
        code[i] = el;
    }

    int size = 0;
    for(int i = 1; i < code.size(); i+=2){
        size += code[i];
    }
    cout << "Vec size: " << size << endl;
    if(size > 0){
        vec = vector<int>(size);
        len = 0;
        for(int i = 0; i < code.size(); i+=2){
            for(int j = 0; j < code[i+1]; j++){
                vec[len] = code[i]+j*tol;
                len++;
            }
        }
    }

    code.clear();
}

inline void extract_substring_info(ifstream& is, vector<substringInfo>& info){
    int size, el1, el2;
    is >> size;
    cout << "Substring size: " << size << endl;
    info = vector<substringInfo>(size);

    for(int i = 0; i < size; i++){
        is >> el1 >> el2;
        info[i] = {el1, el2};
    }
}

inline void extract_identifier_data(ifstream& is){
    length_decoding(is, line_width_vec, 0);
    string line;
    while(!is.eof()){
        is >> line;
        identifier_vec.push_back(line);
    }
}

inline void extract_special_charachter_data(ifstream& is, SequenceInfo& info){
    int special_character_vector_len, pos;
    is >> special_character_vector_len;

    char c;
    int bitCount = 0, position = 0;
    unsigned int packedData = 0;
    const int bitsPerChar = 2;
    const int charsPerInt = sizeof(unsigned int) * 8 / bitsPerChar;
    const int arr_size = ceil((double)special_character_vector_len/(double)charsPerInt);

    info.specialCharInfo = vector<SpecialCharInfo>(special_character_vector_len);
    if(special_character_vector_len > 0){
        for(int i = 0; i < special_character_vector_len; i++){
            is >> pos;
            info.specialCharInfo[i].position = pos;
        }
        int size = 2*special_character_vector_len;
        for(int i = 0; i < arr_size; i++){
            is >> packedData;
            cout << "Here" << endl;

            for(bitCount = min(charsPerInt * bitsPerChar, size); bitCount > 0; bitCount -= bitsPerChar){
                unsigned int encodedChar = (packedData >> (bitCount - bitsPerChar)) & 0b11;
                char decodedChar;
                switch (encodedChar) {
                case 0b00:
                    decodedChar = '-';
                    break;
                case 0b01:
                    decodedChar = 'X';
                    break;
                default:
                    decodedChar = '*';
                    break;
                }
                if(position >= special_character_vector_len) break;
                info.specialCharInfo[position].character = decodedChar;
                position++;
            }
            size -= charsPerInt * bitsPerChar;
        }

        /*vector<char> arr;
        int size, temp;
        is >> size;

        for(int i = 0; i < size; i++){
            is >> temp;
            
            if(temp == 0) arr.push_back('-');
            else if(temp == 1) arr.push_back('X');
            else  arr.push_back('x');
        }

        if(size != 1){
            int bit_num = ceil(log(size) / log(2));
			int v_num = floor(32.0 / bit_num);
            
            for (int i = 0; i < special_character_vector_len;){
			    unsigned int v = 0;
                is >> v;
                vector<char> temp_arr;
                int temp_i = i;

			    for (int j = 0; j < v_num && temp_i < special_character_vector_len; j++, temp_i++)
			    {
                    int mod = v % (1 << bit_num);
				    v >>= bit_num;
                    temp_arr.push_back(arr[mod]);
			    }
            
                for(int j = temp_arr.size() - 1; j >= 0 && i < special_character_vector_len; j--, i++){
                    info.specialCharInfo[i].character = temp_arr[j];
                }
		    }
        }*/
        
    }
}

inline void extract_lowercase_data(ifstream& is, SequenceInfo& info, ReferenceSequenceInfo& ref_info){ 
    bool flag;
    is >> flag;
    if(!flag){
        extract_substring_info(is, info.lowercaseInfo);
    } else {
        length_decoding(is, matchedLowercase, 1);
        extract_substring_info(is, mismatchedLowercase);
        reverse_lowercase_match(info, ref_info);
    }
}

inline void extract_n_charachter_data(ifstream& is, SequenceInfo& info){
    extract_substring_info(is, info.nInfo);
}

inline void extract_other_data(ifstream& is, SequenceInfo& info, ReferenceSequenceInfo& ref_info){
    extract_n_charachter_data(is, info);
    extract_lowercase_data(is, info, ref_info);
    extract_special_charachter_data(is, info);
}

inline void dec_initilize(){
    fst_lvl_res=vector<vector<MatchedInfo>>(sec_ref_seq_num);
}

inline void dec_clear(){
    fst_lvl_res.clear();
}

inline void print_all(SequenceInfo info){
    cout << "identifier: " << info.identifier << endl;
    cout << "sequence: " << info.sequence << endl;
    cout << "lineWidth: " << info.lineWidth << endl;
    cout << "Lowercase:" << endl; 
    for(substringInfo in : info.lowercaseInfo){
        cout << in.startFromLastElement << " " << in.length << endl;
    }
    cout << "\nspecialCharInfo:" << endl; 
    for(SpecialCharInfo in : info.specialCharInfo){
        cout << in.position << " " << in.character << endl;
    }
    cout << "\nnInfo:" << endl;
    for(substringInfo in : info.nInfo){
        cout << in.startFromLastElement << " " << in.length << endl;
    }
}

void decompress(int percent){
    sec_ref_seq_num = ceil((double)percent * (double)seq_file_names.size() / 100.0);
    dec_initilize();
    cout << "Compression started!" << endl;
    
    string hrcm = "_storage_.hrcm";
    string desc = "_identifier_.desc";
    string cmd;
    cmd = "unzip " + zip_file_name + ".zip";
    system(cmd.data());

    string filename;
    ReferenceSequenceInfo ref_info;
    extractReferenceSequenceInfo(ref_seq_file_name, ref_info);

    ifstream DESC(desc);
    if(!DESC.is_open()){
        cerr << "ERROR: An error has occured ...";
        exit(1);
    }

    extract_identifier_data(DESC);

    DESC.close();

    ifstream HRCM(hrcm);
    if(!HRCM.is_open()){
        cerr << "ERROR: An error has occured ...";
        exit(1);
    }

    string line_from_HRCM;
    for(int i = 0; i < seq_file_names.size(); i++){
        SequenceInfo info;
        vector<MatchedInfo> matchedInfo;
        info.identifier = identifier_vec[i];
        info.lineWidth = line_width_vec[i];

        extract_file_name(seq_file_names[i], filename);
        extract_other_data(HRCM, info, ref_info);
        extract_matched_info(HRCM, matchedInfo);

        if(i < sec_ref_seq_num){
            fst_lvl_res[i] = matchedInfo;
        }        

        reverseFirstLevelMatching(ref_info.sequence, matchedInfo, info.sequence);        
        //cout << info.sequence << endl;

        //print_all(info);

        originalSequenceFromSequenceInfo(filename+".fasta", info);
    }

    HRCM.close();
    dec_clear();
    cmd = "rm -f " + hrcm + " " + desc;
    system(cmd.data());

    cout << "Decompression ended!!!\n";
}

