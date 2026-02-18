#pragma once
#include <QtGlobal>

/**
 * @brief Get utilization statistics including min, max, and distribution
 */
struct UtilizationStats {
    double overall;        // Overall cache utilization
    double average;        // Average set utilization
    double minimum;        // Minimum set utilization
    double maximum;        // Maximum set utilization
    double stdDeviation;   // Standard deviation of set utilizations
    qsizetype validLines;  // Total valid lines
    qsizetype totalLines;  // Total cache lines
};