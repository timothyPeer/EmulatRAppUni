#ifndef BRANCHPREDICTIONSTATS_H
#define BRANCHPREDICTIONSTATS_H
#include <QtGlobal>
/**
 * @brief Branch predictor performance statistics.
 *
 * Add this to BranchPredictor class if you want statistics.
 */
struct BranchPredictorStats {
    quint64 totalPredictions{0};      // Total predictions made
    quint64 correctPredictions{0};    // Correct predictions
    quint64 mispredictions{0};        // Incorrect predictions
    double accuracy{0.0};             // Prediction accuracy (0.0-1.0)

    quint64 btbHits{0};               // Branch Target Buffer hits
    quint64 btbMisses{0};             // Branch Target Buffer misses

    quint64 directionCorrect{0};      // Direction predicted correctly
    quint64 directionWrong{0};        // Direction predicted incorrectly

    quint64 targetCorrect{0};         // Target predicted correctly
    quint64 targetWrong{0};           // Target predicted incorrectly
};

#endif // BRANCHPREDICTIONSTATS_H
