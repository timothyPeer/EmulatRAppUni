#pragma once


// Invalidation Group Example
enum class InvalidationType {
	ALL,           // Invalidate entire TLB
	BY_ASN,        // Invalidate entries for specific ASN
	PROCESS,	   // Invalidate entries for Process
	SINGLE_ENTRY   // Invalidate specific entry
};
