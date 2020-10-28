#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { 
    return _sender.stream_in().remaining_capacity(); 
}

size_t TCPConnection::bytes_in_flight() const { 
    return _sender.bytes_in_flight();
}

size_t TCPConnection::unassembled_bytes() const { 
    return _receiver.unassembled_bytes();
}

size_t TCPConnection::time_since_last_segment_received() const {
    return _received_timer;
}

void TCPConnection::segment_received(const TCPSegment &seg) {
    if(seg.header().fin) {

    }
    if(seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }
    _receiver.segment_received(seg);
    _received_timer = 0;
}

void TCPConnection::segment_sent() {
    auto seg = _sender.segments_out().front();
    _sender.segments_out().pop();

    //seg.header().
    //_segments_out.push();
    
}

bool TCPConnection::active() const { return {}; }

size_t TCPConnection::write(const string &data) {
    size_t len = _sender.stream_in().write(data);
    _sender.fill_window();
    return len;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _received_timer += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
}

void TCPConnection::connect() {
    _sender.send_empty_segment();
    
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
