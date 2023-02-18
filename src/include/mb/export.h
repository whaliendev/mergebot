
#ifndef MB_INCLUDE_EXPORT_H_
#define MB_INCLUDE_EXPORT_H_

#if !defined(MB_EXPORT)

#if defined(MB_SHARED_LIBRARY)
#if defined(_WIN32)

#if defined(MB_COMPILE_LIBRARY)
#define MB_EXPORT __declspec(dllexport)
#else
#define MB_EXPORT __declspec(dllimport)
#endif  // defined(MB_COMPILE_LIBRARY)

#else  // defined(_WIN32)
#if defined(MB_COMPILE_LIBRARY)
#define MB_EXPORT __attribute__((visibility("default")))
#else
#define MB_EXPORT
#endif
#endif  // defined(_WIN32)

#else  // defined(MB_SHARED_LIBRARY)
#define MB_EXPORT
#endif

#endif  // !defined(MB_EXPORT)

#endif  // MB_INCLUDE_EXPORT_H_
