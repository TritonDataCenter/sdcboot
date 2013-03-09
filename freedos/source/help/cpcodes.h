#ifndef _CPCODES_H_INCLUDED
#define _CPCODES_H_INCLUDED

#define CP_ENTRY(b, a) {(b), (a)}

const char supportedCodepages[] = "437\n"
                                  "737\n"
                                  "850, 858\n"
                                  "851\n"
                                  "852\n"
                                  "853\n"
                                  "855, 872\n"
                                  "857\n"
                                  "860\n"
                                  "861\n"
                                  "863\n"
                                  "866, 808\n"
                                  "865\n"
                                  "869";

struct codepageEntryStruct
{
   unsigned char cp;
   unsigned long unicode;
};

#include "cp\cp437.h"
#include "cp\cp737.h"
#include "cp\cp850.h" /* 850, 858 */
#include "cp\cp851.h" /* 851, 869 */
#include "cp\cp852.h"
#include "cp\cp853.h"
#include "cp\cp855.h" /* 855, 872 */
#include "cp\cp857.h"
#include "cp\cp860.h"
#include "cp\cp861.h"
#include "cp\cp863.h"
#include "cp\cp865.h"
#include "cp\cp866.h" /* 866, 808 */

#endif _CPCODES_H_INCLUDED
