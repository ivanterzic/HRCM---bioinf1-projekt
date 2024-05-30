
#include <string>
#include "compress.cpp"

using namespace std;

vector<int> H(hashTableLen);
vector<int> L;

string ref_seq;
vector<string> seq_names(0);

vector<vector<int>> H_sec;
vector<vector<int>> L_sec;
vector<vector<MatchedInfo>> fst_lvl_res;

vector<substringInfo> mismatchedLowercase;
vector<int> matchedLowercase;

inline void compress(int percent);