#include <iostream>
#include <fstream>
#include <unistd.h>
//#include <windows.h>
#include <sys/time.h>
//#include <sys/timeb.h>
#include <unistd.h> // for getopt
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include "compress.h"

using namespace std;

void readFile(const string& filename){
    ifstream file(filename);
    string line;
    if (!file.is_open()) {
        cerr << "Error: Could not open the file " << filename << endl;
        return;
    }

    while (getline(file, line)){
        if(!line.empty()){
            seq_names.push_back(line);
        }
    }
    file.close();
    cout << "Ended hier!!!\n";
}

void show_usage(){
	std::cout << "Usage: hrcm -m {compress | decompress} -r <ref-file-path> { [-f] <file-path> [-p] [percent] | [-t] <tar-file-path> }\n";
	std::cout << "  -m is the mode, it can only be {compress | decompress}, choose one of them according to requirement, required\n";
	std::cout << "  -r is the reference, the {ref-file-path} followed, required\n";
	std::cout << "  -t is the target, a single to-be-compressed file path <tar-file-path> followed, optional\n";
	std::cout << "  -f is the alternative option of -t, a set of to-be-compressed file paths included in <filename>, optional\n";
    std::cout << "  -p is the percentage of the second-level matching, default is 10, means 10% of sequences will be used for the second-level matching, optional when -f, illegal when -t\n";
}

int main(int argc, char *argv[]) {
    int oc; // option character

    bool f_value = false, t_value = false, r_value = false, p_value = false, m_value = false;
    int percent = 10;
    string mode, ref, toBe;

    struct timeval start, end;
    gettimeofday(&start, nullptr);
    
    if(argc != 7 && argc != 9){
        std::cout << "Invalid number of parameters!\n";
        show_usage();
        return 0;
    }

    // Loop to process each option
    while ((oc = getopt(argc, argv, "m:r:f:t:p:")) >= 0) {
        switch (oc) {
            case 'm':
                m_value = true;
                mode = optarg;
                break;
            case 'r':
                r_value = true;
                ref_seq = optarg;
                break;
            case 'f':
                f_value = true;
                toBe = optarg;
                break;
            case 't':
                t_value = true;
                toBe = optarg;
                break;
            case 'p':
                percent = std::stoi(optarg);
                break;
            case '?': // Unknown option or missing option argument
                std::cerr << "Unknown option or missing argument!" << std::endl;
                show_usage();
                return 0;
        }
    }

    sec_ref_sec_num = ceil(percent * seq_names.size() / 100);

    if (!m_value || !r_value){
        std::cout << "Missing method or referance parameter!\n";
        show_usage();
        return 0;
    }
    if(!(t_value ^ f_value)){
        std::cout << "Invalid number of to-be-compressed parameters!\n";
        show_usage();
        return 0;
    }
    if(t_value){
        seq_names.push_back(toBe);
    } else if (f_value){
        cout << "Was hier\n";
        readFile(toBe);
    }

    if(mode == "compress"){
        compress();

    } else if(mode == "decompress"){

    } else {
        std::cout << "Invalid method used!\n";
        show_usage();
        return 0;
    }

    gettimeofday(&end, nullptr);
    double duration = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_usec - start.tv_usec) /1000.0;
    std::cout << "Duration of program was " << duration << "ms\n";

    return 0;
}
