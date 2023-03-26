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
// #include <unistd.h>
// #include <netinet/ip.h>
// #include<aio.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "udp.h"

void panic(const char *msg)
{
    fprintf(stderr, "%s\nerrno: %s\n", msg, strerror(errno));
    exit(EXIT_FAILURE);
}

// 1. Nasłuchiwać na porcie 54321, odbierać i przetwarzać rozsyłane przez innych elementy wektora odległości

// 2. Co pewien zdefiniowany w programie czas (15–30 sekund, nazywany turą) wysyłać wszyst kie elementy swojego aktualnego wektora odległości 
// (pary składające się z sieci i odległości do nich) na adresy rozgłoszeniowe wszystkich sąsiednich sieci. 
// Każdy element wektora powinien być wysyłany w osobnym pakiecie UDP, skierowanym do portu 54321, w formacie określonym w kolejnej sekcji.


// 3. Utrzymywać tablicę routingu, czyli bieżący wektor (najkrótszych) odległości do poszczególnych sieci 
// a przypadku sieci niebezpośrednio podłączonych również informację o pierwszym routerze na trasie do takiej sieci.

// 4. Reagować na nieosiągalność sąsiadów, uaktualniając odległość do nich na nieskończoną (a także do prowadzących przez nich sieci). 
// O nieosiągalności dowiadujemy się z dwóch źródeł: 
// (1) funkcja sendto może zwrócić błąd (np. w przypadku nieaktywnego interfejsu),
// (2) nie dostaniemy od maszyny z sąsiedniej sieci przez parę tur żadnej wiadomości

// 5. Rozgłaszać wpisy wektora z odległościami nieskończonymi przez kilka kolejnych tur (w takim samym trybie jak inne wpisy), 
// a następnie usuwać je z wektora odległości i tablicy przekazywania


//  git branch -m <name>

void read_konfig(){
    // TODO: Czytamy z stdin
    // N = ile lini
    // for (każda linia)
    //      [ip/maska] distance [dist]

}


int main(int argc, char **argv) // FIXME czytamy z stdin
{
    uint32_t distance[argc];
    uint32_t ip_addr[argc];
    uint32_t mask[argc];
    if (argc < 3 || (argc - 1) % 3 != 0)
    {
        panic("usage: [ip address] distance [dist]");
    }
    int N = 0;
    for (int i = 1; i < argc; i += 3)
    {
        distance[N] = atoi(argv[i + 2]);
        // int x = strcspn(argv[i], "/");
        char *ip = strtok(argv[i], "/");
        char *m = strtok(NULL, "/");
        mask[N] = atoi(m);
        int succ = inet_pton(AF_INET, ip, &ip_addr[N]);
        if (succ < 0)
            panic("inet_pton: invalid address family");
        if (succ == 0)
            panic("inet_pton: invalid ip address");
        N++;
    }

    for (int i = 0; i < N; i++)
        printf("%d, %d, %d\n", ip_addr[i], mask[i], distance[i]);

    int sockfd = create_socket();
    bind_socket(sockfd, 54321, ip_addr[0]);

    printf(":)\n");
    return 0;
}