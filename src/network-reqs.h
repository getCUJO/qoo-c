#ifndef _DOMOS_NETREQ_H
#define _DOMOS_NETREQ_H

#define MAX_QTA_LIST_SIZE 32

struct simple_QTA {
    int num_percentiles;
    float percentiles[MAX_QTA_LIST_SIZE];
    int num_latencies;
    float latencies[MAX_QTA_LIST_SIZE];
};

// Network requirements can be specified for a type of traffic
// nr_perf is short for "Network Requirement for Perfection"
// nr_useless is short for "Network Requirement Point of Uselessness"
// See: https://www.ietf.org/id/draft-olden-ippm-qoo-00.html for details
struct simple_NR_list {
    char type[MAX_QTA_LIST_SIZE];
    struct simple_QTA nr_perf;
    struct simple_QTA nr_useless;
};

#endif