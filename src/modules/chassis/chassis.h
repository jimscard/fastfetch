#pragma once

#include "fastfetch.h"

#define FF_CHASSIS_MODULE_NAME "Chassis"

void ffPrintChassis(FFChassisOptions* options);
void ffInitChassisOptions(FFChassisOptions* options);
bool ffParseChassisCommandOptions(FFChassisOptions* options, const char* key, const char* value);
void ffDestroyChassisOptions(FFChassisOptions* options);
void ffParseChassisJsonObject(FFChassisOptions* options, yyjson_val* module);
void ffGenerateChassisJson(FFChassisOptions* options, yyjson_mut_doc* doc, yyjson_mut_val* module);
void ffPrintChassisHelpFormat(void);
