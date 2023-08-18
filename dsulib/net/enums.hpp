#pragma once
#include <sys/socket.h>

namespace sns {
    enum class msg_flags : int {
        NONE = 0,
        OUT_OF_BAND = MSG_OOB,
        PEEK = MSG_PEEK,
        DONT_ROUTE = MSG_DONTROUTE,
        DONT_WAIT = MSG_DONTWAIT
    };

    inline msg_flags operator|(msg_flags a, msg_flags b)
    {
        return static_cast<msg_flags>(static_cast<int>(a) | static_cast<int>(b));
    }
    inline msg_flags operator&(msg_flags a, msg_flags b)
    {
        return static_cast<msg_flags>(static_cast<int>(a) & static_cast<int>(b));
    }

    enum class option_name {
#ifdef SO_REUSEADDR
        REUSE_ADDRESS = SO_REUSEADDR,
#endif
#ifdef SO_KEEPALIVE
        KEEPALIVE = SO_KEEPALIVE,
#endif
#ifdef SO_BROADCAST
        BROADCAST = SO_BROADCAST,
#endif
#ifdef SO_LINGER
        LINGER = SO_LINGER,
#endif
#ifdef SO_OOBINLINE
        OUT_OF_BAND_DATA_INLINE = SO_OOBINLINE,
#endif
#ifdef SO_SNDBUF
        SEND_BUFFER_SIZE = SO_SNDBUF,
#endif
#ifdef SO_RCVBUF
        RECEIVE_BUFFER_SIZE = SO_RCVBUF,
#endif
#ifdef SO_SNDLOWAT
        SEND_LOW_AT = SO_SNDLOWAT,
#endif
#ifdef SO_RCVLOWAT
        RECEIVE_LOW_AT = SO_RCVLOWAT,
#endif
#ifdef SO_TYPE
        TYPE = SO_TYPE,
#endif
#ifdef SO_ERROR
        ERROR = SO_ERROR,
#endif
#ifdef SO_RXDATA
        RX_DATA = SO_RXDATA,
#endif
#ifdef SO_TXDATA
        TX_DATA = SO_TXDATA,
#endif
#ifdef SO_NBIO
        SET_NON_BLOCKING = SO_NBIO,
#endif
#ifdef SO_BIO
        SET_BLOCKING_IO = SO_BIO,
#endif
#ifdef SO_NONBLOCK
        NON_BLOCK = SO_NONBLOCK
#endif
    };
    enum class shutdown_type {
        READ = SHUT_RD,
        WRITE = SHUT_WR,
        READ_WRITE = SHUT_RDWR
    };




}