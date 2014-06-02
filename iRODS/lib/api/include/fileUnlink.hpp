/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* fileUnlink.h - This file may be generated by a program or script
 */

#ifndef FILE_UNLINK_HPP
#define FILE_UNLINK_HPP

/* This is a low level file type API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "initServer.hpp"



typedef struct {
    rodsHostAddr_t addr;
    char fileName[MAX_NAME_LEN];
    char rescHier[MAX_NAME_LEN];
    char objPath[MAX_NAME_LEN];
    char in_pdmo[MAX_NAME_LEN];
} fileUnlinkInp_t;

#define fileUnlinkInp_PI "struct RHostAddr_PI; str fileName[MAX_NAME_LEN]; str rescHier[MAX_NAME_LEN]; str objPath[MAX_NAME_LEN]; str in_pdmo[MAX_NAME_LEN];"

#if defined(RODS_SERVER)
#define RS_FILE_UNLINK rsFileUnlink
/* prototype for the server handler */
int
rsFileUnlink( rsComm_t *rsComm, fileUnlinkInp_t *fileUnlinkInp );
int
_rsFileUnlink( rsComm_t *rsComm, fileUnlinkInp_t *fileUnlinkInp );
int
remoteFileUnlink( rsComm_t *rsComm, fileUnlinkInp_t *fileUnlinkInp,
                  rodsServerHost_t *rodsServerHost );
#else
#define RS_FILE_UNLINK NULL
#endif

/* prototype for the client call */
#ifdef __cplusplus
extern "C" {
#endif
int
rcFileUnlink( rcComm_t *conn, fileUnlinkInp_t *fileUnlinkInp );
#ifdef __cplusplus
}
#endif

#endif  /* FILE_UNLINK_H */
