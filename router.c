/* Weronika Tarnawska 331171 */
// #include <sys/cdefs.h>
#include <arpa/inet.h>
// #include <netinet/ip.h>
// #include <netinet/ip_icmp.h>
#include <sys/types.h>
// #include <sys/select.h>
// #include <sys/socket.h>
// #include <netinet/in.h>
// #include <netinet/in_systm.h>
// #include <sys/time.h>
#include <assert.h>
// #include <stdio.h>
// #include <stdlib.h>
#include <string.h>
// #include <strings.h>
#include <errno.h>
#include <unistd.h>
// #include <netinet/ip.h>
// #include<aio.h>
#include <stdbool.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <signal.h>
#include <sys/queue.h>
#include <sys/select.h>
#include "router.h"

void panic(const char *msg)
{
    fprintf(stderr, "%s\nerrno: %s\n", msg, strerror(errno));
    exit(EXIT_FAILURE);
}

void sigint_handler(int signum)
{
    if (signum != SIGINT)
        panic("xd");
    if (write(STDERR_FILENO, "\nBye bye!\n", 11) == -1)
        panic("write err");
    exit(EXIT_SUCCESS);
}

void listening(int sockfd, queue_t *qall, queue_t *qdir)
{
    debug("listening...\n");
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sockfd, &readfds);
    struct timeval timeout;
    timeout.tv_sec = ROUND;
    timeout.tv_usec = 0;
    while (true)
    {
        int ready = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
        if (ready < 0)
            panic("[listening] select: error");
        if (ready == 0)
        {
            debug("[listening] select: timeout\n");
            return;
        }
        entry_t packet;
        receive_packet(sockfd, &packet);
        update(&packet, qall, qdir);
    }
}

void update(entry_t *packet, queue_t *qall, queue_t *qdir)
{
    debug("update: ip=%d, via=%d, dist=%d\n", packet->ip_addr, packet->via, packet->dist);
    entry_t *entry;
    TAILQ_FOREACH(entry, qdir, direct)
    {
        if (entry->ip_addr == packet->via)
        {
            entry->cnt = TIME_TO_DIE;
            break;
        }
    }

    TAILQ_FOREACH(entry, qall, all)
    {
        if (entry->ip_addr == packet->ip_addr && entry->mask == packet->mask)
        {
            if (packet->dist == INFTY)
            {
                if (entry->reachable && packet->via == entry->via)
                {
                    entry->reachable = false;
                    entry->dist = INFTY;
                    entry->cnt = TIME_TO_DIE;
                }
            }
            else if (entry->dist > packet->dist)
            {
                entry->dist = packet->dist;
                if (packet->via != packet->ip_addr)
                    entry->via = packet->via;
                else
                {
                    entry->via = 0;
                    panic("[update] xd");
                    // CHECKIT czy jeśli jest połączona bezpośrednio to powinniśmy to zmieniać?
                }
                entry->reachable = true;
                entry->cnt = TIME_TO_DIE;
            }
            return;
        }
    }
    if (packet->dist != INFTY)
    {
        entry = malloc(sizeof(entry_t));
        entry->dist = packet->dist;
        entry->ip_addr = packet->ip_addr;
        entry->mask = packet->mask;
        entry->via = packet->via;
        entry->reachable = true;
        entry->cnt = TIME_TO_DIE;
        TAILQ_INSERT_TAIL(qall, entry, all);
        if (packet->via == packet->ip_addr)
        {
            entry->via = 0;
            TAILQ_INSERT_TAIL(qdir, entry, direct);
        }
    }
}

void broadcasting(int sockfd, queue_t *qall, queue_t *qdir)
{
    debug("broadcasting...\n");
    entry_t *neighbour;
    TAILQ_FOREACH(neighbour, qdir, direct)
    {
        assert(neighbour->via == 0); // FIXME możesz to potem usunąć
        entry_t *network;
        TAILQ_FOREACH(network, qall, all)
        {
            if (!send_packet(sockfd, network, neighbour))
            {
                neighbour->reachable = false;
                neighbour->cnt = TIME_TO_DIE;
                neighbour->dist = INFTY;
            }
        }
    }
}

void cleanup(queue_t *qall)
{
    debug("cleanup...\n");
    entry_t *e = TAILQ_FIRST(qall);
    while (e)
    {
        entry_t *e2;
        e2 = TAILQ_NEXT(e, all);
        if (e->via == 0 && e->reachable) /* report neighbour unreachable if you havent heard from him for some time */
        {
            e->cnt--;
            if (e->cnt == 0)
            {
                e->reachable = false;
                e->dist = INFTY;
                e->cnt = TIME_TO_DIE;
            }
        }
        else if (!e->reachable && e->via != 0) /* remove unreachable networks after a few rounds */
        {
            e->cnt--;
            if (e->cnt == 0)
                TAILQ_REMOVE(qall, e, all);
        }
        e = e2;
    }
}

void print(queue_t *que)
{
    entry_t *net;
    TAILQ_FOREACH(net, que, all)
    {
        debug("ip int: %d ip cidr: ", net->ip_addr);
        char ipstr[20];
        if (!inet_ntop(AF_INET, &net->ip_addr, ipstr, sizeof(ipstr)))
            panic("[print] inet_ntop error");
        printf("%s/%d distance %d ", ipstr, net->mask, net->dist);
        if (net->via == 0)
        {
            printf("connected direcly\n");
        }
        else
        {
            if (!inet_ntop(AF_INET, &net->via, ipstr, sizeof(ipstr)))
                panic("[print] inet_ntop error");
            printf("via %s\n", ipstr);
        }
    }
    printf("\n");
}

//  git branch -m <name>

void loop(int sockfd, queue_t *qall, queue_t *qdir)
{
    while (true)
    {
        listening(sockfd, qall, qdir);
        broadcasting(sockfd, qall, qdir);
        cleanup(qall);
        print(qall);
    }
}

void read_config(queue_t *tq, queue_t *dir)
{
    int N = 0;
    if (scanf("%d", &N) == EOF)
        panic("[read_config] scanf: eof");
    for (int i = 0; i < N; i++)
    {
        char ip_str[20];
        entry_t *network = malloc(sizeof(entry_t));
        if (scanf("%s", ip_str) == EOF)
            panic("[read_config] scanf: eof");
        char *ip = strtok(ip_str, "/");
        char *m = strtok(NULL, "/");
        network->mask = atoi(m);
        int succ = inet_pton(AF_INET, ip, &network->ip_addr);
        if (succ < 0)
            panic("[read_config] inet_pton: invalid address family");
        if (succ == 0)
            panic("[read_config] inet_pton: invalid ip address");
        if (scanf("%s", ip_str) == EOF)
            panic("[read_config] scanf: eof");
        if (scanf("%d", &network->dist) == EOF)
            panic("[read_config] scanf: eof");
        network->reachable = true;
        network->via = 0;
        network->cnt = TIME_TO_DIE;
        TAILQ_INSERT_TAIL(tq, network, all);
        TAILQ_INSERT_TAIL(dir, network, direct);
    }
}

int main()
{
    signal(SIGINT, sigint_handler);
    queue_t qall;
    queue_t qdir;
    TAILQ_INIT(&qall);
    TAILQ_INIT(&qdir);
    read_config(&qall, &qdir);

    printf("read_config success :)\n");
    entry_t *np;
    TAILQ_FOREACH(np, &qall, all)
    {
        printf("ip: %d, mask: %d, dist: %d, reachable: %d\n", np->ip_addr, np->mask, np->dist, np->reachable);
    }

    int sockfd = create_socket();
    bind_socket(sockfd, TARGET_PORT, INADDR_ANY);

    loop(sockfd, &qall, &qdir);

    printf(":)\n");
    return 0;
}