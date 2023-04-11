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

void panic(const char *msg) {
  fprintf(stderr, "%s\nerrno: %s\n", msg, strerror(errno));
  exit(EXIT_FAILURE);
}

void sigint_handler(int signum) {
  if (signum != SIGINT)
    panic("xd");
  if (write(STDERR_FILENO, "\nBye bye!\n", 11) == -1)
    panic("write err");
  exit(EXIT_SUCCESS);
}

void listening(int sockfd, queue_t *qall, queue_t *qdir) {
  debug("listening...\n");
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(sockfd, &readfds);
  struct timeval timeout;
  timeout.tv_sec = ROUND;
  timeout.tv_usec = 0;
  while (true) {
    int ready = select(sockfd + 1, &readfds, NULL, NULL, &timeout);
    if (ready < 0)
      panic("[listening] select: error");
    if (ready == 0) {
      // debug("[listening] select: timeout\n");
      return;
    }
    entry_t packet;
    receive_packet(sockfd, &packet);
    update(&packet, qall, qdir);
  }
}

void update(entry_t *packet, queue_t *qall, queue_t *qdir) {

  entry_t *sender;
  uint8_t found = false;
  TAILQ_FOREACH (sender, qdir, direct) {
    if (sender->ip_addr == packet->via) {
      found = true;
      sender->cnt = TIME_TO_DIE;
      sender->reachable = true;
      sender->via = 0;
      sender->dist = sender->prev_dist;
      if (packet->ip_addr == sender->ip_addr)
        return;
      break;
    }
  }
  if (!found)
    return;

  entry_t *entry;
  TAILQ_FOREACH (entry, qall, all) {
    if (entry->ip_addr == packet->ip_addr && entry->mask == packet->mask) {
      /* error */
      if (packet->dist == INFTY) {
        if (entry->reachable && packet->via == entry->via) {
          entry->reachable = false;
          entry->dist = INFTY;
          entry->cnt = TIME_TO_DIE;
        }
      }
      /* better route */
      else if (entry->dist > packet->dist + sender->dist) {
        if (!entry->reachable) {
          if (entry->cnt > TIME_TO_RESTORE)
            return;
        }
        // if (entry->via == 0 && packet->via != packet->ip_addr &&
        //     entry->reachable) {
        //   panic("[update] xdddd "); // *XD*
        //   return;
        // }
        if (packet->via != packet->ip_addr)
          entry->via = packet->via;
        else {
          entry->via = 0;
          panic("[update] xd"); // *XD*
        }
        entry->dist = packet->dist + sender->dist;
        entry->reachable = true;
        entry->cnt = TIME_TO_DIE;
      }
      return;
    }
  }
  if (packet->dist != INFTY) {
    entry = malloc(sizeof(entry_t));
    entry->dist = packet->dist + sender->dist;
    entry->ip_addr = packet->ip_addr;
    entry->mask = packet->mask;
    entry->via = packet->via;
    entry->reachable = true;
    entry->cnt = TIME_TO_DIE;
    TAILQ_INSERT_TAIL(qall, entry, all);
    if (packet->via == packet->ip_addr) {
      entry->via = 0;
      panic("xd++");
    }
  }
}

void kill_neighbour(entry_t *net, queue_t *qall) {
  net->reachable = false;
  net->cnt = TIME_TO_DIE;
  net->dist = INFTY;
  entry_t *e;
  TAILQ_FOREACH (e, qall, all) {
    if (e->via == net->ip_addr) {
      debug(":)\n");
      e->reachable = false;
      e->cnt = TIME_TO_DIE;
      e->dist = INFTY;
    }
  }
}

void broadcasting(int sockfd, queue_t *qall, queue_t *qdir) {
  debug("broadcasting...\n");
  entry_t *neighbour;
  TAILQ_FOREACH (neighbour, qdir, direct) {
    entry_t *network;
    TAILQ_FOREACH (network, qall, all) {
      if (!send_packet(sockfd, network, neighbour)) {
        kill_neighbour(neighbour, qall);
      }
    }
  }
}

void cleanup(queue_t *qall) {
  debug("cleanup...\n");
  entry_t *e = TAILQ_FIRST(qall);
  while (e) {
    entry_t *e2;
    e2 = TAILQ_NEXT(e, all);

    if (e->reachable &&
        e->dist >= SMALL_INFTY) { // *XD* /* anty counting to infinity */
      e->reachable = false;
      e->dist = INFTY;
      e->cnt = TIME_TO_DIE;
    }
    if (e->via == 0 && e->reachable) /* report neighbour unreachable if you
                                        havent heard from him for some time */
    {
      e->cnt--;
      debug("$\n");
      if (e->cnt == 0) {
        kill_neighbour(e, qall);
      }
    } else if (!e->reachable &&
               e->via != 0) /* remove unreachable networks after a few rounds */
    {
      e->cnt--;
      if (e->cnt == 0)
        TAILQ_REMOVE(qall, e, all);
    } else if (!e->reachable && e->cnt > 0) {
      e->cnt--;
    }
    e = e2;
  }
}

void print(entry_t *net) {
  char ipstr[20];
  if (!inet_ntop(AF_INET, &net->ip_addr, ipstr, sizeof(ipstr)))
    panic("[print] inet_ntop error");
  printf("%s/%d distance %d ", ipstr, net->mask, net->dist);
  if (!net->reachable) {
    printf("network unreachable\n");
  } else if (net->via == 0) {
    printf("connected direcly\n");
  } else {
    if (!inet_ntop(AF_INET, &net->via, ipstr, sizeof(ipstr)))
      panic("[print] inet_ntop error");
    printf("via %s\n", ipstr);
  }
}

//  git branch -m <name>

void loop(int sockfd, queue_t *qall, queue_t *qdir) {
  int r = 0;
  while (true) {
    debug(
      "=================================================================\n");
    entry_t *net;
    TAILQ_FOREACH (net, qall, all) {
      print(net);
    }
    debug(
      "=================================================================\n");

    printf("\n");

    r++;
    printf("************   %d   ************\n", r);

    listening(sockfd, qall, qdir);
    broadcasting(sockfd, qall, qdir);
    cleanup(qall);
  }
}

void read_config(queue_t *tq, queue_t *dir) {
  int N = 0;
  if (scanf("%d", &N) == EOF)
    panic("[read_config] scanf: eof");
  for (int i = 0; i < N; i++) {
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
    network->prev_dist = network->dist;
    TAILQ_INSERT_TAIL(tq, network, all);
    TAILQ_INSERT_TAIL(dir, network, direct);
  }
}

// void get_my_ip() {
//   struct ifaddrs *ifaddr;

//   if (getifaddrs(&ifaddr) == -1) {
//     panic("[get_my_ip] getifaddrs error");
//   }

//   for (struct ifaddrs *ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
//     if (ifa->ifa_addr == NULL)
//       continue;
//   }
// }

int main() {
  signal(SIGINT, sigint_handler);
  queue_t qall;
  queue_t qdir;
  TAILQ_INIT(&qall);
  TAILQ_INIT(&qdir);
  read_config(&qall, &qdir);

  int sockfd = create_socket();
  bind_socket(sockfd, TARGET_PORT, INADDR_ANY);

  loop(sockfd, &qall, &qdir);

  return 0;
}

// 1. jak umiera to
// *  sąsiedzi ją znikają (jak długo od niej nic nie dostają lub jak sendto
// nie wyjdzie)
// *  i wysyłają do innych że jej nie ma
// *  i oznaczają wszystkich którzy mieli ją po drodze na niedostępnych

// 2. jak wstaje
// *  wysyła co ma w tablicy
// *  sąsiedzi to dostają i wiedzą że żyje (i przywracają oryginalny dystans)
// *
