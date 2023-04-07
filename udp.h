int create_socket();
void bind_socket(int sockfd, int port, int addr);
int receive_packet(int sockfd);
void send_packet(int sockfd);