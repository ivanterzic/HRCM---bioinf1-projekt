#include <iostream>
#include <unistd.h>
#include <sys/time.h>
#include <unistd.h> // for getopt
#include <iostream>

using namespace std;

void show_usage(){
	cout << "Usage: hrcm -m {compress | decompress} -r <ref-file-path> { [-f] <file-path> [-p] [percent] | [-t] <tar-file-path> }\n";
	cout << "  -m is the mode, it can only be {compress | decompress}, choose one of them according to requirement, required\n";
	cout << "  -r is the reference, the {ref-file-path} followed, required\n";
	cout << "  -t is the target, a single to-be-compressed file path {tar-file-path} followed, optional\n";
	cout << "  -f is the alternative option of -t, a set of to-be-compressed file paths included in {filename}, optional\n";
    cout << "  -p is the percentage of the second-level matching, default is 10, means 10% of sequences will be used for the second-level matching, optional when -f, illegal when -t\n";
}

int main(int argc, char *argv[]) {
    int oc; // option character

    bool f_value = false, t_value = false, r_value = false, p_value = false, m_value = false;

    if(argc != 7 || argc != 9){
        cout << "Invalid number of characters!/n";
        show_usage();
        return 0;
    }

    // Loop to process each option
    while ((oc = getopt(argc, argv, "m:r:f:t:p:")) >= 0) {
        switch (oc) {
            case 'm':
                m_value = true;
                break;
            case 'r':
                r_value = true;
                break;
            case 'f':
                f_value = true;
                break;
            case 't':
                t_value = true;
                break;
            case 'p':
                p_value = true;
                break;
            case '?': // Unknown option or missing option argument
                std::cerr << "Unknown option or missing argument!" << std::endl;
                show_usage();
                return 0;
        }
    }
    
    return 0;
}
