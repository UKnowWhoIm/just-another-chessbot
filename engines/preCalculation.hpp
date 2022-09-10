#ifndef CHESS_PRECALC
#define CHESS_PRECALC 1

#include<string>
#include<fstream>
#include<memory>

#include "definitions.hpp"
#include "log.hpp"

using std::array;
using std::string;

namespace preCalculation {
    struct preCalc {
        array<array<boardType, 512>, 64> bishopLookup;
        array<array<boardType, 4096>, 64> rookLookup;
        array<unsigned long long, 64> bishopMagic;
        array<unsigned long long, 64> rookMagic;
        array<boardType, 64> bishopBlockers;
        array<boardType, 64> rookBlockers;
        prnType PRN;
        bool operator==(const preCalc &other);
    };

    typedef std::shared_ptr<preCalc> preCalcType;

    bool preCalc::operator==(const preCalc &other) {
        for (int i=0; i < other.bishopLookup.size(); i++) {
            for (int j=0; j < other.bishopLookup[0].size(); j++) {
                if (bishopLookup[i][j] != other.bishopLookup[i][j]) {
                    logging::printData("Bishop lookup failed");
                    logging::printData({i, j});
                    logging::printData({bishopLookup[i][j], other.bishopLookup[i][j]});
                    return false;
                }
            }
        }
        for (int i=0; i < other.rookLookup.size(); i++) {
            for (int j=0; j < other.rookLookup[0].size(); j++) {
                if (rookLookup[i][j] != other.rookLookup[i][j]) {
                    logging::printData("Rook lookup failed");
                    logging::printData({i, j});
                    logging::printData({rookLookup[i][j], other.rookLookup[i][j]});
                    return false;
                }
            }
        }
        for (int i=0; i < other.bishopMagic.size(); i++) {
            if (bishopMagic[i] != other.bishopMagic[i]) {
                logging::printData("BishopMagic lookup failed");
                logging::printData({bishopMagic[i], other.bishopMagic[i]});
                return false;
            }
        }
        for (int i=0; i < other.rookMagic.size(); i++) {
            if (rookMagic[i] != other.rookMagic[i]) {
                logging::printData("rookMagic lookup failed");
                logging::printData({rookMagic[i], other.rookMagic[i]});
                return false;
            }
        }
        for (int i=0; i < other.bishopBlockers.size(); i++) {
            if (bishopBlockers[i] != other.bishopBlockers[i]) {
                logging::printData("bishopBlockers lookup failed");
                logging::printData({bishopBlockers[i], other.bishopBlockers[i]});
                return false;
            }
        }
        for (int i=0; i < other.rookBlockers.size(); i++) {
            if (rookBlockers[i] != other.rookBlockers[i]) {
                logging::printData("rookBlockers lookup failed");
                logging::printData({rookBlockers[i], other.rookBlockers[i]});
                return false;
            }
        }
        for (int i=0; i < other.PRN.size(); i++) {
            if (PRN[i] != other.PRN[i]) {
                logging::printData("PRN lookup failed");
                logging::printData({PRN[i], other.PRN[i]});
                return false;
            }
        }
        return true;
    }

    std::istream& operator>> (std::istream& is, preCalc& data) {
        unsigned long long temp;
        for (int i=0; i < data.bishopLookup.size(); i++) {
            for (int j=0; j < data.bishopLookup[0].size(); j++) {
                is >> temp;
                data.bishopLookup[i][j] = boardType(temp);
            }
        }
        for (int i=0; i < data.rookLookup.size(); i++) {
            for (int j=0; j < data.rookLookup[0].size(); j++) {
                is >> temp;
                data.rookLookup[i][j] = boardType(temp);
            }
        }
        for (int i=0; i < data.bishopMagic.size(); i++) {
            is >> data.bishopMagic[i];
        }
        for (int i=0; i < data.rookMagic.size(); i++) {
            is >> data.rookMagic[i];
        }
        for (int i=0; i < data.bishopBlockers.size(); i++) {
            is >> temp;
            data.bishopBlockers[i] = boardType(temp);
        }
        for (int i=0; i < data.rookBlockers.size(); i++) {
            is >> temp;
            data.rookBlockers[i] = boardType(temp);
        }
        for (int i=0; i < data.PRN.size(); i++) {
            is >> data.PRN[i];
        }
        return is;
    }

    std::ostream& operator<< (std::ostream& os, const preCalc& data) {
        for (int i=0; i < data.bishopLookup.size(); i++) {
            for (int j=0; j < data.bishopLookup[0].size(); j++) {
                os << data.bishopLookup[i][j].to_ullong() << " ";
            }
            os << " ";
        }
        for (int i=0; i < data.rookLookup.size(); i++) {
            for (int j=0; j < data.rookLookup[0].size(); j++) {
                os << data.rookLookup[i][j].to_ullong() << " ";
            }
            os << " ";
        }
        for (int i=0; i < data.bishopMagic.size(); i++) {
            os << data.bishopMagic[i] << " ";
        }
        for (int i=0; i < data.rookMagic.size(); i++) {
            os << data.rookMagic[i] << " ";
        }
        for (int i=0; i < data.bishopBlockers.size(); i++) {
            os << data.bishopBlockers[i].to_ullong() << " ";
        }
        for (int i=0; i < data.rookBlockers.size(); i++) {
            os << data.rookBlockers[i].to_ullong() << " ";
        }
        for (int i=0; i < data.PRN.size(); i++) {
            os << data.PRN[i] << " ";
        }
        return os;
    }

    preCalcType load() {
        preCalcType data = std::make_shared<preCalc>();
        std::ifstream file("/app/engines/utils/preCalc.dat");
        file >> *data;
        file.close();
        return data;
    }

    // preCalcType loadJSON() {
    //     // Load the magics and blockers
    //     string attacksFile = "/app/engines/utils/attacks.json";
    //     string magicFile = "/app/engines/utils/magics.json";
    //     string blockerFile = "/app/engines/utils/blockers.json";
    //     string prnFile = "/app/engines/utils/PRN";

    //     json attacks, magics, blockers;
    //     std::ifstream aF(attacksFile), mF(magicFile), bF(blockerFile), pF(prnFile);
    //     aF >> attacks;
    //     mF >> magics;
    //     bF >> blockers;

    //     unsigned long long randomNum;
    //     uint16_t i = 0;
    //     prnType PRN;
    //     while (pF >> randomNum) {
    //         PRN[i] = randomNum;
    //         i++;
    //     }

    //     array<array<unsigned long long, 512>, 64> rawBishop = attacks.at("bishop").get<array<array<unsigned long long, 512>, 64>>();
    //     array<array<unsigned long long, 4096>, 64> rawRook =  attacks.at("rook").get<array<array<unsigned long long, 4096>, 64>>();
    //     array<array<boardType, 512>, 64> bishopLookup;
    //     array<array<boardType, 4096>, 64> rookLookup;
    //     array<unsigned long long, 64> rawBishopBlockers = blockers.at("bishop").get<array<unsigned long long, 64>>();
    //     array<unsigned long long, 64> rawRookBlockers = blockers.at("rook").get<array<unsigned long long, 64>>();
    //     array<boardType, 64> bishopBlockers;
    //     array<boardType, 64> rookBlockers;

    //     for (int i=0; i<64; i++) {
    //         for (int j=0; j<4096; j++) {
    //             if (j < 512) {
    //                 bishopLookup[i][j] = boardType(rawBishop[i][j]);
    //             } 
    //             rookLookup[i][j] = boardType(rawRook[i][j]);   
    //         }
    //         bishopBlockers[i] = boardType(rawBishopBlockers[i]);
    //         rookBlockers[i] = boardType(rawRookBlockers[i]);
    //     }
    //     preCalcType data = std::make_shared<preCalc>(
    //         bishopLookup,
    //         rookLookup,
    //         magics.at("bishop").get<array<unsigned long long, 64>>(),
    //         magics.at("rook").get<array<unsigned long long, 64>>(),
    //         bishopBlockers,
    //         rookBlockers,
    //         PRN
    //     );
    //     return data;
    // }
};

#endif