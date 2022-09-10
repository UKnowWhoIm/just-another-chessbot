#include <fstream>
#include <memory>
#include "definitions.hpp"
#include "statsutil.hpp"


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
            depth = 0;
        }

        HashEntry(unsigned long long hash, uint8_t depth, unsigned long score, bool isBeta, moveType bestMove) {
            this->zobristHash = hash;
            this->isAncient = false;
            this->depth = depth;
            this->isBetaCutOff = isBeta;
            this->bestMove = bestMove;
            this->score = score;
        }

        HashEntry(const HashEntry& other) {
            this->zobristHash = other.zobristHash;
            this->isAncient = other.isAncient;
            this->depth = other.depth;
            this->isBetaCutOff = other.isBetaCutOff;
            this->bestMove = other.bestMove;
            this->score = other.score;
        }

        bool replaceHash(const HashEntry &newHash) {
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

        TranspositionTable() {
            // Empty tables used when TT is disabled
            cache = std::vector<HashEntry>();
            cache.shrink_to_fit();
            isLoaded = false;
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
            if (!isLoaded) {
                return std::shared_ptr<HashEntry>(nullptr);
            }
            std::shared_ptr<HashEntry> entry = std::make_shared<HashEntry>(cache[zobristVal % CACHE_SIZE]);
            if (entry->zobristHash == zobristVal) {
                entry->looked();
                return entry;
            }
            stats::missTT();
            return nullptr;
        }

        void set(const HashEntry &entry) {
            if (!isLoaded) {
                return;
            }
            if (cache[entry.zobristHash % CACHE_SIZE].replaceHash(entry)) {
                stats::insertTT();
                cache[entry.zobristHash % CACHE_SIZE] = entry;
            }
        }

        void dumpCache() {
            if (!isLoaded) {
                return;
            }
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

        void resetAge() {
            if (!isLoaded) {
                return;
            }
            for (auto i = cache.begin(); i != cache.end(); i++) {
                i->isAncient = true;
            }
        }
};

const string TranspositionTable::cacheRoot = "/app/engines/TranspositionTables/";
typedef std::shared_ptr<TranspositionTable> ttType;
