#include <pthread.h>
#include "lwip/tcpip.h"
#include "lwip/opt.h"
#include "lwip/sockets.h"
#include "lwip/api.h"


#define READ_TASKS 2
#define CHECK_ERROR(ret, function)  do {                        \
        printf("%s returns %d\n", function, ret);               \
        if (ret < 0) {  printf("error number %d\n", errno);     \
                        exit(1);  }                             \
        } while(0)


static void *
reader_thread(void *arg)
{
    char rxbuf[4];
    int srv = (uintptr_t)arg;
    int len = 0;
    while (len != -1 && errno != EAGAIN) {
        len = lwip_recv(srv, rxbuf, sizeof(rxbuf), 0);
        if (len > 0) {
            printf("read(%d),", len);
        }
    }
    printf("Thread done %d\n", errno);
    return NULL;

}

static void *
server_thread(void *arg)
{
    int srv;
    int i;
    int listener = (uintptr_t)arg;
    pthread_t readers[READ_TASKS];

    int ret;
    struct sockaddr_storage source_addr;
    socklen_t addr_len = sizeof(source_addr);

    /* accept the connection */
    srv = lwip_accept(listener, (struct sockaddr *)&source_addr, &addr_len);
    /* accepted, so won't need the listener socket anymore, close it */
    ret = lwip_close(listener);
    CHECK_ERROR(ret, "close(listener)");
    usleep(10000);

    for (i=0; i < READ_TASKS; ++ i) {
        int err = pthread_create(&readers[i], NULL, reader_thread, (void*)(uintptr_t)srv);
        CHECK_ERROR(err, "pthread_create");
    }
    usleep(1000000);

    ret = lwip_close(srv);
    CHECK_ERROR(ret, "close(server)");
    return NULL;
}

int main(void)
{
    int client;
    int ret;
    unsigned i;
    int listener;
    struct sockaddr_in sa_listen;
    const u16_t port = 1234;
    static char txbuf[16*1024] = "something";
    int err;
    pthread_t srv_thread;

    for (i=0; i< sizeof(txbuf); ++i) {
        txbuf[i] = i & 0xff;
    }
    tcpip_init(NULL, NULL);

    /* set up the listener */
    memset(&sa_listen, 0, sizeof(sa_listen));
    sa_listen.sin_family = AF_INET;
    sa_listen.sin_port = PP_HTONS(port);
    sa_listen.sin_addr.s_addr = PP_HTONL(INADDR_LOOPBACK);
    listener = lwip_socket(AF_INET, SOCK_STREAM, 0);

    ret = lwip_bind(listener, (struct sockaddr *)&sa_listen, sizeof(sa_listen));
    CHECK_ERROR(ret, "bind");
    ret = lwip_listen(listener, 1);
    CHECK_ERROR(ret, "listen");
    /* continue to serve connections in a separate thread*/
    err = pthread_create(&srv_thread, NULL, server_thread, (void*)(uintptr_t)listener);
    CHECK_ERROR(err, "pthread_create");

    /* set up the client */
    client = lwip_socket(AF_INET, SOCK_STREAM, 0);
    ret = lwip_connect(client, (struct sockaddr *) &sa_listen, sizeof(sa_listen));
    CHECK_ERROR(ret, "connect");
    /* set socket to enable SO_LINGER option */
    ret = lwip_send(client, txbuf, sizeof(txbuf), 0);
    CHECK_ERROR(ret, "send");
    usleep(200000);

    /* close from client's side */
    ret = lwip_close(client);
    CHECK_ERROR(ret, "close(client)");
    err = errno;

    pthread_join(srv_thread, NULL);

  return 0;
}
