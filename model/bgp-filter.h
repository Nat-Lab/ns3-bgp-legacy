#ifndef BGP_FILTER_H
#define BGP_FILTER_H

#include <vector>
#include "ns3/ipv4-address.h"

namespace ns3 {

typedef enum BGPFilterOP {
    NOP,
    ACCEPT,
    REJECT
} BGPFilterOP;

typedef struct BGPFilterRule {
    BGPFilterOP op;
    Ipv4Address prefix;
    Ipv4Mask mask;

    BGPFilterRule();
    BGPFilterRule(const BGPFilterOP &op, const Ipv4Address &prefix, const Ipv4Mask &mask);
    BGPFilterOP apply(const Ipv4Address &prefix, const Ipv4Mask &mask) const;
} BGPFilterRule;

typedef struct BGPFilterRules {
    std::vector<BGPFilterRule> rules;
    BGPFilterOP default_op;

    void operator= (const std::vector<BGPFilterRule> &rules);
    void set (const std::vector<BGPFilterRule> &rules);
    void insert (const BGPFilterOP &op, const Ipv4Address &prefix, const Ipv4Mask &mask);
    void append (const BGPFilterOP &op, const Ipv4Address &prefix, const Ipv4Mask &mask);
    BGPFilterOP apply (const Ipv4Address &prefix, const Ipv4Mask &mask) const;
} BGPFilterRules;

}

#endif //BGP_FILTER_H