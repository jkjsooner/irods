


// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_auth_factory.h"
#include "eirods_native_auth_object.h"

namespace eirods {
   /// =-=-=-=-=-=-=-
   /// @brief super basic free factory function to create an auth object
   ///        given the requested authentication scheme
   error auth_factory( 
       const std::string& _scheme,
       rError_t*          _r_error,
       auth_object_ptr&   _ptr ) {
       // =-=-=-=-=-=-=-
       // currently just support the native scheme
       if( _scheme.empty() ||
           AUTH_NATIVE_SCHEME == _scheme ) {
           native_auth_object* nat_obj = new native_auth_object( _r_error );
           if( !nat_obj ) {
               return ERROR( 
                          SYS_INVALID_INPUT_PARAM, 
                          "native auth allocation failed" );
           }

           auth_object* auth_obj = dynamic_cast< auth_object* >( nat_obj );
           if( !auth_obj ) {
               return ERROR( 
                          SYS_INVALID_INPUT_PARAM, 
                          "native auth dynamic cast failed" );
           }
           
           _ptr.reset( auth_obj );

       } else {
           std::string msg( "auth scheme not supported [" );
           msg += _scheme + "]";
           return ERROR( 
                      SYS_INVALID_INPUT_PARAM, 
                      msg );
       }

       return SUCCESS();

   } // auth_factory

}; // namespace eirods


