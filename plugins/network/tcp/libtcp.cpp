/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

// =-=-=-=-=-=-=-
// irods includes
#include "rodsDef.h"
#include "msParam.h"
#include "reGlobalsExtern.h"
#include "rcConnect.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_network_plugin.h"
#include "eirods_network_constants.h"
#include "eirods_tcp_object.h"
#include "eirods_stacktrace.h"

// =-=-=-=-=-=-=-
// stl includes
#include <sstream>
#include <string>

extern "C" {
    // =-=-=-=-=-=-=-
    // 1. Define plugin Version Variable, used in plugin
    //    creation when the factory function is called.
    //    -- currently only 1.0 is supported.
    double EIRODS_PLUGIN_INTERFACE_VERSION=1.0;
    
    // =-=-=-=-=-=-=-
    // local function to read a buffer from a socket
    eirods::error tcp_socket_read(
        int             _socket, 
        void*           _buffer, 
        int             _length, 
        int&            _bytes_read, 
        struct timeval* _time_value ) {
        // =-=-=-=-=-=-=-
        // Initialize the file descriptor set
        fd_set set;
        FD_ZERO( &set );
        FD_SET( _socket, &set );
        
        // =-=-=-=-=-=-=-
        // local copy of time value? 
        struct timeval timeout;
        if( _time_value != NULL ) { 
            timeout = (*_time_value);
        }

        // =-=-=-=-=-=-=-
        // local working variables 
        int   len_to_read = _length;
        char* read_ptr    = static_cast<char*>( _buffer );

        // =-=-=-=-=-=-=-
        // reset bytes read 
        _bytes_read = 0;

        // =-=-=-=-=-=-=-
        // loop while there is data to read
        while( len_to_read > 0 ) {
            // =-=-=-=-=-=-=-
            // do a time out managed select of the socket fd
            if( 0 != _time_value ) {
                int status = select( _socket + 1, &set, NULL, NULL, &timeout );
                if( status == 0 ) {
                    // =-=-=-=-=-=-=-
                    // the select has timed out 
                    if( ( _length - len_to_read ) > 0 ) {
                        return ERROR( _length - len_to_read, 
                                      "failed to read requested number of bytes" );
                    } else {
                        return ERROR( SYS_SOCK_READ_TIMEDOUT, 
                                      "socket timeout error" );
                    }

                } else if( status < 0 ) {
                    // =-=-=-=-=-=-=-
                    // keep trying on interrupt or just error out
                    if ( errno == EINTR ) {
                        continue;

                    } else {
                        return ERROR( SYS_SOCK_READ_ERR - errno, 
                                      "error on select" );

                    }

                } // else

            } // if tv

            // =-=-=-=-=-=-=-
            // select has been done, finally do the read
            int num_bytes = read( _socket, (void *) read_ptr,  len_to_read );
           
            // =-=-=-=-=-=-=-
            // error trapping the read
            if( num_bytes <= 0 ) {
                // =-=-=-=-=-=-=-
                // gracefully handle an interrupt
                if( EINTR == errno ) {
                    errno     = 0;
                    num_bytes = 0;
                } else {
                    break;
                }
            }

            // =-=-=-=-=-=-=-
            // all has gone well, do byte book keeping
            len_to_read -= num_bytes;
            read_ptr    += num_bytes;
            _bytes_read += num_bytes;
             
        } // while

        // =-=-=-=-=-=-=-
        // and were done? report length not read
        return CODE( _length - len_to_read );
    
    } // tcp_socket_read
    
    // =-=-=-=-=-=-=-
    // local function to write a buffer to a socket
    eirods::error tcp_socket_write(
        int   _socket, 
        void* _buffer, 
        int   _length, 
        int&  _bytes_written ) {
        // =-=-=-=-=-=-=-
        // local variables for write
        int   len_to_write = _length;
        char* write_ptr    = static_cast<char*>( _buffer );

        // =-=-=-=-=-=-=-
        // reset bytes written
        _bytes_written = 0;

        // =-=-=-=-=-=-=-
        // loop while there is data to read
        while( len_to_write > 0 ) {
            int num_bytes = write( _socket, 
                                   static_cast<void*>( write_ptr ), 
                                   len_to_write );
            // =-=-=-=-=-=-=-
            // error trapping the write
            if( num_bytes <= 0 ) {
                // =-=-=-=-=-=-=-
                // gracefully handle an interrupt
                if( errno == EINTR ) {
                    errno     = 0;
                    num_bytes = 0;

                } else {
                    break;
                
                }
            }
            
            // =-=-=-=-=-=-=-
            // increment working variables
            len_to_write   -= num_bytes;
            write_ptr      += num_bytes;
            _bytes_written += num_bytes;

        }

        // =-=-=-=-=-=-=-
        // and were done? report length not written
        return CODE( _length - len_to_write );

    } // tcp_socket_write

    // =-=-=-=-=-=-=-
    // 
    eirods::error tcp_start( 
        eirods::plugin_context& _ctx ) { 
        return SUCCESS();

    } // tcp_start

    // =-=-=-=-=-=-=-
    // 
    eirods::error tcp_end( 
        eirods::plugin_context& _ctx ) { 
        return SUCCESS();

    } // tcp_end

    // =-=-=-=-=-=-=-
    // 
    eirods::error tcp_shutdown( 
        eirods::plugin_context& _ctx ) { 
        return SUCCESS();

    } // tcp_end

    // =-=-=-=-=-=-=-
    // 
    eirods::error tcp_read_msg_header( 
        eirods::plugin_context& _ctx,
        void*                   _buffer,
        struct timeval*         _time_val ) {
        // =-=-=-=-=-=-=-
        // check the context
        eirods::error ret = _ctx.valid< eirods::tcp_object >();
        if( !ret.ok() ) {
            return PASS( ret ); 
        }

        // =-=-=-=-=-=-=-
        // extract the useful bits from the context
        eirods::tcp_object_ptr tcp = boost::dynamic_pointer_cast< eirods::tcp_object >( _ctx.fco() );
        int socket_handle = tcp->socket_handle();

        // =-=-=-=-=-=-=- 
        // read the header length packet */
        int header_length = 0;
        int bytes_read    = 0;
        ret = tcp_socket_read( 
                  socket_handle, 
                  static_cast<void*>( &header_length ), 
                  sizeof( int ), 
                  bytes_read, 
                  _time_val );
        if( !ret.ok() || 
            bytes_read != sizeof( header_length ) ) {

            int status = 0;
            if( bytes_read < 0) {
                status =  bytes_read - errno;
            } else {
                status = SYS_HEADER_READ_LEN_ERR - errno;
            }
            std::stringstream msg;
            msg << "read " 
                << bytes_read
                << " expected " << sizeof( header_length );
            return ERROR( status, msg.str() );
        }

        // =-=-=-=-=-=-=-
        // convert from network to host byte order
        header_length = ntohl( header_length );

        // =-=-=-=-=-=-=-
        // check head length against expected size range
        if( header_length >  MAX_NAME_LEN || 
            header_length <= 0 ) {
            std::stringstream msg;
            msg << "header length is out of range: " 
                << header_length
                << " expected >= 0 and < "
                << MAX_NAME_LEN;
            return ERROR( SYS_HEADER_READ_LEN_ERR, msg.str() );

        }

        // =-=-=-=-=-=-=-
        // now read the actual header
        ret = tcp_socket_read( 
                  socket_handle, 
                  _buffer,
                  header_length,
                  bytes_read, 
                  _time_val );
        if( !ret.ok() || 
            bytes_read != header_length) {
            int status = 0;
            if (bytes_read < 0) {
                status = bytes_read - errno;
            } else {
                status = SYS_HEADER_READ_LEN_ERR - errno;
            }
            std::stringstream msg;
            msg << "read " 
                << bytes_read
                << " expected " << header_length;
            return ERROR( status, msg.str() );

        }

        // =-=-=-=-=-=-=-
        // log debug information if appropriate
        if( getRodsLogLevel () >= LOG_DEBUG3 ) {
            printf( "received header: len = %d\n%s\n", 
                    header_length, 
                    static_cast<char*>( _buffer ) );
        }

        return SUCCESS();

    } // tcp_read_msg_header

    // =-=-=-=-=-=-=-
    // 
    eirods::error tcp_write_msg_header( 
        eirods::plugin_context& _ctx,
        bytesBuf_t*             _header ) {
        // =-=-=-=-=-=-=-
        // check the context
        eirods::error ret = _ctx.valid< eirods::tcp_object >();
        if( !ret.ok() ) {
            return PASS( ret ); 
        }

        // =-=-=-=-=-=-=-
        // log debug information if appropriate
        if( getRodsLogLevel () >= LOG_DEBUG3 ) {
            printf( "sending header: len = %d\n%s\n", 
                    _header->len, 
                    (char *) _header->buf);
        }
        
        // =-=-=-=-=-=-=-
        // extract the useful bits from the context
        eirods::tcp_object_ptr tcp = boost::dynamic_pointer_cast< eirods::tcp_object >( _ctx.fco() );
        int socket_handle = tcp->socket_handle();

        // =-=-=-=-=-=-=-
        // convert host byte order to network byte order
        int header_length = htonl( _header->len );

        // =-=-=-=-=-=-=-
        // write the length of the header to the socket
        int bytes_written = 0;
        ret = tcp_socket_write( 
                  socket_handle,  
                  &header_length,
                  sizeof( header_length ),
                  bytes_written );       
        if( !ret.ok() ||
            bytes_written != sizeof( header_length ) ) {
            std::stringstream msg;
            msg << "wrote " 
                << bytes_written
                << " expected " << header_length;
            return ERROR( SYS_HEADER_WRITE_LEN_ERR - errno, 
                          msg.str() );
        }

        // =-=-=-=-=-=-=-
        // now send the actual header
        ret = tcp_socket_write( 
                  socket_handle,  
                  _header->buf,
                  _header->len,
                  bytes_written );       
        if( !ret.ok() ||
            bytes_written != _header->len ) {
            std::stringstream msg;
            msg << "wrote " 
                << bytes_written
                << " expected " << _header->len;
            return ERROR( SYS_HEADER_WRITE_LEN_ERR - errno, 
                          msg.str() );
        }       

        return SUCCESS();

    } // tcp_write_msg_header

    // =-=-=-=-=-=-=-
    // 
    eirods::error tcp_send_rods_msg( 
        eirods::plugin_context& _ctx,
        char*                   _msg_type,
        bytesBuf_t*             _msg_buf,
        bytesBuf_t*             _stream_bbuf,
        bytesBuf_t*             _error_buf,
        int                     _int_info,
        irodsProt_t             _protocol ) {
        // =-=-=-=-=-=-=-
        // check the context
        eirods::error ret = _ctx.valid< eirods::tcp_object >();
        if( !ret.ok() ) {
            return PASS( ret ); 
        }
        
        // =-=-=-=-=-=-=-
        // check the params
        if( !_msg_type ) {
            return ERROR( SYS_INVALID_INPUT_PARAM, "null msg type" );
        }


        // =-=-=-=-=-=-=-
        // extract the useful bits from the context
        eirods::tcp_object_ptr tcp = boost::dynamic_pointer_cast< eirods::tcp_object >( _ctx.fco() );
        int socket_handle = tcp->socket_handle();

        // =-=-=-=-=-=-=-
        // initialize a new header
        msgHeader_t msg_header;
        memset( &msg_header, 0, sizeof( msg_header ) ); 
        
        strncpy( msg_header.type, _msg_type, HEADER_TYPE_LEN );  
        msg_header.intInfo = _int_info;        

        // =-=-=-=-=-=-=-
        // initialize buffer lengths
        if( _msg_buf ) {
            msg_header.msgLen = _msg_buf->len; 
        }
        if( _stream_bbuf ) {
            msg_header.bsLen = _stream_bbuf->len; 
        }
        if( _error_buf ) {
            msg_header.errorLen = _error_buf->len; 
        }
        
        // =-=-=-=-=-=-=-
        // send the header
        ret = writeMsgHeader( 
                  tcp,
                  &msg_header ); 
        if( !ret.ok() ) {
           return PASSMSG( "writeMsgHeader failed", ret ); 
        }
        
        // =-=-=-=-=-=-=-
        // send the message buffer
        int bytes_written = 0;
        if( msg_header.msgLen > 0 ) {
            if( XML_PROT == _protocol &&
                getRodsLogLevel() >= LOG_DEBUG3 ) {
                printf ("sending msg: \n%s\n", (char*) _msg_buf->buf );
            }
            ret = tcp_socket_write( 
                socket_handle,  
                _msg_buf->buf,
                _msg_buf->len,
                bytes_written );       
            if( !ret.ok() ) {
                return PASS( ret );
            }

        } // if msgLen > 0
 
        // =-=-=-=-=-=-=-
        // send the error buffer 
        if( msg_header.errorLen > 0 ) {
            if( XML_PROT == _protocol &&
                getRodsLogLevel() >= LOG_DEBUG3 ) {
                printf ("sending msg: \n%s\n", (char*) _error_buf->buf );

            }
         
            ret = tcp_socket_write( 
                socket_handle,  
                _error_buf->buf,
                _error_buf->len,
                bytes_written );       
            if( !ret.ok() ) {
                return PASS( ret );
            }

        } // if errorLen > 0 
  
        // =-=-=-=-=-=-=-
        // send the stream buffer 
        if( msg_header.bsLen > 0 ) {
            if( XML_PROT == _protocol &&
                getRodsLogLevel() >= LOG_DEBUG3 ) {
                printf ("sending msg: \n%s\n", (char*) _stream_bbuf->buf );
            }
         
            ret = tcp_socket_write( 
                socket_handle,  
                _stream_bbuf->buf,
                _stream_bbuf->len,
                bytes_written );       
            if( !ret.ok() ) {
                return PASS( ret );
            }

        } // if bsLen > 0 
              
        return SUCCESS();

    } // tcp_send_rods_msg
    
    // =-=-=-=-=-=-=-
    // helper fcn to read a bytes buf
    eirods::error read_bytes_buf( 
        int             _socket_handle,
        int             _length,
        bytesBuf_t*     _buffer,
        irodsProt_t     _protocol, 
        struct timeval* _time_val ) { 
        // =-=-=-=-=-=-=-
        // trap input buffer ptr
        if( !_buffer ) {
            return ERROR( SYS_READ_MSG_BODY_INPUT_ERR, 
                          "null buffer ptr" );
        }
        
        // =-=-=-=-=-=-=-
        // allocate and read buffer
        int bytes_read = 0;
        _buffer->buf = malloc( _length );
        eirods::error ret = tcp_socket_read( 
                                _socket_handle, 
                                _buffer->buf,
                                _length,
                                bytes_read, 
                                _time_val );
        _buffer->len = bytes_read;
         
        // =-=-=-=-=-=-=-
        // log transaction if requested
        if( _protocol == XML_PROT && 
            getRodsLogLevel() >= LOG_DEBUG3 ) {
            printf( "received msg: \n%s\n", 
                     (char*) _buffer->buf );
        }

        // =-=-=-=-=-=-=-
        // trap failed read
        if( !ret.ok() || 
            bytes_read != _length ) {
            
            free( _buffer->buf );

            std::stringstream msg;
            msg << "read " 
                << bytes_read
                << " expected " << _length;
            return ERROR( SYS_READ_MSG_BODY_LEN_ERR - errno, 
                          msg.str() );

        }

        return SUCCESS();

    } // read_bytes_buf

    // =-=-=-=-=-=-=-
    // read a message body off of the socket
    eirods::error tcp_read_msg_body( 
        eirods::plugin_context& _ctx,
        msgHeader_t*            _header,
        bytesBuf_t*             _input_struct_buf,
        bytesBuf_t*             _bs_buf,
        bytesBuf_t*             _error_buf,
        irodsProt_t             _protocol,
        struct timeval*         _time_val ) { 
        // =-=-=-=-=-=-=-
        // check the context
        eirods::error ret = _ctx.valid< eirods::tcp_object >();
        if( !ret.ok() ) {
            return PASS( ret ); 
        }

        // =-=-=-=-=-=-=-
        // extract the useful bits from the context
        eirods::tcp_object_ptr tcp = boost::dynamic_pointer_cast< eirods::tcp_object >( _ctx.fco() );
        int socket_handle = tcp->socket_handle();

        // =-=-=-=-=-=-=-
        // trap header ptr
        if( !_header ) {
            return ERROR( SYS_READ_MSG_BODY_INPUT_ERR, 
                          "null header ptr" );
        }

        // =-=-=-=-=-=-=-
        // reset error buf
        // NOTE :: do not reset bs buf as it can be reused 
        //         on the client side
        if( _error_buf ) {
            memset( _error_buf, 0, sizeof( bytesBuf_t ) );
        }

        // =-=-=-=-=-=-=-
        // read input buffer
        if(  0 != _input_struct_buf ) {
            if( _header->msgLen > 0 ) {
                ret = read_bytes_buf( 
                          socket_handle,
                          _header->msgLen,
                          _input_struct_buf,
                          _protocol,
                          _time_val );
                if( !ret.ok() ) {
                    return PASS( ret );
                }
    
            } else {
                // =-=-=-=-=-=-=-
                // ensure msg len is 0 as this can cause issues
                // in the agent
                _input_struct_buf->len = 0;

            }

        } // input buffer

        // =-=-=-=-=-=-=-
        // read error buffer
        if(  0 != _error_buf ) {
            if( _header->errorLen > 0 ) {
                ret = read_bytes_buf( 
                          socket_handle,
                          _header->errorLen,
                          _error_buf,
                          _protocol,
                          _time_val );
                if( !ret.ok() ) {
                    return PASS( ret );
                }
            } else {
                _error_buf->len = 0;

            }

        } // error buffer

        // =-=-=-=-=-=-=-
        // read bs buffer
        if( 0 != _bs_buf ) {
            if( _header->bsLen > 0 ) {
                ret = read_bytes_buf( 
                          socket_handle,
                          _header->bsLen,
                          _bs_buf,
                          _protocol,
                          _time_val );
                if( !ret.ok() ) {
                    return PASS( ret );
                }
            } else {
                _bs_buf->len = 0;
            
            }

        } // bs buffer

        return SUCCESS();

    } // tcp_read_msg_body

    // =-=-=-=-=-=-=-
    // stub for ops that the tcp plug does 
    // not need to support - accept etc
    eirods::error tcp_success_stub( 
        eirods::plugin_context& _ctx ) {
        return SUCCESS();

    } // tcp_success_stub


    // =-=-=-=-=-=-=-
    // derive a new tcp network plugin from
    // the network plugin base class for handling
    // tcp communications
    class tcp_network_plugin : public eirods::network {
    public:
        tcp_network_plugin( 
           const std::string& _nm, 
           const std::string& _ctx ) :
               eirods::network( 
                   _nm, 
                   _ctx ) {
        } // ctor

        ~tcp_network_plugin() {
        }

    }; // class tcp_network_plugin



    // =-=-=-=-=-=-=-
    // factory function to provide instance of the plugin
    eirods::network* plugin_factory( 
        const std::string& _inst_name, 
        const std::string& _context ) {
        // =-=-=-=-=-=-=-
        // create a tcp network object
        tcp_network_plugin* tcp = new tcp_network_plugin( 
                                          _inst_name,
                                          _context );
        if( !tcp ) {
            rodsLog( LOG_ERROR, "plugin_factory - failed to alloc tcp_network_plugin" );
            return 0;
        }
        
        // =-=-=-=-=-=-=-
        // fill in the operation table mapping call 
        // names to function names
        tcp->add_operation( eirods::NETWORK_OP_CLIENT_START, "tcp_success_stub"     );
        tcp->add_operation( eirods::NETWORK_OP_CLIENT_STOP,  "tcp_success_stub"     );
        tcp->add_operation( eirods::NETWORK_OP_AGENT_START,  "tcp_success_stub"     );
        tcp->add_operation( eirods::NETWORK_OP_AGENT_STOP,   "tcp_success_stub"     );
        tcp->add_operation( eirods::NETWORK_OP_READ_HEADER,  "tcp_read_msg_header"  );
        tcp->add_operation( eirods::NETWORK_OP_READ_BODY,    "tcp_read_msg_body"    );
        tcp->add_operation( eirods::NETWORK_OP_WRITE_HEADER, "tcp_write_msg_header" );
        tcp->add_operation( eirods::NETWORK_OP_WRITE_BODY,   "tcp_send_rods_msg"    );

        eirods::network* net = dynamic_cast< eirods::network* >( tcp );
        if( !net ) {
            rodsLog( LOG_ERROR, "failed to dynamic cast to eirods::network*" );
        }

        return net;

    } // plugin_factory

}; // extern "C"
































