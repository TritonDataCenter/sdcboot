#ifndef FAT_TRANSFORMATION_ENGINE
#define FAT_TRANSFORMATION_ENGINE

/* low level functionality */
#include "bool.h"
#include "traversl.h"
#include "direct.h"
#include "drive.h"
#include "fat.h"
#include "fatconst.h"
#include "fteerr.h"
#include "rdwrsect.h"
#include "subdir.h"
#include "boot.h"
#include "fsinfo.h"
#include "dataarea.h"
#include "volume.h"
#include "backup.h"
#include "lfn.h"

/* high level functionality */
#include "cpysct.h"
#include "dircnt.h"
#include "fndffspc.h"
#include "fndcidir.h"
#include "fndcifat.h"
#include "fndlstct.h"
#include "gtnthcst.h"
#include "nthentry.h"
#include "nthflclt.h"
#include "relocclt.h"
#include "swpclst.h"
#include "walktree.h"
#include "adcpdirs.h"
#include "fexist.h"
#include "filechn.h"
#include "locpath.h"
#include "walkwpth.h"
#include "tracepth.h"
#include "clcfsize.h"
#include "freespac.h"

/* Miscelanous functionality */
#include "ftemisc.h"
#include "wildcard.h"
#include "pathconv.h"
#include "mkabspth.h"

/* Heap memory management */
#include "ftemem.h"

/* Sector cache */
#include "sctcache.h"
#include "preread.h"

/* Data structures */
#include "dtstrct.h"

#endif
