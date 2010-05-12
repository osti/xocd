/* constants.h is part of XMLS - X-ray MultiLayer mirror Simulations */
/* please see README for general information */

/* constants is a small web server servic optical constants */

#ifndef CONSTANTS
#define CONSTANTS

#include <string>
#include <vector>
#include <iostream>
#include <string.h>
#include <stdlib.h>

using namespace std;

#include "mongoose.h"

typedef struct mg_connection c;
typedef const struct mg_request_info r;

string path;
string port;
struct mg_context* ctx;

void head(c*, int);
void say(c*, const char*);

void index (c*, r*, void*);
void all   (c*, r*, void*);
void energy(c*, r*, void*);
void wavel (c*, r*, void*);

char* interpolate(const char*, double);

#endif // CKERNEL_H

