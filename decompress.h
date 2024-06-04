#include <string>
#include "decompress.cpp"

using namespace std;


string name_of_zip_file;
string dec_ref_seq_name;
vector<string> zipped_files;
vector<int> dec_line_width_vec;
vector<string> dec_identifier_vec;
vector<vector<MatchedInfo>> dec_fst_lvl_matching;


void decompress(int percent);