/* Windows CE has no <stddef.h>; the Dreamcast CRT shim supplies size_t, NULL,
   offsetof, etc. Vendored zlib includes <stddef.h> from zutil.h, so route it
   here. */
#ifndef _DC_STDDEF_H
#define _DC_STDDEF_H

#include "dreamcast_crt.h"

#endif
