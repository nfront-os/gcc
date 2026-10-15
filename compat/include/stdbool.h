/*
 * 9front-gcc: replaces APE's stdbool.h, which is from before C99
 * ("typedef char _Bool": an error now _Bool is a keyword). As gcc's own:
 * in C23 bool, true and false are keywords, in C++ built in.
 */
#ifndef __STDBOOL_H__
#define __STDBOOL_H__

#ifndef __cplusplus
#if !defined __STDC_VERSION__ || __STDC_VERSION__ < 202311L
#define bool	_Bool
#define true	1
#define false	0
#endif
#endif
#define __bool_true_false_are_defined	1

#endif
