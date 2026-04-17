#ifndef LAUNCHER_BROWSER_INTERNAL_H
#define LAUNCHER_BROWSER_INTERNAL_H

#include "launcher_browser.h"
#include "launcher_browser_state.h"
#include "launcher_theme.h"

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define LAUNCHER_BROWSER_ROOT ""
#define LAUNCHER_BROWSER_ROOT_LABEL "DEVICES"
#define LAUNCHER_BROWSER_LOAD_CHUNK 256
#define LAUNCHER_BROWSER_CAPACITY_GROW 256
#define LAUNCHER_BROWSER_MAX_USB_DEVICES 10

int launcher_browser_is_root_path(const char *path);
void launcher_browser_close_scan_dir(void);
void launcher_browser_clear_entries(void);
int launcher_browser_ensure_capacity(int need);
int launcher_browser_append_entry(const char *name, const char *full_path, int is_dir);

void launcher_browser_init(void);
int launcher_browser_reset_to_path(const char *path);
int launcher_browser_open(const char *path);
int launcher_browser_refresh(void);
int launcher_browser_go_parent(void);

int launcher_browser_scan_root_devices(void);
int launcher_browser_scan_memory_card_path(const char *path);
int launcher_browser_open_scan_dir(const char *path);
int launcher_browser_scan_disc_path(const char *path);
int launcher_browser_load_more_entries(int want);
void launcher_browser_path_join(char *out, size_t out_size, const char *base, const char *name);
void launcher_browser_sort_entries(void);

#endif
