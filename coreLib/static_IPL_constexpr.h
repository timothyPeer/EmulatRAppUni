#pragma once
#include <QtGlobal>

// ======================================================================
// AST delivery constants
// ----------------------------------------------------------------------
// ASA + OpenVMS model: ASTs are delivered at a low IPL (typically IPL 2),
// below most hardware interrupts but above pure user-level activity.
// This value is used as the IPL override when entering the AST PAL vector.
//
// Ref: Alpha Architecture Handbook / OS PALcode docs – AST delivery IPL
//      is a fixed low priority; VMS commonly uses IPL 2 for AST.
// ======================================================================
inline constexpr quint8 AST_DELIVERY_IPL = 2;