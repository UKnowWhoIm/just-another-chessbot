#include <fstream>
#include <memory>
#include "definitions.hpp"
#include "statsutil.hpp"

#define TT_EXACT 1
#define TT_UB 2
#define TT_LB 3

class HashEntry {
    public:
        unsigned long long zobristHash;
        long score;
        uint8_t depth;
        uint8_t flag;
        bool isAncient;
        moveType bestMove;

        HashEntry() {
            isAncient = true;
            depth = 0;
        }

        HashEntry(unsigned long long hash, uint8_t depth, long score, uint8_t flag, moveType bestMove) {
            this->zobristHash = hash;
            this->isAncient = false;
            this->depth = depth;
            this->flag = flag;
            this->bestMove = bestMove;
            this->score = score;
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
            if(newHash.flag == TT_EXACT && this->flag != TT_EXACT)
                // New hash has exact value
                return true;
            if(this->flag == TT_EXACT && newHash.flag != TT_EXACT)
                // Old hash has exact value
                return false;

            if (this->score != newHash.score && newHash.flag != TT_LB && this->flag != TT_LB) {
                return this->score < newHash.score;
            }
            return this->score > newHash.score;
        }

        void looked() {
            this->isAncient = false;
        }
};

std::istream& operator>> (std::istream& is, HashEntry& hashEntry) {
    is >> hashEntry.zobristHash;
    is >> hashEntry.score;
    is >> hashEntry.depth;
    is >> hashEntry.flag;
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
    os << hashEntry.flag;
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
                logging::d("TT", "Cache file found, reading...");
                for (int i=0; i < CACHE_SIZE; i++) {
                    cacheFile >> cache[i];
                }
                cacheFile.close();
            } else {
                logging::d("TT", "Cache file not found, creating new cache");
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
            } else {
                stats::collisionTT();
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

const string TranspositionTable::cacheRoot = "/app/engines/transpositionTables/";
typedef std::shared_ptr<TranspositionTable> ttType;
