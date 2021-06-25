#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    // init isn with syn comes to avoid other side send multiple sync
    if (seg.header().syn) {
        connection_state = SYN_RECV;
        _isn = seg.header().seqno;
        _absolute_shift = 0;
    }
    if (connection_state == SYN_RECV && seg.header().fin) {
        connection_state = FIN_RECV;
    }

    size_t absolute_seqno = unwrap(seg.header().seqno, _isn, _reassembler.assembled_bytes());
    _reassembler.push_substring(seg.payload().copy(), absolute_seqno - _absolute_shift, seg.header().fin);

    _absolute_shift += (seg.header().syn? 1: 0);
    
    // no more data to assembled
    if(_reassembler.empty() && connection_state == FIN_RECV) {
        _absolute_shift = 2;
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(connection_state != LISTEN) {
        return WrappingInt32(wrap(_reassembler.assembled_bytes() + _absolute_shift, _isn));
    }
    else {
        return {};
    }
}

size_t TCPReceiver::window_size() const { 
    return _reassembler.stream_out().bytes_read() + _capacity - _reassembler.assembled_bytes();
}
