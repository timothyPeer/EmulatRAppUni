#ifndef TRACECONVERT_H
#define TRACECONVERT_H

#include <string>
#include <QString>

inline std::string toStdString(const std::string& s) {
    return s;
}

inline std::string toStdString(const char* s) {
    return std::string{s};
}

inline std::string toStdString(const QString& s) {
    return s.toStdString();
}

#endif

