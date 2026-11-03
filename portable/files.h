#ifndef PHOTON_PORTABLE_FILES_H
#define PHOTON_PORTABLE_FILES_H

#include <stddef.h>

#include <errors.h>
#include <memory.h>

error_t fsReadFile(c_string_t path, char** buffer, size_t* len);

#endif