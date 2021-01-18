#include "router.hh"

#include <iostream>

using namespace std;

// Dummy implementation of an IP router

// Given an incoming Internet datagram, the router decides
// (1) which interface to send it out on, and
// (2) what next hop address to send it to.

// For Lab 6, please replace with a real implementation that passes the
// automated checks run by `make check_lab6`.

// You will need to add private members to the class declaration in `router.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

//! \param[in] route_prefix The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
//! \param[in] prefix_length For this route to be applicable, how many high-order (most-significant) bits of the route_prefix will need to match the corresponding bits of the datagram's destination address?
//! \param[in] next_hop The IP address of the next hop. Will be empty if the network is directly attached to the router (in which case, the next hop address should be the datagram's final destination).
//! \param[in] interface_num The index of the interface to send the datagram out on.
void Router::add_route(const uint32_t route_prefix,
                       const uint8_t prefix_length,
                       const optional<Address> next_hop,
                       const size_t interface_num) {
    cerr << "DEBUG: adding route " << Address::from_ipv4_numeric(route_prefix).ip() << "/" << int(prefix_length)
         << " => " << (next_hop.has_value() ? next_hop->ip() : "(direct)") << " on interface " << interface_num << "\n";

    uint32_t prefix = prefix_length == 0? 0: route_prefix >> (IPv4_LENGTH - prefix_length);
    route_table[prefix_length][prefix] = {interface_num, next_hop};
}

void Router::send_datagram_to_interface(InternetDatagram &dgram, std::pair<size_t, std::optional<Address>> route) {
    size_t interface_num = route.first;
    auto next_hop = route.second.has_value()?route.second.value():Address::from_ipv4_numeric(dgram.header().dst);

    interface(interface_num).send_datagram(dgram, next_hop);
}

//! \param[in] dgram The datagram to be routed
void Router::route_one_datagram(InternetDatagram &dgram) {
    if (dgram.header().ttl <= 1) {
        return;
    }
    dgram.header().ttl--;
    
    uint32_t dst = dgram.header().dst;
    for (int8_t length = IPv4_LENGTH; length >= 0; length--) {
        uint32_t prefix = length == 0? 0: dst >> (IPv4_LENGTH - length);
        if (route_table[length].count(prefix) > 0) {
            send_datagram_to_interface(dgram, route_table[length][prefix]);
            return;
        }
    }
}

void Router::route() {
    // Go through all the interfaces, and route every incoming datagram to its proper outgoing interface.
    for (auto &interface : _interfaces) {
        auto &queue = interface.datagrams_out();
        while (not queue.empty()) {
            route_one_datagram(queue.front());
            queue.pop();
        }
    }
}
