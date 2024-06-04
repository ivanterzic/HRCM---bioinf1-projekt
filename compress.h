
#include <string>
#include "compress.cpp" 
using namespace std;

int* H;
vector<int> L;

string to_store_name;
string ref_seq;
vector<string> seq_names(0);

vector<vector<int>> H_sec;
vector<vector<int>> L_sec;
vector<vector<MatchedInfo>> fst_lvl_res;

vector<substringInfo> mismatchedLowercase;
vector<int> matchedLowercase;

vector<int> line_width_vec;
vector<string> identifier_vec;

inline void compress(int percent);