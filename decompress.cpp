#include <iostream>
#include <fstream>
#include <string>
#include <vector>

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

// Reverzan postupak, rekonstrukcija originalne sekvence iz informacija
// 3.1 Sequence information extraction za to be compressed sekvencu, reverzno
void originalSequenceFromSequenceInfo(string outputFileName, SequenceInfo& seqInfo){

    // prvo cu rekonstruirati indekse za N i specijalne karaktere u originalnoj sekvenci da mogu onda za mala slova
    int nextNIndex, nextSpecialIndex;
    // postavljanje inicijalnih poziicija
    // gledat cu na koji prvi indeks na idem da ne smrdam indekse kasnije
    seqInfo.nInfo.size() > 0 ? nextNIndex = seqInfo.nInfo[0].startFromLastElement : nextNIndex = -1;
    seqInfo.specialCharInfo.size() > 0 ? nextSpecialIndex = seqInfo.specialCharInfo[0].position : nextSpecialIndex = -1;
    // dok god postoji n i speicjalnih znakova za rekonstrukciju
    while (nextNIndex != -1 || nextSpecialIndex != -1){
        // ako je indeks za N manji od indeksa za specijalne karaktere ili ako nema vise indeksa za specijalne karaktere
        if (nextNIndex < nextSpecialIndex || nextSpecialIndex == -1){
            seqInfo.sequence.insert(nextNIndex, seqInfo.nInfo[0].length, 'N'); // ubacujem N na indeksu
            nextNIndex += seqInfo.nInfo[0].length; // povecavam indeks za ubacivanje N za duzinu N
            seqInfo.nInfo.erase(seqInfo.nInfo.begin()); // brisem prvi element iz vektora koji sam upisao
            if (seqInfo.nInfo.size() > 0){ // ako ima jos elemenata u vektoru, povecavam indeks za ubacivanje za duzinu od zadnjeg elementa
                nextNIndex += seqInfo.nInfo[0].startFromLastElement;
            } else { // ako vise nema elemenata, postavljam indeks na -1
                nextNIndex = -1;
            }
        // ako je indeks za specijalne karaktere manji od indeksa za N ili ako nema vise indeksa za N
        } else if (nextSpecialIndex <= nextNIndex || nextNIndex == -1){
            seqInfo.sequence.insert(nextSpecialIndex, 1, seqInfo.specialCharInfo[0].character); // ubacujem specijalni karakter na indeksu
            nextSpecialIndex += 1;
            seqInfo.specialCharInfo.erase(seqInfo.specialCharInfo.begin()); // brisem prvi element iz vektora
            if (seqInfo.specialCharInfo.size() > 0){
                // ako ima jos elemenata u vektoru, povecavam indeks za ubacivanje za duzinu od zadnjeg elementa
                nextSpecialIndex += seqInfo.specialCharInfo[0].position;
            } else {
                // ako vise nema elemenata, postavljam indeks na -1
                nextSpecialIndex = -1;
            }
        } 
    }

    // sad kad su svi indeksi okej vracam lowercase jer i N moze biti lowercase
    int nextLowercaseIndex = 0;
    // prolazim kroz sve informacije o malim slovima
    for (auto& info : seqInfo.lowercaseInfo) {
        // indeks na kojem pocinje mala slova je indeks od zadnjeg elementa + 1
        nextLowercaseIndex += info.startFromLastElement;
        std::string substring = seqInfo.sequence.substr(nextLowercaseIndex, info.length);
        // pretvaram sva mala slova u velika
        for (char& c : substring) {
            c = std::tolower(c);
        }
        // ubacujem substring u sekvencu
        seqInfo.sequence.replace(nextLowercaseIndex, info.length, substring);
        nextLowercaseIndex += info.length;
    }
    
    // upisivanje u file
    ofstream outputFile(outputFileName);
    outputFile << seqInfo.identifier << endl;
    for (unsigned int i = 0; i < seqInfo.sequence.size(); i += seqInfo.lineWidth){
        if (i != 0) {
            outputFile << endl;
        }
    outputFile << seqInfo.sequence.substr(i, seqInfo.lineWidth);
}

}

