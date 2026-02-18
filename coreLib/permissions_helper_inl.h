#pragma once
#include <QtGlobal>
#include "enum_header.h"

using corePermMask = quint8;


static constexpr quint64 pageShift(PageSizeCode code) noexcept {
	switch (code) {
	case PageSizeCode::PageSize_4K:  return 12;
	case PageSizeCode::PageSize_8K:  return 13;
	case PageSizeCode::PageSize_64K: return 16;
	default: return 12;  // Default 4K
	}
}

static constexpr quint64 pageSizeBytes(PageSizeCode code) noexcept {
	return 1ULL << pageShift(code);
}


// Protection computation helpers
static constexpr bool allowRead(AccessPerm perm, bool userMode) noexcept {
	switch (perm) {
	case AccessPerm::Read:
	case AccessPerm::ReadExec:
	case AccessPerm::ReadWrite:
	case AccessPerm::Full:
		return true;
	default:
		return false;
	}
}

static constexpr bool allowWrite(AccessPerm perm, bool userMode) noexcept {
	switch (perm) {
	case AccessPerm::Write:
	case AccessPerm::WriteExec:
	case AccessPerm::ReadWrite:
	case AccessPerm::Full:
		return true;
	default:
		return false;
	}
}

static constexpr bool allowExecute(AccessPerm perm, bool userMode) noexcept {
	switch (perm) {
	case AccessPerm::Execute:
	case AccessPerm::ReadExec:
	case AccessPerm::WriteExec:
	case AccessPerm::Full:
		return true;
	default:
		return false;
	}
}
inline bool canUserRead(quint8 perms) { return perms & USER_READ; }
inline bool canUserWrite(quint8 perms) { return perms & USER_WRITE; }
inline bool canUserExec(quint8 perms) { return perms & USER_EXEC; }
inline bool canKernelRead(quint8 perms) { return perms & KERNEL_READ; }

inline bool hasPermission(quint8 perms, AccessType type, PrivilegeLevel level) {
	switch (type) {
	case Read:
		return (level == (PrivilegeLevel)Mode_Privilege::User) ? (perms & USER_READ) : (perms & KERNEL_READ);
	case Write:
		return (level == (PrivilegeLevel)Mode_Privilege::User) ? (perms & USER_WRITE) : (perms & KERNEL_WRITE);
	case Execute:
		return (level == (PrivilegeLevel)Mode_Privilege::User) ? (perms & USER_EXEC) : (perms & KERNEL_EXEC);
	}
	return false;
}