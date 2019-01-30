#include "bgp-fragment.h"

namespace ns3 {

BGPFragment* BGPFragments::getFragment(Ipv4Address addr) {
    int sz = frags.size();
    for (int i = 0; i < sz; i++) {
        if (frags[i]->addr.IsEqual(addr)) return frags[i];
    }

    auto frag = new BGPFragment;
    frag->addr = addr;
    frag->size = 0;
    frags.push_back(frag);

    return frag;

}

void BGPFragments::setFragment(Ipv4Address addr, uint8_t *buf, int buf_sz) {
    auto frag = getFragment(addr);
    frag->set(buf, buf_sz);
}

void BGPFragments::clearFragment(Ipv4Address addr) {
    auto frag = getFragment(addr);
    frag->clear();
}

void BGPFragment::set(uint8_t *buf, int buf_sz) {
    this->buffer = (uint8_t *) malloc(buf_sz);
    this->size = buf_sz;
    memcpy(this->buffer, buf, buf_sz);
    
}

void BGPFragment::clear() {
    delete this->buffer;
    this->size = 0;
}

}