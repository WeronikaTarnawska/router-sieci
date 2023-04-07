#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <strings.h>
#include <stdio.h>
#include <poll.h>
#include <stdbool.h>
#include "router.h"

int create_socket()
{
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        panic("socket: [create_socket()]");
    return sockfd;
}

void bind_socket(int sockfd, int port, int addr)
{
    struct sockaddr_in server_address;
    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = htonl(addr); // htonl(INADDR_ANY); ?
    int succ = bind(sockfd, (struct sockaddr *)&server_address, sizeof(server_address));
    if (succ < 0)
        panic("bind: [bind_socket()]");
    printf("ok");
}

int receive_packet(int sockfd)
{
    struct sockaddr_in sender;
    socklen_t sender_len = sizeof(sender);
    uint8_t buf[IP_MAXPACKET];
    struct pollfd fds;
    fds.fd = sockfd;
    fds.events = POLLIN;
    int ready = poll(&fds, 1, 1000);
    if (ready < 0)
        panic("poll: [receive_packet]: err");
    if (ready == 0)
    {
        fprintf(stderr, "poll: [receive_packets]: timeout");
        return false;
    }
    ssize_t packet_len = recvfrom(sockfd, buf, IP_MAXPACKET, MSG_DONTWAIT, (struct sockaddr *)&sender, &sender_len);
    return true;
}

void send_packet(int sockfd)
{
    struct sockaddr_in recipient;
    /* TODO recipient - uzupełnij co tam ... */
    uint8_t buf[/*?*/ 0];
    size_t buflen = /*?*/ 0;

    ssize_t bytes_sent = sendto(sockfd, buf, buflen, 0, (struct sockaddr *)&recipient, sizeof(recipient));
}
