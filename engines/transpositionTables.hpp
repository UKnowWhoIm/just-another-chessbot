#include <fstream>
#include <memory>
#include "definitions.hpp"


class HashEntry {
    public:
        unsigned long long zobristHash;
        unsigned long score;
        uint8_t depth;
        bool isBetaCutOff;
        bool isAncient;
        moveType bestMove;

        HashEntry() {
            isAncient = true;
            depth = -1;
        }

        HashEntry(unsigned long long &hash, unsigned long &gameId, uint8_t &depth, bool &isBeta, moveType &bestMove) {
            this->zobristHash = hash;
            this->isAncient = false;
            this->depth = depth;
            this->isBetaCutOff = isBeta;
            this->bestMove = bestMove;
        }

        HashEntry(const HashEntry& other) {
            this->zobristHash = other.zobristHash;
            this->isAncient = other.isAncient;
            this->depth = other.depth;
            this->isBetaCutOff = other.isBetaCutOff;
            this->bestMove = other.bestMove;
        }

        bool replaceHash(HashEntry &newHash) {
            if(!this->isAncient)
                // This hash is not old
                return false;
            if(this->depth > newHash.depth)
                // This hash searched deeper
                return false;
            if(this->depth < newHash.depth)
                // New hash searched deeper
                return true;
            if(!newHash.isBetaCutOff)
                // New hash has exact value
                return true;
            if(!this->isBetaCutOff)
                // Old hash has exact value
                return false;
            
            // Both have beta cutoffs, store the 1 with lower cutoff
            return (this->score < newHash.score);
        }

        void looked() {
            this->isAncient = false;
        }
};

std::istream& operator>> (std::istream& is, HashEntry& hashEntry) {
    is >> hashEntry.zobristHash;
    is >> hashEntry.score;
    is >> hashEntry.depth;
    is >> hashEntry.isBetaCutOff;
    is >> hashEntry.isAncient;
    uint8_t origin, target;
    is >> origin;
    is >> target;
    hashEntry.bestMove = {origin, target};
    return is;
}

std::ostream& operator<< (std::ostream& os, const HashEntry& hashEntry) {
    os << hashEntry.zobristHash;
    os << hashEntry.score;
    os << hashEntry.depth;
    os << hashEntry.isBetaCutOff;
    os << hashEntry.isAncient;
    os << hashEntry.bestMove[0];
    os << hashEntry.bestMove[1];
    return os;
}

class TranspositionTable {
    private:
        std::vector<HashEntry> cache;
        string gameId;
        bool isLoaded = false;
    public:
        static const string cacheRoot;
        TranspositionTable(string gameId) {
            cache = std::vector<HashEntry>(CACHE_SIZE);
            this->gameId = gameId;
            loadCache();
        }

        void loadCache() {
            string cachePath = cacheRoot + gameId;
            std::ifstream cacheFile(cachePath);
            if (cacheFile.is_open()) {
                for (int i=0; i < CACHE_SIZE; i++) {
                    cacheFile >> cache[i];
                }
                cacheFile.close();
            } else {
                cache.assign(CACHE_SIZE, HashEntry());
            }
            isLoaded = true;
        }

        std::shared_ptr<HashEntry> get(const unsigned long long &zobristVal) {
            std::shared_ptr<HashEntry> entry = std::make_shared<HashEntry>(cache[zobristVal % CACHE_SIZE]);
            if (entry->zobristHash == zobristVal) {
                return entry;
            }
            return nullptr;
        }

        void set(const unsigned long long &zobristVal, const std::shared_ptr<HashEntry> &entry) {
            if (cache[zobristVal % CACHE_SIZE].replaceHash(*entry)) {
                cache[zobristVal % CACHE_SIZE] = *entry;
            }
        }

        void dumpCache() {
            string cachePath = cacheRoot + gameId;
            std::ofstream cacheFile(cachePath);
            if (cacheFile.is_open()) {
                for (int i=0; i < CACHE_SIZE; i++) {
                    cacheFile << cache[i];
                }
                cacheFile.close();
            }
        }

        bool isCacheLoaded() {
            return isLoaded;
        }
};

const string TranspositionTable::cacheRoot = "/app/engines/TranspositionTables/";
typedef std::shared_ptr<TranspositionTable> ttType;
