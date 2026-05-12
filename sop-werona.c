#include "l7-common.h"

void usage(char* name)
{
    printf("%s <timeout>\n", name);
    printf("  timeout - max waiting time after receiving the last message/connection (in seconds)\n");
    exit(EXIT_FAILURE);
}

#define SWAP(a, b)                     \
    do {                               \
        __typeof__(a) __a = (a);       \
        __typeof__(b) __b = (b);       \
        __typeof__(*__a) __tmp = *__a; \
        *__a = *__b;                   \
        *__b = __tmp;                  \
    } while (0)

#define MAX_CLIENTS 10
#define MAX_PAIRS 3
#define UNIX_SK_NAME "Laurenty"
#define MAX_MSG_LEN 63

#define BACKLOG 3 // nwm ile dac tutaj tak szczerze

void do_server(int local_socket_fd, int timeout)
{
    int epoll_fd;
    if ((epoll_fd = epoll_create1(0)) == -1) {
        ERR("epoll_create1");
    }

    struct epoll_event events[MAX_CLIENTS];
    struct epoll_event local_socket_event = { .events = EPOLLIN,
                                              .data.fd = local_socket_fd };

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, local_socket_fd, &local_socket_event) == -1) {
        ERR("epoll_ctl");
    }

    while (1) {
        int nfds;
        if ((nfds = epoll_wait(epoll_fd, events, MAX_CLIENTS, timeout * 1000)) == -1) { // nwm?
            if (errno == EINTR) {
                continue;
            }
            ERR("epoll_pwait");
        }

        if (nfds == 0) {
            printf("No one needs my help anymore!\n");
            close(epoll_fd);
            return;
        }

        for (int n = 0; n < nfds; n++) {
            int fd = events[n].data.fd;

            if (fd == local_socket_fd) {
                int client_fd;
                if ((client_fd = add_new_client(local_socket_fd)) == -1) {
                    ERR("add_new_client");
                }

                printf("Another young person (%d) needs my help!\n", client_fd);

                if (close(client_fd) == -1) {
                    ERR("close");
                }
            }
        }
    }
}

int main(int argc, char** argv)
{
    if (argc != 2) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    int timeout = atoi(argv[1]);
    if (timeout < 1) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    sethandler(SIG_IGN, SIGPIPE);

    int local_socket_fd = bind_local_socket(UNIX_SK_NAME, BACKLOG);

    int new_flags;
    if ((new_flags = fcntl(local_socket_fd, F_GETFL) | O_NONBLOCK) == -1) {
        ERR("fcntl");
    }
    if (fcntl(local_socket_fd, F_SETFL, new_flags) == -1) {
        ERR("fcntl");
    }

    do_server(local_socket_fd, timeout);

    if (close(local_socket_fd) == -1) {
        ERR("close");
    }

    if (unlink(UNIX_SK_NAME) == -1) {
        ERR("unlink");
    }

    return EXIT_SUCCESS;
}
