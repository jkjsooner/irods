#include "ruleExecSubmit.hpp"
#include "ruleExecDel.hpp"
#include "reServerLib.hpp"
#include "objMetaOpr.hpp"
#include "icatHighLevelRoutines.hpp"

int
rsRuleExecDel( rsComm_t *rsComm, ruleExecDelInp_t *ruleExecDelInp ) {
    rodsServerHost_t *rodsServerHost;
    int status;

    if ( ruleExecDelInp == NULL ) {
        rodsLog( LOG_NOTICE,
                 "rsRuleExecDel error. NULL input" );
        return SYS_INTERNAL_NULL_INPUT_ERR;
    }

    status = getAndConnReHost( rsComm, &rodsServerHost );
    if ( status < 0 ) {
        return status;
    }

    if ( rodsServerHost->localFlag == LOCAL_HOST ) {
#ifdef RODS_CAT
        status = _rsRuleExecDel( rsComm, ruleExecDelInp );
#else
        rodsLog( LOG_NOTICE,
                 "rsRuleExecDel error. ICAT is not configured on this host" );
        return SYS_NO_RCAT_SERVER_ERR;
#endif
    }
    else {
        status = rcRuleExecDel( rodsServerHost->conn, ruleExecDelInp );
    }
    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "rsRuleExecDel: rcRuleExecDel failed, status = %d",
                 status );
    }
    return status;
}

int
_rsRuleExecDel( rsComm_t *rsComm, ruleExecDelInp_t *ruleExecDelInp ) {
    genQueryOut_t *genQueryOut = NULL;
    int status;
    sqlResult_t *reiFilePath;
    sqlResult_t *ruleUserName;

    char reiDir[MAX_NAME_LEN];

    status = getReInfoById( rsComm, ruleExecDelInp->ruleExecId,
                            &genQueryOut );


    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "_rsRuleExecDel: getReInfoById failed, status = %d",
                 status );
        /* unregister it anyway */
#ifdef RODS_CAT
        status = chlDelRuleExec( rsComm, ruleExecDelInp->ruleExecId );
        if ( status < 0 ) {
            rodsLog( LOG_ERROR,
                     "_rsRuleExecDel: chlDelRuleExec for %s error, status = %d",
                     ruleExecDelInp->ruleExecId, status );
        }
        return status;
#else
        rodsLog( LOG_ERROR,
                 "_rsRuleExecDel: chlDelRuleExec only in ICAT host" );
        return SYS_NO_RCAT_SERVER_ERR;
#endif
    }

    if ( ( reiFilePath = getSqlResultByInx( genQueryOut,
                                            COL_RULE_EXEC_REI_FILE_PATH ) ) == NULL ) {
        rodsLog( LOG_NOTICE,
                 "_rsRuleExecDel: getSqlResultByInx for REI_FILE_PATH failed" );
        return UNMATCHED_KEY_OR_INDEX;
    }

    /* First check permission (now that API is allowed for non-admin users) */
    if ( rsComm->proxyUser.authInfo.authFlag < LOCAL_PRIV_USER_AUTH ) {
        if ( rsComm->proxyUser.authInfo.authFlag == LOCAL_USER_AUTH ) {
            if ( ( ruleUserName = getSqlResultByInx( genQueryOut,
                                  COL_RULE_EXEC_USER_NAME ) ) == NULL ) {
                rodsLog( LOG_NOTICE,
                         "_rsRuleExecDel: getSqlResultByInx for COL_RULE_EXEC_USER_NAME failed" );
                return UNMATCHED_KEY_OR_INDEX;
            }
            if ( strncmp( ruleUserName->value,
                          rsComm->clientUser.userName, MAX_NAME_LEN ) != 0 ) {
                return USER_ACCESS_DENIED;
            }
        }
        else {
            return USER_ACCESS_DENIED;
        }
    }

    /* some sanity check */
    snprintf( reiDir, MAX_NAME_LEN,
              "/%-s/%-s.", PACKED_REI_DIR, REI_FILE_NAME );

    if ( strstr( reiFilePath->value, reiDir ) == NULL ) {
#ifdef RODS_CAT
        int i;
        char errMsg[105];

        rodsLog( LOG_NOTICE,
                 "_rsRuleExecDel: reiFilePath: %s is not a proper rei path",
                 reiFilePath->value );

        /* Try to unregister it anyway */
        status = chlDelRuleExec( rsComm, ruleExecDelInp->ruleExecId );

        if ( status ) {
            return ( status );   /* that failed too, report it */
        }

        /* Add a message to the error stack for the client user */
        snprintf( errMsg, sizeof errMsg, "Rule was removed but reiPath was invalid: %s",
                  reiFilePath->value );
        i = addRErrorMsg( &rsComm->rError, 0, errMsg );
        if ( i < 0 ) {
            irods::log( ERROR( i, "addRErrorMsg failed" ) );
        }
        freeGenQueryOut( &genQueryOut );

        return SYS_INVALID_FILE_PATH;
#else
        rodsLog( LOG_ERROR,
                 "_rsRuleExecDel: chlDelRuleExec only in ICAT host" );
        return SYS_NO_RCAT_SERVER_ERR;
#endif
    }

    status = unlink( reiFilePath->value );
    if ( status < 0 ) {
        status = UNIX_FILE_UNLINK_ERR - errno;
        rodsLog( LOG_ERROR,
                 "_rsRuleExecDel: unlink of %s error, status = %d",
                 reiFilePath->value, status );
        if ( errno != ENOENT ) {
            freeGenQueryOut( &genQueryOut );
            return status;
        }
    }
#ifdef RODS_CAT
    int unlinkStatus = status;

    /* unregister it */
    status = chlDelRuleExec( rsComm, ruleExecDelInp->ruleExecId );

    if ( status < 0 ) {
        rodsLog( LOG_ERROR,
                 "_rsRuleExecDel: chlDelRuleExec for %s error, status = %d",
                 ruleExecDelInp->ruleExecId, status );
    }
    if ( unlinkStatus ) {
        int i;
        char errMsg[105];
        /* Add a message to the error stack for the client user */
        snprintf( errMsg, sizeof errMsg,
                  "Rule was removed but unlink of rei file failed" );
        i = addRErrorMsg( &rsComm->rError, 0, errMsg );
        if ( i < 0 ) {
            irods::log( ERROR( i, "addRErrorMsg failed" ) );
        }
        snprintf( errMsg, sizeof errMsg,
                  "rei file: %s",
                  reiFilePath->value );
        i = addRErrorMsg( &rsComm->rError, 1, errMsg );
        if ( status == 0 ) {
            status = unlinkStatus;
        }  /* return this error if
					          no other error occurred */

    }
    freeGenQueryOut( &genQueryOut );

    return status;
#else
    rodsLog( LOG_ERROR,
             "_rsRuleExecDel: chlDelRuleExec only in ICAT host" );
    return SYS_NO_RCAT_SERVER_ERR;
#endif

}

