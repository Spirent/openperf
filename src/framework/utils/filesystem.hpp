#ifndef _OP_UTILS_FILESYSTEM_HPP_
#define _OP_UTILS_FILESYSTEM_HPP_

/*
 * If we have the __has_include macro, use it to figure out if have
 * the <filesystem> header. If we have neither, presume that we
 * have the <experimental/filesystem> header.
 */
#if defined(__has_include) && __has_include(<filesystem>)
#define HAVE_EXPERIMENTAL_FILESYSTEM 0
#else
#define HAVE_EXPERIMENTAL_FILESYSTEM 1
#endif

#if HAVE_EXPERIMENTAL_FILESYSTEM
#include <experimental/filesystem>
namespace std {
namespace filesystem = experimental::filesystem;
}
#else
#include <filesystem>
#endif

#endif /* _OP_UTILS_FILESYSTEM_HPP_ */
