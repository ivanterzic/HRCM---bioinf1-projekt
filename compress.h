
#include <string>
#include "compress.cpp"

using namespace std;

vector<int> H(hashTableLen);
vector<int> L;

inline void compress(bool tar, int percent, string refFile, string toBeCom);