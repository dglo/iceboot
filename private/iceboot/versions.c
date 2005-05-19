#include "versions.h"

#define STR(a) #a
#define STRING(a) STR(a)

Versions versions = {
   .tag   = STRING(PROJECT_TAG);
   .build = STRING(ICESOFT_BUILD);
};
