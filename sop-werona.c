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

typedef struct client {
    int fd;
    char name[MAX_MSG_LEN];
    char beloved_name[MAX_MSG_LEN];
} client_t;

int new_client_index(client_t* clients)
{
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (-1 == clients[i].fd) {
            return i;
        }
    }
    return -1;
}

int get_client_index(client_t* clients, int client_fd)
{
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_fd == clients[i].fd) {
            return i;
        }
    }
    return -1;
}

void init_clients(client_t* clients)
{
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        clients[i].name[0] = '\0';
        clients[i].beloved_name[0] = '\0';
    }
}

void do_server(int local_socket_fd, int timeout)
{
    int epoll_fd;
    if ((epoll_fd = epoll_create1(0)) == -1) {
        ERR("epoll_create1");
    }

    struct epoll_event events[MAX_CLIENTS];
    struct epoll_event event = { .events = EPOLLIN,
                                 .data.fd = local_socket_fd };

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, local_socket_fd, &event) == -1) {
        ERR("epoll_ctl");
    }

    client_t clients[MAX_CLIENTS];
    init_clients(clients);

    while (1) {
        int nfds;
        if ((nfds = epoll_wait(epoll_fd, events, MAX_CLIENTS, timeout * 1000)) == -1) {
            if (errno == EINTR) {
                continue;
            }
            ERR("epoll_pwait");
        }

        if (nfds == 0) {
            printf("No one needs my help anymore!\n");
            close(epoll_fd);
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i].fd != -1) {
                    if (close(clients[i].fd) == -1) {
                        ERR("close");
                    }
                }
            }
            return;
        }

        for (int n = 0; n < nfds; n++) {
            int fd = events[n].data.fd;

            if (fd == local_socket_fd) {
                int client_fd;
                if ((client_fd = add_new_client(local_socket_fd)) == -1) {
                    ERR("add_new_client");
                }

                // czy na pewno tutaj fcntl?

                printf("Another young person (%d) needs my help!\n", client_fd);

                int client_index = new_client_index(clients);
                if (client_index == -1) {
                    if (close(client_fd) == -1) {
                        ERR("close");
                    }
                    continue;
                }

                clients[client_index].fd = client_fd;
                event.data.fd = client_fd;

                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &event) == -1) {
                    ERR("epoll_ctl");
                }

                continue;
            }

            int client_index;
            if ((client_index = get_client_index(clients, fd)) == -1) {
                ERR("get_client_index");
            }

            char msg[MAX_MSG_LEN + 1] = { 0 };
            ssize_t msg_size;
            if ((msg_size = recv(fd, msg, sizeof(msg) - 1, 0)) == -1) {
                ERR("recv");
            }

            // disconnect
            if (msg_size == 0) {
                if (clients[client_index].name[0] == '\0') {
                    printf("I lost contact with ??\n");
                } else {
                    printf("I lost contact with %s\n", clients[client_index].name);
                }

                if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
                    ERR("epoll_ctl");
                }
                if (close(fd) == -1) {
                    ERR("close");
                }

                clients[client_index].fd = -1;
                clients[client_index].name[0] = '\0';
                clients[client_index].beloved_name[0] = '\0';

                continue;
            }

            // incorrect msg
            if (strcspn(msg, "\n") == (size_t)msg_size) {
                continue;
            }

            msg[strcspn(msg, "\n")] = '\0';
            if (clients[client_index].name[0] == '\0') {
                strcpy(clients[client_index].name, msg);
            } else if (clients[client_index].beloved_name[0] == '\0') {
                strcpy(clients[client_index].beloved_name, msg);
                printf("%s wants to marry %s\n", clients[client_index].name, clients[client_index].beloved_name);

                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i].fd != -1 && i != client_index) {
                        if (strcmp(clients[client_index].name, clients[i].beloved_name) == 0) {
                            if (strcmp(clients[client_index].beloved_name, clients[i].name) == 0) {
                                printf("%s and %s got married!\n", clients[client_index].name, clients[client_index].beloved_name);

                                char congratulations_msg[MAX_MSG_LEN];
                                snprintf(
                                    congratulations_msg,
                                    sizeof(congratulations_msg),
                                    "Congratulations, %s and %s!",
                                    clients[client_index].name,
                                    clients[client_index].beloved_name
                                );

                                if (send(fd, congratulations_msg, strlen(congratulations_msg), 0) == -1) {
                                    ERR("send");
                                }

                                snprintf(
                                    congratulations_msg,
                                    sizeof(congratulations_msg),
                                    "Congratulations, %s and %s!",
                                    clients[i].name,
                                    clients[i].beloved_name
                                );

                                if (send(clients[i].fd, congratulations_msg, strlen(congratulations_msg), 0) == -1) {
                                    ERR("send");
                                }

                                if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
                                    ERR("epoll_ctl");
                                }
                                if (close(fd) == -1) {
                                    ERR("close");
                                }

                                clients[client_index].fd = -1;
                                clients[client_index].name[0] = '\0';
                                clients[client_index].beloved_name[0] = '\0';

                                if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, clients[i].fd, NULL) == -1) {
                                    ERR("epoll_ctl");
                                }
                                if (close(clients[i].fd) == -1) {
                                    ERR("close");
                                }

                                clients[i].fd = -1;
                                clients[i].name[0] = '\0';
                                clients[i].beloved_name[0] = '\0';
                            }
                        }
                    }
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
