#ifndef CHESS_STATSUTIL
#define CHESS_STATSUTIL 1
#include<chrono>

#include "log.hpp"

namespace stats {
    unsigned long pruneCount;
    unsigned long heruisticCount;
    
    unsigned long ttExactHits;
    unsigned long ttAlphaHits;
    unsigned long ttBetaHits;
    unsigned long ttMisses;
    unsigned long ttCollisions;
    unsigned long ttInserts;
    unsigned long ttExactInconsistent;

    unsigned long pvNullWindowSuccess;
    unsigned long pvNullWindowFail;

    unsigned long nullMovePruned;
    unsigned long nullMoveNotPruned;

    unsigned long illegalKingCapturePrune;

    unsigned long qSearchCount;

    std::chrono::_V2::system_clock::time_point startTime;

    void printStats() {
        auto now = std::chrono::system_clock::now();
        logging::i("Elapsed time(s): ", std::chrono::duration_cast<std::chrono::seconds>(now - startTime).count());
        logging::i("Prune count: ", pruneCount);
        logging::i("Heruistic count: ", heruisticCount);
        logging::i("QSearch Count: ", qSearchCount);
        logging::i("TT Exact Hits: ", ttExactHits);
        logging::i("TT Alpha Hits: ", ttAlphaHits);
        logging::i("TT Beta Hits: ", ttBetaHits);
        logging::i("TT Misses: ", ttMisses);
        logging::i("TT Collisions: ", ttCollisions);
        logging::i("TT Inserts: ", ttInserts);
        logging::i("TT Exact Inconsistent", ttExactInconsistent);
        logging::i("PV Null Window Success: ", pvNullWindowSuccess);
        logging::i("PV Null Window Fail: ", pvNullWindowFail);
        logging::i("Null Move Pruned: ", nullMovePruned);
        logging::i("Null Move Not Pruned: ", nullMoveNotPruned);
        logging::i("Illegal King Capture Prune: ", illegalKingCapturePrune);
    }

    void hitTTExact() {
        ttExactHits++;
    }

    void hitTTAlpha() {
        ttAlphaHits++;
    }

    void hitTTBeta() {
        ttBetaHits++;
    }

    void collisionTT() {
        ttCollisions++;
    }

    void insertTT() {
        ttInserts++;
    }

    void missTT() {
        ttMisses++;
    }

    void prune() {
        pruneCount++;
    }

    void heruistic() {
        heruisticCount++;
    }

    void pvZWSSuccess() {
        pvNullWindowSuccess++;
    }

    void pvZWSFail() {
        pvNullWindowFail++;
    }

    void nullMovePrune() {
        nullMovePruned++;
    }

    void nullMoveNotPrune() {
        nullMoveNotPruned++;
    }

    void illegalKingCapture() {
        illegalKingCapturePrune++;
    }

    void quiescenceSearch() {
        qSearchCount++;
    }


    void reset() {
        startTime = std::chrono::system_clock::now();
        pruneCount = 0;
        heruisticCount = 0;
        ttExactHits = 0;
        ttAlphaHits = 0;
        ttBetaHits = 0;
        ttMisses = 0;
        ttCollisions = 0;
        ttInserts = 0;
        ttExactInconsistent = 0;
        pvNullWindowSuccess = 0;
        pvNullWindowFail = 0;
        illegalKingCapturePrune = 0;
    }
}

#endif