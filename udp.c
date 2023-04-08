#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <strings.h>
#include <stdio.h>
#include <poll.h>
#include <stdbool.h>
#include <string.h>
#include "router.h"

int create_socket()
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        panic("socket: [create_socket()]");
    int broadcastPermission = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST,
               (void *)&broadcastPermission,
               sizeof(broadcastPermission));
    return sockfd;
}

void bind_socket(int sockfd, int port, int addr)
{
    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = htonl(addr);
    int succ = bind(sockfd, (struct sockaddr *)&server_address, sizeof(server_address));
    if (succ < 0)
        panic("bind: [bind_socket()]");
    printf("ok");
}

void receive_packet(int sockfd, entry_t *network)
{
    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);
    uint8_t buf[IP_MAXPACKET];
    ssize_t packet_len = recvfrom(sockfd, buf, IP_MAXPACKET, MSG_DONTWAIT, (struct sockaddr *)&sender, &sender_len);
    if (packet_len < 0)
        panic("[receive_packet] recvfrom: error");
    if (packet_len < MSG_LEN) // FIXME
    {
        panic("[receive_packet] recvfrom: wrong format"); // CHECKIT może to nie tak działa
    }
    // TODO jakieś sprawdzanie czy to ten pakiet który chcieliśmy
    network->ip_addr = *(uint32_t *)buf;
    network->mask = *(uint8_t *)(buf + 4);
    network->dist = *(uint32_t *)(buf + 5);
    network->via = ntohl(sender.sin_addr.s_addr);
}

// int receive_packet(int sockfd)
// {
//     struct sockaddr_in sender;
//     socklen_t sender_len = sizeof(sender);
//     uint8_t buf[IP_MAXPACKET];
//     struct pollfd fds;
//     fds.fd = sockfd;
//     fds.events = POLLIN;
//     int ready = poll(&fds, 1, 1000);
//     if (ready < 0)
//         panic("poll: [receive_packet]: err");
//     if (ready == 0)
//     {
//         fprintf(stderr, "poll: [receive_packets]: timeout");
//         return false;
//     }
//     ssize_t packet_len = recvfrom(sockfd, buf, IP_MAXPACKET, MSG_DONTWAIT, (struct sockaddr *)&sender, &sender_len);
//     return true;
// }

int send_packet(int sockfd, entry_t *network, entry_t *target)
{
    uint8_t buf[MSG_LEN];
    *buf = network->ip_addr;
    *(buf+4) = network->mask;
    *(buf+5) = network->dist;

    struct sockaddr_in recipient;
    bzero(&recipient, sizeof(recipient));
    recipient.sin_family = AF_INET;
    recipient.sin_port = htons(TARGET_PORT);
    recipient.sin_addr.s_addr = htonl(target->ip_addr);

    ssize_t bytes_sent = sendto(sockfd, buf, MSG_LEN, 0, (struct sockaddr *)&recipient, sizeof(recipient));
    if (bytes_sent < 0){
        panic("[send_packet] sendto error"); // CHECKIT
        return false;
    }
    return true;
}
