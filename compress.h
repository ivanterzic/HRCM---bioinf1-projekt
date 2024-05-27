
#include <string>
#include "compress.cpp"

using namespace std;

vector<int> H(hashTableLen);
vector<int> L;

string ref_seq;
vector<string> seq_names(1);

inline void compress();