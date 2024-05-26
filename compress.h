
#include <string>
#include <vector>
#include "compress.cpp"

using namespace std;

vector<int> H(hashTableLen);
vector<int> L;

inline void compress(vector<string> seqNames, int percent, string name);