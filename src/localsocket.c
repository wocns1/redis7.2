/*
 * Copyright (c) 2019, Redis Labs
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *   * Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *   * Neither the name of Redis nor the names of its contributors may be used
 *     to endorse or promote products derived from this software without
 *     specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "server.h"
#include "connhelpers.h"

#ifndef LSOCKET
#define LSOCKET 1
ConnectionType CT_LSocket;
#endif
/* The connections module provides a lean abstraction of network connections
 * to avoid direct socket and async event management across the Redis code base.
 *
 * It does NOT provide advanced connection features commonly found in similar
 * libraries such as complete in/out buffer management, throttling, etc. These
 * functions remain in networking.c.
 *
 * The primary goal is to allow transparent handling of TCP and TLS based
 * connections. To do so, connections have the following properties:
 *
 * 1. A connection may live before its corresponding socket exists.  This
 *    allows various context and configuration setting to be handled before
 *    establishing the actual connection.
 * 2. The caller may register/unregister logical read/write handlers to be
 *    called when the connection has data to read from/can accept writes.
 *    These logical handlers may or may not correspond to actual AE events,
 *    depending on the implementation (for TCP they are; for TLS they aren't).
 */

//static ConnectionType CT_LSocket;

/* When a connection is created we must know its type already, but the
 * underlying socket may or may not exist:
 *
 * - For accepted connections, it exists as we do not model the listen/accept
 *   part; So caller calls connCreateSocket() followed by connAccept().
 * - For outgoing connections, the socket is created by the connection module
 *   itself; So caller calls connCreateSocket() followed by connConnect(),
 *   which registers a connect callback that fires on connected/error state
 *   (and after any transport level handshake was done).
 *
 * NOTE: An earlier version relied on connections being part of other structs
 * and not independently allocated. This could lead to further optimizations
 * like using container_of(), etc.  However it was discontinued in favor of
 * this approach for these reasons:
 *
 * 1. In some cases conns are created/handled outside the context of the
 * containing struct, in which case it gets a bit awkward to copy them.
 * 2. Future implementations may wish to allocate arbitrary data for the
 * connection.
 * 3. The container_of() approach is anyway risky because connections may
 * be embedded in different structs, not just client.
 */

ConnectionType *connTypeOfLocalSocket(void) {
        return connectionTypeLS();
}

connection *connCreateLSocket(void) {
    connection *conn = zcalloc(sizeof(connection));
    conn->type = &CT_LSocket;
    conn->fd = -1;

    return conn;
}

/* Create a new socket-type connection that is already associated with
 
 *
 * The socket is not ready for I/O until connAccept() was called and
 * invoked the connection-level accept handler.
 *
 * Callers should use connGetState() and verify the created connection
 * is not in an error state (which is not possible for a socket connection,
 * but could but possible with other protocols).
 */
static connection *connCreateAcceptedLSocket(int fd, void *priv) {
    UNUSED(priv);
    connection *conn = connCreateLSocket();
    conn->fd = fd;
    conn->state = CONN_STATE_ACCEPTING;
    return conn;
}

static int connLocalSocketConnect(connection *conn, const char *addr, int port, const char *src_addr,
        ConnectionCallbackFunc connect_handler) {
    return 0;
    /*
    int fd = anetTcpNonBlockBestEffortBindConnect(NULL,addr,port,src_addr);
    if (fd == -1) {
        conn->state = CONN_STATE_ERROR;
        conn->last_errno = errno;
        return C_ERR;
    }

    conn->fd = fd;
    conn->state = CONN_STATE_CONNECTING;

    conn->conn_handler = connect_handler;
    aeCreateFileEvent(server.el, conn->fd, AE_WRITABLE,
            conn->type->ae_handler, conn);

    return C_OK;
*/
}

/* ------ Pure socket connections ------- */

/* A very incomplete list of implementation-specific calls.  Much of the above shall
 * move here as we implement additional connection types.
 */

static void connLocalSocketShutdown(connection *conn) {
    return;
    if (conn->fd == -1) return;

    //shutdown(conn->fd, SHUT_RDWR);
}

/* Close the connection and free resources. */
static void connLocalSocketClose(connection *conn)
{
    return;
#if 0 
    if (conn->fd != -1) {
        aeDeleteFileEvent(server.el,conn->fd, AE_READABLE | AE_WRITABLE);
        close(conn->fd);
        conn->fd = -1;
    }

    /* If called from within a handler, schedule the close but
     * keep the connection until the handler returns.
     */
    if (connHasRefs(conn))
    {
        conn->flags |= CONN_FLAG_CLOSE_SCHEDULED;
        return;
    }

    zfree(conn);
#endif
}

static int connLocalSocketWritev(connection *conn, const void *data, size_t data_len) {
    //serverLog(LL_VERBOSE,"conn %p write data %s len :%ld", conn, (char*)data, data_len);
#if 0
    int ret = write(conn->fd, data, data_len);
    if (ret < 0 && errno != EAGAIN) {
        conn->last_errno = errno;

        /* Don't overwrite the state of a connection that is not already
         * connected, not to mess with handler callbacks.
         */
        if (errno != EINTR && conn->state == CONN_STATE_CONNECTED)
            conn->state = CONN_STATE_ERROR;
    }
#endif
    return 0;
}
static int connLocalSocketWrite(connection *conn, const void *data, size_t data_len) {
    //serverLog(LL_VERBOSE,"conn %p write data %s len :%ld", conn, (char*)data, data_len);
#if 0
    int ret = write(conn->fd, data, data_len);
    if (ret < 0 && errno != EAGAIN) {
        conn->last_errno = errno;

        /* Don't overwrite the state of a connection that is not already
         * connected, not to mess with handler callbacks.
         */
        if (errno != EINTR && conn->state == CONN_STATE_CONNECTED)
            conn->state = CONN_STATE_ERROR;
    }
#endif
    return 0;
}

static int connLocalSocketRead(connection *conn, void *buf, size_t buf_len) {
    int ret = 0;
    if (server.op == 0) {
        ret = snprintf(buf, buf_len, "*3\r\n$3\r\nSET\r\n$128\r\nwo-%0125llu\r\n$128\r\n%0128llu\r\n", server.el->wcnt, server.el->wcnt);
    }
    else {
        ret = snprintf(buf, buf_len, "*2\r\n$3\r\nGET\r\n$128\r\nwo-%0125llu\r\n", server.el->wcnt);
    }
    //Whileone
    //serverLog(LL_VERBOSE, "buf %s\n", buf);
    //int ret = snprintf(buf, buf_len, "%s", inarray[conn->count]);
    if (!ret) {
        conn->state = CONN_STATE_CLOSED;
    } else if (ret < 0 && errno != EAGAIN) {
        conn->last_errno = errno;

        /* Don't overwrite the state of a connection that is not already
         * connected, not to mess with handler callbacks.
         */
        if (errno != EINTR && conn->state == CONN_STATE_CONNECTED)
            conn->state = CONN_STATE_ERROR;
    }

    return ret;
}

static int connLocalSocketAccept(connection *conn, ConnectionCallbackFunc accept_handler) {
    int ret = C_OK;

    if (conn->state != CONN_STATE_ACCEPTING) return C_ERR;
    conn->state = CONN_STATE_CONNECTED;

    connIncrRefs(conn);
    if (!callHandler(conn, accept_handler)) ret = C_ERR;
    connDecrRefs(conn);

    return ret;
}

/* Register a write handler, to be called when the connection is writable.
 * If NULL, the existing handler is removed.
 *
 * The barrier flag indicates a write barrier is requested, resulting with
 * CONN_FLAG_WRITE_BARRIER set. This will ensure that the write handler is
 * always called before and not after the read handler in a single event
 * loop.
 */
static int connLocalSocketSetWriteHandler(connection *conn, ConnectionCallbackFunc func, int barrier) {
    if (func == conn->write_handler) return C_OK;

    conn->write_handler = func;
    if (barrier)
        conn->flags |= CONN_FLAG_WRITE_BARRIER;
    else
        conn->flags &= ~CONN_FLAG_WRITE_BARRIER;
    if (!conn->write_handler)
        aeDeleteFileEvent(server.el,conn->fd,AE_WRITABLE);
    else
        if (aeCreateFileEvent(server.el,conn->fd,AE_WRITABLE,
                    conn->type->ae_handler,conn) == AE_ERR) return C_ERR;
    return C_OK;
}

/* Register a read handler, to be called when the connection is readable.
 * If NULL, the existing handler is removed.
 */
static int connLocalSocketSetReadHandler(connection *conn, ConnectionCallbackFunc func) {
    if (func == conn->read_handler) return C_OK;

    conn->read_handler = func;
    if (!conn->read_handler)
        aeDeleteFileEvent(server.el,conn->fd,AE_READABLE);
    else
        if (aeCreateFileEvent(server.el,conn->fd,
                    AE_READABLE,conn->type->ae_handler,conn) == AE_ERR) return C_ERR;
    return C_OK;
}

static const char *connLocalSocketGetLastError(connection *conn) {
    return strerror(conn->last_errno);
}

static void connLocalSocketEventHandler(struct aeEventLoop *el, int fd, void *clientData, int mask)
{
    UNUSED(el);
    UNUSED(fd);
    connection *conn = clientData;

    if (conn->state == CONN_STATE_CONNECTING &&
            (mask & AE_WRITABLE) && conn->conn_handler) {

        int conn_error = anetGetError(conn->fd);
        if (conn_error) {
            conn->last_errno = conn_error;
            conn->state = CONN_STATE_ERROR;
        } else {
            conn->state = CONN_STATE_CONNECTED;
        }

        if (!conn->write_handler) aeDeleteFileEvent(server.el,conn->fd,AE_WRITABLE);

        if (!callHandler(conn, conn->conn_handler)) return;
        conn->conn_handler = NULL;
    }

    /* Normally we execute the readable event first, and the writable
     * event later. This is useful as sometimes we may be able
     * to serve the reply of a query immediately after processing the
     * query.
     *
     * However if WRITE_BARRIER is set in the mask, our application is
     * asking us to do the reverse: never fire the writable event
     * after the readable. In such a case, we invert the calls.
     * This is useful when, for instance, we want to do things
     * in the beforeSleep() hook, like fsync'ing a file to disk,
     * before replying to a client. */
    int invert = conn->flags & CONN_FLAG_WRITE_BARRIER;

    int call_write = (mask & AE_WRITABLE) && conn->write_handler;
    int call_read = (mask & AE_READABLE) && conn->read_handler;

    /* Handle normal I/O flows */
    if (!invert && call_read) {
        if (!callHandler(conn, conn->read_handler)) return;
    }
    /* Fire the writable event. */
    if (call_write) {
        if (!callHandler(conn, conn->write_handler)) return;
    }
    /* If we have to invert the call, fire the readable event now
     * after the writable one. */
    if (invert && call_read) {
        if (!callHandler(conn, conn->read_handler)) return;
    }
}

static void connLocalSocketAcceptHandler(aeEventLoop *el, int fd, void *privdata, int mask) {
#if 1
    int cport, cfd, max = MAX_ACCEPTS_PER_CALL;
    char cip[NET_IP_STR_LEN];
    UNUSED(el);
    UNUSED(mask);
    UNUSED(privdata);

    while(max--) {
//        cfd = anetTcpAccept(server.neterr, fd, cip, sizeof(cip), &cport);
        cfd = 0;
        if (cfd == ANET_ERR) {
            if (errno != EWOULDBLOCK)
                serverLog(LL_WARNING,
                    "Accepting client connection: %s", server.neterr);
            return;
        }
        serverLog(LL_VERBOSE,"Accepted %s:%d", cip, cport);
        acceptCommonHandler(connCreateAcceptedLSocket(cfd, NULL),0,cip);
    }
#endif
}

static int connLocalSocketAddr(connection *conn, char *ip, size_t ip_len, int *port, int remote) {
    if (anetFdToString(conn->fd, ip, ip_len, port, remote) == 0)
        return C_OK;

    conn->last_errno = errno;
    return C_ERR;
}

static int connLocalSocketIsLocal(connection *conn) {
    char cip[NET_IP_STR_LEN + 1] = { 0 };

    if (connLocalSocketAddr(conn, cip, sizeof(cip) - 1, NULL, 1) == C_ERR)
        return -1;

    return !strncmp(cip, "127.", 4) || !strcmp(cip, "::1");
}

static int connLocalSocketListen(connListener *listener) {
    return listenToPort(listener);
    return 0;
}

static int connLocalSocketBlockingConnect(connection *conn, const char *addr, int port, long long timeout) {
    int fd = anetTcpNonBlockConnect(NULL,addr,port);
    if (fd == -1) {
        conn->state = CONN_STATE_ERROR;
        conn->last_errno = errno;
        return C_ERR;
    }

    if ((aeWait(fd, AE_WRITABLE, timeout) & AE_WRITABLE) == 0) {
        conn->state = CONN_STATE_ERROR;
        conn->last_errno = ETIMEDOUT;
    }

    conn->fd = fd;
    conn->state = CONN_STATE_CONNECTED;
    return C_OK;
}

/* Connection-based versions of syncio.c functions.
 * NOTE: This should ideally be refactored out in favor of pure async work.
 */

static ssize_t connLocalSocketSyncWrite(connection *conn, char *ptr, ssize_t size, long long timeout) {
    return syncWrite(conn->fd, ptr, size, timeout);
}

static ssize_t connLocalSocketSyncRead(connection *conn, char *ptr, ssize_t size, long long timeout) {
    return syncRead(conn->fd, ptr, size, timeout);
}

static ssize_t connLocalSocketSyncReadLine(connection *conn, char *ptr, ssize_t size, long long timeout) {
    return syncReadLine(conn->fd, ptr, size, timeout);
}

static const char *connLocalSocketGetType(connection *conn) {
    (void) conn;

    return CONN_TYPE_LOCAL;
}

ConnectionType CT_LSocket = {
    /* connection type */
    .get_type = connLocalSocketGetType,

    /* connection type initialize & finalize & configure */
    .init = NULL,
    .cleanup = NULL,
    .configure = NULL,

    /* ae & accept & listen & error & address handler */
    .ae_handler = connLocalSocketEventHandler,
    .accept_handler = connLocalSocketAcceptHandler,
    .addr = connLocalSocketAddr,
    .is_local = connLocalSocketIsLocal,
    .listen = connLocalSocketListen,

    /* create/shutdown/close connection */
    .conn_create = connCreateLSocket,
    .conn_create_accepted = connCreateAcceptedLSocket,
    .shutdown = connLocalSocketShutdown,
    .close = connLocalSocketClose,

    /* connect & accept */
    .connect = connLocalSocketConnect,
    .blocking_connect = connLocalSocketBlockingConnect,
    .accept = connLocalSocketAccept,

    /* IO */
    .write = connLocalSocketWrite,
    .writev = connLocalSocketWritev,
    .read = connLocalSocketRead,
    .set_write_handler = connLocalSocketSetWriteHandler,
    .set_read_handler = connLocalSocketSetReadHandler,
    .get_last_error = connLocalSocketGetLastError,
    .sync_write = connLocalSocketSyncWrite,
    .sync_read = connLocalSocketSyncRead,
    .sync_readline = connLocalSocketSyncReadLine,

    /* pending data */
    .has_pending_data = NULL,
    .process_pending_data = NULL,
};

int connBlock(connection *conn) {
    return 0;
    if (conn->fd == -1) return C_ERR;
    return anetBlock(NULL, conn->fd);
}

int connNonBlock(connection *conn) {
    return 0;
    if (conn->fd == -1) return C_ERR;
    return anetNonBlock(NULL, conn->fd);
}

int connEnableTcpNoDelay(connection *conn) {
    return 0;
    if (conn->fd == -1) return C_ERR;
    return anetEnableTcpNoDelay(NULL, conn->fd);
}

int connDisableTcpNoDelay(connection *conn) {
    return 0;
    if (conn->fd == -1) return C_ERR;
    return anetDisableTcpNoDelay(NULL, conn->fd);
}

int connKeepAlive(connection *conn, int interval) {
    return 0;
    if (conn->fd == -1) return C_ERR;
    return anetKeepAlive(NULL, conn->fd, interval);
}

int connSendTimeout(connection *conn, long long ms) {
    return 0;
    return anetSendTimeout(NULL, conn->fd, ms);
}

int connRecvTimeout(connection *conn, long long ms) {
    return 0;
    return anetRecvTimeout(NULL, conn->fd, ms);
}
int RedisRegisterConnectionTypeLSocket(void)
{
    return connTypeRegister(&CT_LSocket);
}
