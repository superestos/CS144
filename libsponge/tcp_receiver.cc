#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {

    if(connection_state == LISTEN && seg.header().syn) { // handle syn and fin
        connection_state = SYN_RECV;
        _isn = seg.header().seqno;
        absolute_shift = 0;
        stream_index = 0;
    }
    if(connection_state == SYN_RECV && seg.header().fin) {
        connection_state = FIN_RECV;
    }

    size_t index = unwrap(seg.header().seqno, _isn, stream_index);
    _reassembler.push_substring(seg.payload().copy(), index - absolute_shift, seg.header().fin);

    stream_index += seg.payload().size(); // handle seq number
    absolute_shift += (seg.header().syn ? 1 : 0);

    if(_reassembler.empty() && connection_state == FIN_RECV) {
        absolute_shift += 1;
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(connection_state != LISTEN) {
        return WrappingInt32(wrap(_reassembler.assembled_bytes() + absolute_shift, _isn));
    }
    else {
        return {};
    }    
}

size_t TCPReceiver::window_size() const { 
    return _reassembler.stream_out().bytes_read() + _capacity - _reassembler.assembled_bytes();
}
