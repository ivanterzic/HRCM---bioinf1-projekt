
#include <string>
#include "compress.cpp"

using namespace std;

inline void extractSequenceInfo(string filename, SequenceInfo& seqInfo);
inline void extractReferenceSequenceInfo(string filename, ReferenceSequenceInfo& refSeqInfo);
inline void firstLevelMatching(string &referenceSequence, string &toBeCompressedSequence, vector<MatchedInfo>& matchedInfo, int k);