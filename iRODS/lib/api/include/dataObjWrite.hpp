/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* dataObjWrite.h - This dataObj may be generated by a program or script
 */

#ifndef DATA_OBJ_WRITE_HPP
#define DATA_OBJ_WRITE_HPP

/* This is a high level type API call */

#include "rods.hpp"
#include "procApiRequest.hpp"
#include "apiNumber.hpp"
#include "initServer.hpp"
#include "fileWrite.hpp"

#if defined(RODS_SERVER)
#define RS_DATA_OBJ_WRITE rsDataObjWrite
/* prototype for the server handler */
int
rsDataObjWrite( rsComm_t *rsComm, openedDataObjInp_t *dataObjWriteInp,
                bytesBuf_t *dataObjWriteInpBBuf );
int
l3Write( rsComm_t *rsComm, int l1descInx, int len,
         bytesBuf_t *dataObjWriteInpBBuf );
int
_l3Write( rsComm_t *rsComm, int destRescTypeInx, int l3descInx,
          void *buf, int len );
#else
#define RS_DATA_OBJ_WRITE NULL
#endif

#ifdef __cplusplus
extern "C" {
#endif
    /* prototype for the client call */
    /* rcDataObjWrite - Write the content of dataObjWriteInpBBuf to
     * an opened iRODS data object descriptor.
     * Input -
     *   rcComm_t *conn - The client connection handle.
     *   dataObjWriteInp_t *dataObjWriteInp - Relevant items are:
     *      l1descInx - the iRODS data object descriptor to write.
     *      len - the number of bytes to write
     *   bytesBuf_t *dataObjWriteInpBBuf - the bytesBuf for the write input.
     *
     * OutPut -
     *   int status of the operation - >= 0 ==> success, < 0 ==> failure.
     */

    int
    rcDataObjWrite( rcComm_t *conn, openedDataObjInp_t *dataObjWriteInp,
                    bytesBuf_t *dataObjWriteInpBBuf );

#ifdef __cplusplus
}
#endif
#endif	/* DATA_OBJ_WRITE_H */
