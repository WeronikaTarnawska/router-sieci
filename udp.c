/* Weronika Tarnawska 331171 */

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

int create_socket() {
  int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
  if (sockfd < 0)
    panic("socket: [create_socket()]");
  int broadcastPermission = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (void *)&broadcastPermission, sizeof(broadcastPermission));
  return sockfd;
}

void bind_socket(int sockfd, int port, int addr) {
  struct sockaddr_in server_address;
  bzero(&server_address, sizeof(server_address));
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(port);
  server_address.sin_addr.s_addr = htonl(addr);
  int succ = bind(sockfd, (struct sockaddr *)&server_address, sizeof(server_address));
  if (succ < 0)
    panic("bind: [bind_socket()]");
}

void receive_packet(int sockfd, entry_t *network) {
  struct sockaddr_in sender;
  socklen_t sender_len = sizeof(sender);
  uint8_t buf[IP_MAXPACKET];
  ssize_t packet_len = recvfrom(sockfd, buf, IP_MAXPACKET, MSG_DONTWAIT, (struct sockaddr *)&sender, &sender_len);
  if (packet_len < 0)
    panic("[receive_packet] recvfrom: error");
  if (packet_len < MSG_LEN) {
    panic("[receive_packet] recvfrom: wrong format");
  }
  network->ip_addr = *(uint32_t *)buf;
  network->mask = *(uint8_t *)(buf + 4);
  network->dist = *(uint32_t *)(buf + 5);
  network->via = sender.sin_addr.s_addr;
  network->reachable = 1;
}


int send_packet(int sockfd, entry_t *network, entry_t *target) {
  uint8_t buf[MSG_LEN];
  *(uint32_t *)buf = network->ip_addr;
  *(buf + 4) = network->mask;
  *(uint32_t *)(buf + 5) = network->dist;

  struct sockaddr_in recipient;
  bzero(&recipient, sizeof(recipient));
  recipient.sin_family = AF_INET;
  recipient.sin_port = htons(TARGET_PORT);

  recipient.sin_addr.s_addr = get_broadcast_ip(target->ip_addr, target->mask);

  ssize_t bytes_sent = sendto(sockfd, buf, MSG_LEN, 0, (struct sockaddr *)&recipient, sizeof(recipient));
  if (bytes_sent < 0) {
    debug("[send_packet] sendto error\n");
    return false;
  }
  return true;
}

uint32_t get_network_ip(uint32_t ip, uint8_t mask) {
  ip = htonl(ip);
  ip = ip & ((0xFFFFFFFF << (32 - mask)) & 0xFFFFFFFF);
  return htonl(ip);
}

uint32_t get_broadcast_ip(uint32_t ip, uint32_t mask) {
  ip = htonl(ip);
  ip = ip | ~((0xFFFFFFFF << (32 - mask)) & 0xFFFFFFFF);
  return htonl(ip);
}
