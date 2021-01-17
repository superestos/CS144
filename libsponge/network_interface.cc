#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

#include <iostream>

// Dummy implementation of a network interface
// Translates from {IP datagram, next hop address} to link-layer frame, and from link-layer frame to IP datagram

// For Lab 5, please replace with a real implementation that passes the
// automated checks run by `make check_lab5`.

// You will need to add private members to the class declaration in `network_interface.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] ethernet_address Ethernet (what ARP calls "hardware") address of the interface
//! \param[in] ip_address IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface(const EthernetAddress &ethernet_address, const Address &ip_address)
    : _ethernet_address(ethernet_address), _ip_address(ip_address) {
    cerr << "DEBUG: Network interface has Ethernet address " << to_string(_ethernet_address) << " and IP address "
         << ip_address.ip() << "\n";
}

//! \param[in] dgram the IPv4 datagram to be sent
//! \param[in] next_hop the IP address of the interface to send it to (typically a router or default gateway, but may also be another host if directly connected to the same network as the destination)
//! (Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) with the Address::ipv4_numeric() method.)
void NetworkInterface::send_datagram(const InternetDatagram &dgram, const Address &next_hop) {
    // convert IP address of next hop to raw 32-bit representation (used in ARP header)
    const uint32_t next_hop_ip = next_hop.ipv4_numeric();
    EthernetFrame frame;
    EthernetHeader header;

    if (translator.count(next_hop_ip) == 0 || translator[next_hop_ip].first == ETHERNET_BROADCAST || translator[next_hop_ip].second + TIMEOUT_FOR_APR_STORAGE < current_time) {
        if (translator[next_hop_ip].first == ETHERNET_BROADCAST && translator[next_hop_ip].second + TIMEOUT_FOR_APR_REQUEST > current_time) {
            return;
        }
        translator[next_hop_ip] = {ETHERNET_BROADCAST, current_time};
        
        ARPMessage arp;
        arp.opcode = ARPMessage::OPCODE_REQUEST;
        arp.sender_ethernet_address = _ethernet_address;
        arp.sender_ip_address = _ip_address.ipv4_numeric();
        arp.target_ip_address = next_hop_ip;

        frame.payload() = arp.serialize();
        
        header.dst = ETHERNET_BROADCAST;
        header.type = EthernetHeader::TYPE_ARP;

        if (unsent_packets.count(next_hop_ip) == 0) {
            unsent_packets[next_hop_ip] = std::queue<std::pair<InternetDatagram, Address>>();
        }

        unsent_packets[next_hop_ip].push({dgram, next_hop});
    }
    else {
        frame.payload() = dgram.serialize();

        header.dst = translator[next_hop_ip].first;
        header.type = EthernetHeader::TYPE_IPv4;
    }
    
    header.src = _ethernet_address;
    frame.header() = header;
    _frames_out.push(frame);
}

//! \param[in] frame the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame(const EthernetFrame &frame) {
    DUMMY_CODE(frame);

    EthernetHeader header = frame.header();

    if (header.dst != _ethernet_address && header.dst != ETHERNET_BROADCAST) {
        return {};
    }

    if (header.type == EthernetHeader::TYPE_ARP) {
        ARPMessage arp;
        arp.parse(frame.payload());
        auto ip = arp.sender_ip_address;
        translator[ip] = {arp.sender_ethernet_address, current_time};

        if (arp.opcode == ARPMessage::OPCODE_REPLY) {
            if (unsent_packets.count(ip) > 0) {
                while (!unsent_packets[ip].empty()) {
                    auto pair = unsent_packets[ip].front();
                    unsent_packets[ip].pop();
                    send_datagram(pair.first, pair.second);
                }
            }
        }
        else if (arp.opcode == ARPMessage::OPCODE_REQUEST) {
            if (arp.target_ip_address != _ip_address.ipv4_numeric()) {
                return {};
            }

            ARPMessage reply_arp;
            reply_arp.opcode = ARPMessage::OPCODE_REPLY;
            reply_arp.sender_ethernet_address = _ethernet_address;
            reply_arp.sender_ip_address = _ip_address.ipv4_numeric();
            reply_arp.target_ethernet_address = arp.sender_ethernet_address;
            reply_arp.target_ip_address = arp.sender_ip_address;

            EthernetHeader reply_header;
            reply_header.src = _ethernet_address;
            reply_header.dst = header.src;
            reply_header.type = EthernetHeader::TYPE_ARP;

            EthernetFrame reply_frame;
            reply_frame.header() = reply_header;
            reply_frame.payload() = reply_arp.serialize();
            _frames_out.push(reply_frame);
        }
        
    }
    else if (header.type == EthernetHeader::TYPE_IPv4) {
        InternetDatagram dgram;
        dgram.parse(frame.payload());
        return dgram;
    }

    return {};
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void NetworkInterface::tick(const size_t ms_since_last_tick) {
    current_time += ms_since_last_tick;
}
