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
// #include <assert.h>
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

#include "udp.h"
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
    if(write(STDERR_FILENO, "\nBye bye!\n", 11)==-1)
    panic("write err");
    exit(EXIT_SUCCESS);
}

// 1. Nasłuchiwać na porcie 54321, odbierać i przetwarzać rozsyłane przez innych elementy wektora odległości
void listen_uptadev()
{
    // listen(TARGET_PORT, N); // CHECKIT
    // while(receive_packet(socket, ))
    update_table();
}

// 2. Co pewien zdefiniowany w programie czas (15–30 sekund, nazywany turą) wysyłać wszystkie elementy swojego aktualnego wektora odległości
// (pary składające się z sieci i odległości do nich) na adresy rozgłoszeniowe wszystkich sąsiednich sieci.
// Każdy element wektora powinien być wysyłany w osobnym pakiecie UDP, skierowanym do portu 54321, w formacie określonym w kolejnej sekcji.
void broadcast()
{
    // for (int sockfd in TQ)
    //     send_packet(sockfd);
}

// 3. Utrzymywać tablicę routingu, czyli bieżący wektor (najkrótszych) odległości do poszczególnych sieci
// a przypadku sieci niebezpośrednio podłączonych również informację o pierwszym routerze na trasie do takiej sieci.
void update_table()
{
}

// 4. Reagować na nieosiągalność sąsiadów, uaktualniając odległość do nich na nieskończoną (a także do prowadzących przez nich sieci).
// O nieosiągalności dowiadujemy się z dwóch źródeł:
// (1) funkcja sendto może zwrócić błąd (np. w przypadku nieaktywnego interfejsu),
// (2) nie dostaniemy od maszyny z sąsiedniej sieci przez parę tur żadnej wiadomości

// 5. Rozgłaszać wpisy wektora z odległościami nieskończonymi przez kilka kolejnych tur (w takim samym trybie jak inne wpisy),
// a następnie usuwać je z wektora odległości i tablicy przekazywania

//  git branch -m <name>

void read_config(queue_t *tq)
{
    int N = 0;
    if(scanf("%d", &N)==EOF)panic("[read_config] scanf: eof");
    for (int i = 0; i < N; i++)
    {
        char ip_str[20];
        entry_t *network = malloc(sizeof(entry_t));
        if(scanf("%s", ip_str)==EOF)
        panic("[read_config] scanf: eof");
        // int x = strcspn(argv[i], "/");
        char *ip = strtok(ip_str, "/");
        char *m = strtok(NULL, "/");
        network->mask = atoi(m);
        int succ = inet_pton(AF_INET, ip, &network->ip_addr);
        if (succ < 0)
            panic("[read_config] inet_pton: invalid address family");
        if (succ == 0)
            panic("[read_config] inet_pton: invalid ip address");
        if(scanf("%s", ip_str)==EOF) panic("[read_config] scanf: eof");
        if(scanf("%d", &network->dist)==EOF) panic("[read_config] scanf: eof");
        network->reachable=true;
        TAILQ_INSERT_TAIL(tq, network, que);
    }

    // for (int i = 0; i < N; i++)
    //     printf("%d, %d, %d\n", ip_addr[i], mask[i], distance[i]);

    // int sockfd = create_socket();
    // bind_socket(sockfd, 54321, ip_addr[0]);
}

int main(int argc, char **argv)
{
    signal(SIGINT, sigint_handler);
    queue_t head;
    TAILQ_INIT(&head);
    read_config(&head);
    printf("read_config success :)\n");
    entry_t *np;
    TAILQ_FOREACH(np, &head, que){
        printf("ip: %d, mask: %d, dist: %d, reachable: %d\n", np->ip_addr, np->mask, np->dist, np->reachable);
    }
    while (true)
    {
        // 1. nasłuch na 54321
        // 2. update tablicy + zmniejszanie o 1 czasu do usunięcia jeżeli unreachable + usunięcie tych z czasem 0
        // 3. rozsyłanie tablicy
        // 4. update tablicy (in case sendto failed)
        // 6. zaczekać na koniec tury? albo na początku ustawić alarm a teraz zasnąć do wybudzenia? CHECKIT
    }

    printf(":)\n");
    return 0;
}