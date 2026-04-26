#ifndef ROM_LOADER_UTIL_H
#define ROM_LOADER_UTIL_H

/*
 * Tiny helpers shared between rom_loader.c and rom_zip.c. Both files
 * historically defined identical copies of these — having one source of
 * truth avoids bug fixes drifting between the two.
 */

char rom_loader_util_to_lower_ascii(char c);

int rom_loader_util_is_cdrom_path(const char *path);

/*
 * Case-insensitive extension comparison. `ext` should include the leading
 * dot, e.g. ".sfc". A trailing ";1" on the path (ISO9660 file-version
 * suffix) is accepted as a match.
 */
int rom_loader_util_ext_equals(const char *path, const char *ext);

/*
 * Returns a pointer into `path` past the last '/' or '\' separator.
 * Returns "" if path is NULL.
 */
const char *rom_loader_util_base_name_only(const char *path);

#endif /* ROM_LOADER_UTIL_H */
