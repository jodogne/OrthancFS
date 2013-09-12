#ifndef _PARAMS_H_
#define _PARAMS_H_

#define FUSE_USE_VERSION 26

#define _XOPEN_SOURCE 500

#include <limits.h>
#include <stdio.h>
struct ofs_state {
    FILE *logfile;
    char *orthanc_adress;
};

#define OFS_DATA ((struct ofs_state *) fuse_get_context()->private_data)

#endif
