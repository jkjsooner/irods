/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* fileReaddir.h - This file may be generated by a program or script
 */

#ifndef FILE_READDIR_HPP
#define FILE_READDIR_HPP

/* This is a low level file type API call */

#include "rods.hpp"
#include "rcMisc.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "initServer.hpp"

typedef struct {
    int fileInx;
} fileReaddirInp_t;

#define fileReaddirInp_PI "int fileInx;"

#if defined(RODS_SERVER)
#define RS_FILE_READDIR rsFileReaddir
/* prototype for the server handler */
int
rsFileReaddir( rsComm_t *rsComm, fileReaddirInp_t *fileReaddirInp,
               rodsDirent_t **fileReaddirOut );
int
_rsFileReaddir( rsComm_t *rsComm, fileReaddirInp_t *fileReaddirInp,
                rodsDirent_t **fileReaddirOut );
int
remoteFileReaddir( rsComm_t *rsComm, fileReaddirInp_t *fileReaddirInp,
                   rodsDirent_t **fileReaddirOut, rodsServerHost_t *rodsServerHost );
#else
#define RS_FILE_READDIR NULL
#endif

/* prototype for the client call */
#ifdef __cplusplus
extern "C" {
#endif
int
rcFileReaddir( rcComm_t *conn, fileReaddirInp_t *fileReaddirInp,
               rodsDirent_t **fileReaddirOut );
#ifdef __cplusplus
}
#endif

#endif	/* FILE_READDIR_H */
