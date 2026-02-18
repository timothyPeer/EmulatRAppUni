// exceptionVectorTable.h
#pragma once
#include <QVector>
#include <QtGlobal>
#include "ExceptionHandler.h"

class ExceptionVectorTable {
public:
	// Table initialization
	void initEV6() {}   // Load EV6 exception vectors
	void initEV5() {}   // Load EV5 exception vectors (optional)
	void initCustom() {} // For VMS, Linux/Tru64 special handlers

	// Access lookup
	inline const ExceptionHandler& handlerForVector(quint32 vector) const {
		return m_handlers[vector];
	}

	ExceptionVectorTable() = default;
private:
	

	QVector<ExceptionHandler> m_handlers; // static mapping
};

inline ExceptionVectorTable& globalExceptionVectorTable()
{
	static ExceptionVectorTable instance;
	return instance;
}

