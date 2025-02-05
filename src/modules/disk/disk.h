#pragma once

#include "fastfetch.h"

#define FF_DISK_MODULE_NAME "Disk"

void ffPrintDisk(FFDiskOptions* options);
void ffInitDiskOptions(FFDiskOptions* options);
bool ffParseDiskCommandOptions(FFDiskOptions* options, const char* key, const char* value);
void ffDestroyDiskOptions(FFDiskOptions* options);
void ffParseDiskJsonObject(FFDiskOptions* options, yyjson_val* module);
void ffGenerateDiskJson(FFDiskOptions* options, yyjson_mut_doc* doc, yyjson_mut_val* module);
void ffPrintDiskHelpFormat(void);
