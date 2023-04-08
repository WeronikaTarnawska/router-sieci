#include <sys/queue.h>
#include <stdint.h>
#define TARGET_PORT 54321 /* we listen and broadcast here */
#define ROUND 20 /* one round is 20 seconds */
#define SMALL_INFTY 12345678 /* to deal with "counting to infinity" issue */
#define INFTY 0xffffffff /* uint32_t max */
#define TIME_TO_DIE 5 /* if network is unreachable, we broadcast about it for 5 rounds and then remove the network */
#define MSG_LEN 9


typedef struct entry{ /* we store network data here */
    uint32_t ip_addr;
    uint8_t mask;
    uint32_t dist; /* if(not reachable) dist = INFTY */
    uint8_t reachable;
    uint32_t via; /* next ip address on route to this network or 0 if connected directly */
    uint32_t cnt; /* if directly connected means number of rounds since we last heard from this network, else if not reachable means time to die */
    TAILQ_ENTRY(entry) all;
    TAILQ_ENTRY(entry) direct;
} entry_t;

typedef TAILQ_HEAD(tailhead, entry) queue_t; /* and we keep the abovementioned structures on tailqueue */

/* router.c */
void panic(const char *msg);
void update(entry_t *packet, queue_t *qall, queue_t *qdir);
void listening(int sockfd, queue_t *qall, queue_t *qdir);
void broadcasting(int sockfd, queue_t *qall, queue_t *qdir);
void cleanup(queue_t *qall);
void loop(int sockfd, queue_t *qall, queue_t *qdir);
void read_config(queue_t *tq, queue_t *dir);



/* udp.c */
int create_socket();
void bind_socket(int sockfd, int port, int addr);
void receive_packet(int sockfd, entry_t *network);
int send_packet(int sockfd, entry_t *network, entry_t *target);


#define DEBUG
#ifdef DEBUG
#define debug(...) fprintf(stderr, __VA_ARGS__)
#else
#define debug(...)
#endif