#ifndef COMMON_H
#define COMMON_H

#include <string>
#include <vector>

using namespace std;

struct substringInfo {
    int startFromLastElement;
    int length;
};

struct SpecialCharInfo {
    int position;
    char character;
};

struct SequenceInfo {
    string sequence;
    string identifier;
    vector<substringInfo> lowercaseInfo;
    vector<substringInfo> nInfo;
    vector<SpecialCharInfo> specialCharInfo;
    int lineWidth;
};

struct ReferenceSequenceInfo {
    string identifier;
    string sequence;
    vector<substringInfo> lowercaseInfo;
};

struct MatchedInfo {
    int position;
    int length;
    string mismatched;
};

extern int sec_ref_seq_num;

extern int* H;
extern vector<int> L;

extern string zip_file_name;
extern string ref_seq_file_name;
extern vector<string> seq_file_names;

extern vector<vector<int>> H_sec;
extern vector<vector<int>> L_sec;
extern vector<vector<MatchedInfo>> fst_lvl_res;

extern vector<substringInfo> mismatchedLowercase;
extern vector<int> matchedLowercase;

extern vector<int> line_width_vec;
extern vector<string> identifier_vec;

void print_matched_entity(MatchedInfo info);
void extractReferenceSequenceInfo(string filename, ReferenceSequenceInfo& seqInfo);
void compress(int percent);

void extract_file_name(string path, string& filename);
void decompress(int percent);

#endif