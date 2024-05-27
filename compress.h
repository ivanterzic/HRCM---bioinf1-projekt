
#include <string>
#include "compress.cpp"

using namespace std;

vector<int> H(hashTableLen);
vector<int> L;

int sec_ref_sec_num;
string ref_seq;
vector<string> seq_names(1);

vector<vector<int>> H_sec;
vector<vector<int>> L_sec;
vector<vector<MatchedInfo>> fst_lvl_res;

inline void compress();