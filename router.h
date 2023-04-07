#include <sys/queue.h>
#include <stdint.h>
#define TARGET_PORT 54321 /* we listen and broadcast here */
#define ROUND 20 /* one round is 20 seconds */
#define INFINITY 12345678 /* to deal with "counting to infinity" issue */

typedef struct distv{ /* we store network data here */
    int32_t ip_addr;
    int32_t mask; // CHECKIT nie potrzebujemy 32, int16_t?
    int32_t dist; /* if(not reachable) dist means time to die */
    int8_t reachable;
    TAILQ_ENTRY(entry_t) que;
} entry_t;

typedef TAILQ_HEAD(tailhead, entry_t) queue_t; /* and we keep the abovementioned structures on tailqueue */

void panic(const char *msg);