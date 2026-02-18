#pragma once
#include <QtGlobal>
#include "../mmioLib/mmio_Manager.h"
#include "../coreLib/define_helpers.h"
#include "raiseMachineCheck_inl.h"


void handleMMIOFault(quint64 pa, quint8 width, MMIOStatus status, bool isRead)
{
	QString opStr = isRead ? "Read" : "Write";
	QString statusStr;

	switch (status) {
	case MMIOStatus::UNIMPL:
		statusStr = "register not implemented";
		break;
	case MMIOStatus::WRITE_ONLY:
		statusStr = "write-only register";
		break;
	case MMIOStatus::READ_ONLY:
		statusStr = "read-only register";
		break;
	case MMIOStatus::DEVICE_ERROR:
		statusStr = "device error";
		break;
	case MMIOStatus::TIMEOUT:
		statusStr = "device timeout";
		break;
	case MMIOStatus::BUS_ERROR:
		statusStr = "bus error (no device)";
		break;
	case MMIOStatus::ALIGN_FAULT:
		statusStr = "alignment fault";
		break;
	case MMIOStatus::WIDTH_FAULT:
		statusStr = "unsupported width";
		break;
	default:
		statusStr = "unknown error";
	}

	ERROR_LOG(QString("MMIO %1 fault: PA=0x%2 width=%3 - %4")
		.arg(opStr)
		.arg(pa, 16, 16, QChar('0'))
		.arg(width)
		.arg(statusStr));

	// Raise machine check (Alpha ISA)
	 raiseMachineCheck(pa, isRead); // TODO Inline Stub Created
}