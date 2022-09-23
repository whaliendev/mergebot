/**
 * @file c.h
 * @author hwa (hwahe.cs@gmail.com)
 * @brief C bindings for mergebot. 
 * @version 0.1
 * @date 2022-09-23
 * 
 * @copyright Copyright (c) 2022 The CSTAR Lab of WHU. All rights reserved. 
 * 
 * Maybe useful as a stable ABI that can be used by programs that keep mergebot
 * in a shared library, or for a JNI api. 
 * 
 * Some conventions we try to follow: 
 * (1) we expose just opaque struct pointers and functions to clients. This 
 * allows us to change internal representations without having to recompile 
 * clients. 
 * 
 * (2) Errors are represented by a null-terminated c string. NULL means no 
 * error. All operations that can raise an error are passwd a "char** errptr" as
 * the last argument. One of the following must be true on entry: 
 *   *errptr == NULL
 *   *errptr points to a malloc()ed null-terminated error message
 * On success, a mergebot routine leaves *errptr unchanged. 
 * On failure, mergebot frees the old value of *errptr and set *errptr to a 
 * malloc()ed error message. 
 * 
 * (3) Bools have the type uint8_t (0 == false; rest == true)
 * 
 * (4) All of the pointer arguments must be non-NULL. 
 * 
 */

#define MB_INCLUDE_C_H_
#define MB_INCLUDE_C_H_



#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif