/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* fileSyncToArch.h - This file may be generated by a program or script
 */

#ifndef FILE_SYNC_TO_ARCH_HPP
#define FILE_SYNC_TO_ARCH_HPP

/* This is a Object File I/O call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "initServer.hpp"
#include "fileStageToCache.hpp"

#if defined(RODS_SERVER)
#define RS_FILE_SYNC_TO_ARCH rsFileSyncToArch
/* prototype for the server handler */
int
rsFileSyncToArch( rsComm_t *rsComm, fileStageSyncInp_t *fileSyncToArchInp, fileSyncOut_t** );
int
rsFileSyncToArchByHost( rsComm_t *rsComm, fileStageSyncInp_t *fileSyncToArchInp, fileSyncOut_t**,
                        rodsServerHost_t *rodsServerHost );
int
_rsFileSyncToArch( rsComm_t *rsComm, fileStageSyncInp_t *fileSyncToArchInp, fileSyncOut_t** );
int
remoteFileSyncToArch( rsComm_t *rsComm, fileStageSyncInp_t *fileSyncToArchInp, fileSyncOut_t**,
                      rodsServerHost_t *rodsServerHost );
#else
#define RS_FILE_SYNC_TO_ARCH NULL
#endif

/* prototype for the client call */
#ifdef __cplusplus
extern "C" {
#endif
int
rcFileSyncToArch( rcComm_t *conn, fileStageSyncInp_t *fileSyncToArchInp, fileSyncOut_t** );
#ifdef __cplusplus
}
#endif

#endif	/* FILE_SYNC_TO_ARCH_H */
