#pragma once

#define __sync_fetch_and_add(ptr,value)		_InterlockedExchangeAdd(ptr,value)
#define __sync_fetch_and_sub(ptr, value)	_InterlockedExchangeAdd(ptr,-value)
#define __sync_add_and_fetch(ptr, value)	_InterlockedAdd_r(ptr,value)
#define __sync_sub_and_fetch(ptr, value)	_InterlockedAdd_r(ptr,-value)
#define __sync_or_and_fetch(ptr, value)		_InterlockedOr(ptr ,value)
#define __sync_and_and_fetch(ptr, value)	_InterlockedAnd(ptr ,value)
#define __sync_xor_and_fetch(ptr, value)	_InterlockedXor(ptr ,value)

#define __sync_bool_compare_and_swap(ptr, oldval,newval) (_InterlockedCompareExchange(ptr,newval ,oldval)==oldval)

#define __sync_lock_test_and_set(ptr ,value) _InterlockedExchange(ptr ,value)
#define __sync_lock_release(ptr) _InterlockedExchange(ptr ,0)