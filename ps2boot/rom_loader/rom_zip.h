#ifndef ROM_ZIP_H
#define ROM_ZIP_H

#include <stddef.h>

int rom_zip_load(const char *zip_path, void **out_data, size_t *out_size,
                 char *out_name, size_t out_name_size);

int rom_zip_extract_to_temp_file(const char *zip_path,
                                 char *out_path,
                                 size_t out_path_size,
                                 char *out_name,
                                 size_t out_name_size);

#endif
