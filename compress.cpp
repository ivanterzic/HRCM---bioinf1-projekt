#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <cctype> // Include the <cctype> header for the correct overload of tolower function

using namespace std;

// Struct za predstavljanje informacija o substringu, to je za N i lowercase
struct substringInfo {
    int startFromLastElement;
    int length;
};

// Struct za predstavljanje informacija o specijalnim karakterima
struct SpecialCharInfo {
    int position;
    char character;
};

// Struct za predstavljanje informacija o sekvenci, to-be-compressed sekvenci
struct SequenceInfo {
    string identifier;
    string sequence;
    vector<substringInfo> lowercaseInfo;
    vector<substringInfo> nInfo;
    vector<SpecialCharInfo> specialCharInfo;
    int lineWidth;
};

// Struct za predstavljanje informacija o referentnoj sekvenci, reference sequence u originalnom radu
struct ReferenceSequenceInfo {
    string identifier;
    string sequence;
    vector<substringInfo> lowercaseInfo;
};

// funkcija za ekstrakciju informacija iz sekvence, to-be-compressed sekvence
// 3.1 Sequence information extraction za to be compressed sekvencu
inline void extractSequenceInfo(string filename, SequenceInfo& seqInfo){
    // ucitavanje filea
    ifstream file(filename);
    string line, sequence = "";
    int lineCount = 0;
    while (getline(file, line)){
        if (lineCount == 0){
            seqInfo.identifier = line;
        } else {
            if (lineCount == 1){
                seqInfo.lineWidth = line.size();
            }
            sequence += line;
        }
        ++lineCount;
    }
    file.close();


    int lowercaseStart = -1;
    int lowercaseLength = 0;
    int distanceFromLastLowercase = 0;
    int distanceFromLastN = 0;
    int distanceFromLastSpecialChar = 0;
    int nStart = -1;
    int nLength = 0;

    // izvlacenje informacija o N, lowercase i specijalnim karakterima
    // sve radim u istoj petlji, u istom prolaženju kroz sekvencu i onda stedim resurse
    // dakle, pamtim pocetak i duzinu svakog substringa te kad on zavrsi onda ga dodam u vektor i restartam brojace
    for (unsigned int i = 0; i < sequence.size(); ++i){
        // ako je karakter mala slova, onda ga pretvaram u veliko i povecavam duzinu lowercase substringa
        if (islower(sequence[i])){
            if (lowercaseStart == -1){
                //ako vec postoji zapis, gledam od njega, inace od trenutnog indexa
                if (seqInfo.lowercaseInfo.size() > 0){
                    lowercaseStart = distanceFromLastLowercase;
                } else {    
                    lowercaseStart = i;
                }
            }
            // pretvaram u veliko slovo i povecavam duzinu
            sequence[i] = toupper(sequence[i]);
            ++lowercaseLength;
        } else {
            // ako je veliko slovo, onda ako postoji trenutni substirng malih slova, dodajem ga u vektor i restartam brojace
            if (lowercaseStart != -1){
                seqInfo.lowercaseInfo.push_back({lowercaseStart, lowercaseLength});
                lowercaseStart = -1;
                lowercaseLength = 0;
                distanceFromLastLowercase = 0;
            }
            // ako ne postoji trenutan niz, povacavam brojac
            distanceFromLastLowercase++;
        }
        // analogno za N
        if (sequence[i] == 'N'){
            if (nStart == -1){
                if (seqInfo.nInfo.size() > 0){
                    nStart = distanceFromLastN;
                } else {
                    nStart = i;
                }
            }
            ++nLength;
        } else {
            if (nStart != -1){
                seqInfo.nInfo.push_back({nStart, nLength});
                nStart = -1;
                nLength = 0;
                distanceFromLastN = 0;

            }
            distanceFromLastN++;
        }
        // ako je specijalni karakter, dodajem ga u vektor, nemma brojac duljine jer se oni gledaju zasebno
        if (sequence[i] == 'X' || sequence[i] == 'x' || sequence[i] == '-'){
            seqInfo.specialCharInfo.push_back({distanceFromLastSpecialChar, sequence[i]});
            distanceFromLastSpecialChar = 0;
        } else {
            distanceFromLastSpecialChar++;
        }
    }
    // dodadavnje ako je ostao zaostalih nizova
    if (lowercaseStart != -1){
        seqInfo.lowercaseInfo.push_back({lowercaseStart, lowercaseLength});
    }
    if (nStart != -1){
        seqInfo.nInfo.push_back({nStart, nLength});
    }
    
    seqInfo.sequence = sequence; 

    //brisem sve N i specijalne karaktere iz sekvence
    for (unsigned int i = 0; i < seqInfo.sequence.size(); ++i){
        if (seqInfo.sequence[i] == 'N' || seqInfo.sequence[i] == 'X' || seqInfo.sequence[i] == 'x' || seqInfo.sequence[i] == '-'){
            seqInfo.sequence.erase(i, 1);
            --i;
        }
    }
}

// funckija za ekstrakciju informacija iz referentne sekvence
// 3.1 Reference sequence information extraction
inline void extractReferenceSequenceInfo(string filename, ReferenceSequenceInfo& refSeqInfo){
    // ucitavanje filea
    ifstream file(filename);
    string line, sequence = "";
    int lineCount = 0;
    while (getline(file, line)){
        if (lineCount == 0){
            refSeqInfo.identifier = line;
        } else {
            sequence += line;
        }
        ++lineCount;
    }
    file.close();

    // micanje svih slova koje nisu veliko ili malo acgt
    for (unsigned int i = 0; i < sequence.size(); ++i){
        if (!isalpha(sequence[i]) || 
            (sequence[i] != 'A' && sequence[i] != 'C' && sequence[i] != 'G' && sequence[i] != 'T' && 
            sequence[i] != 'a' && sequence[i] != 'c' && sequence[i] != 'g' && sequence[i] != 't')){
            sequence.erase(i, 1);
            --i;
        }
    }

    int lowercaseStart = -1;
    int lowercaseLength = 0;
    int distanceFromLastLowercase = 0;

    // izvlacenje informacija o malim slovima
    for (unsigned int i = 0; i < sequence.size(); ++i){
        // ako je karakter mala slova, onda ga pretvaram u veliko i povecavam duzinu lowercase substringa
        if (islower(sequence[i])){
            if (lowercaseStart == -1){
                //ako vec postoji zapis, gledam od njega, inace od trenutnog indexa
                if (refSeqInfo.lowercaseInfo.size() > 0){
                    lowercaseStart = distanceFromLastLowercase;
                } else {    
                    lowercaseStart = i;
                }
            }
            // pretvaram u veliko slovo i povecavam duzinu
            sequence[i] = toupper(sequence[i]);
            ++lowercaseLength;
        } else {
            // ako je veliko slovo, onda ako postoji trenutni substirng malih slova, dodajem ga u vektor i restartam brojace
            if (lowercaseStart != -1){
                refSeqInfo.lowercaseInfo.push_back({lowercaseStart, lowercaseLength});
                lowercaseStart = -1;
                lowercaseLength = 0;
                distanceFromLastLowercase = 0;
            }
            // ako ne postoji trenutan niz, povacavam brojac
            distanceFromLastLowercase++;
        }
    }
    // dodadavnje ako je ostao zaostalih nizova
    if (lowercaseStart != -1){
        refSeqInfo.lowercaseInfo.push_back({lowercaseStart, lowercaseLength});
    }
    refSeqInfo.sequence = sequence; 

}