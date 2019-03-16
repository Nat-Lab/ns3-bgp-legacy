#include "bgp-filter.h"

namespace ns3 {

BGPFilterRule::BGPFilterRule() {}

BGPFilterRule::BGPFilterRule (const BGPFilterOP &op, const Ipv4Address &prefix, const Ipv4Mask &mask, bool exact) {
    this->op = op;
    this->prefix = prefix;
    this->mask = mask;
    this->exact = exact;
}

BGPFilterOP BGPFilterRule::apply (const Ipv4Address &prefix, const Ipv4Mask &mask) const {
    if (mask.Get() < this->mask.Get())
        return BGPFilterOP::NOP;

    if (exact && prefix == this->prefix && mask == this->mask)
        return this->op;

    if (prefix.CombineMask(this->mask) == this->prefix)
        return this->op;

    return BGPFilterOP::NOP;
}

void BGPFilterRules::operator= (const std::vector<BGPFilterRule> &rules) {
    this->rules = rules;
}

BGPFilterOP BGPFilterRules::apply (const Ipv4Address &prefix, const Ipv4Mask &mask) const {
    BGPFilterOP op = default_op;

    for (auto rule : rules) {
        auto cur_op = rule.apply(prefix, mask);
        if (cur_op != BGPFilterOP::NOP) op = cur_op;
    }

    return op;
}

void BGPFilterRules::set (const std::vector<BGPFilterRule> &rules) {
    this->rules = rules;
}

void BGPFilterRules::insert (const BGPFilterOP &op, const Ipv4Address &prefix, const Ipv4Mask &mask, bool exact) {
    rules.insert(rules.begin(), BGPFilterRule(op, prefix, mask, exact));
}

void BGPFilterRules::append (const BGPFilterOP &op, const Ipv4Address &prefix, const Ipv4Mask &mask, bool exact) {
    rules.push_back(BGPFilterRule(op, prefix, mask, exact));
}


}