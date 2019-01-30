#ifndef BGP_FRAG_H
#define BGP_FRAG_H

#include <vector>

#include "ns3/ipv4-address.h"

#include <string.h>

namespace ns3 {

typedef struct BGPFragment {
    int size;
    uint8_t *buffer;
    Ipv4Address addr;

    void set(uint8_t *buf, int buf_sz);
    void clear();
} BGPFragment;

typedef struct BGPFragments {
    std::vector<BGPFragment*> frags;
    
    BGPFragment* getFragment(Ipv4Address addr);
    void setFragment(Ipv4Address addr, uint8_t *buf, int buf_sz);
    void clearFragment(Ipv4Address addr);
} BGPFragments;

}

#endif // BGP_FRAG_H