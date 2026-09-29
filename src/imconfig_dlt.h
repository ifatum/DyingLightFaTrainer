#pragma once
void dlt_assert_fail(const char* expr, const char* file, int line);
#define IM_ASSERT(_EXPR) ((_EXPR) ? (void)0 : dlt_assert_fail(#_EXPR, __FILE__, __LINE__))
