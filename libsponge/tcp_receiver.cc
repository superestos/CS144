#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

map<TCPReceiver::ConnectionState, size_t> TCPReceiver::offset = {
    {LISTEN, 0},
    {SYN_RECV, 1},
    {FIN_WAIT, 1},
    {FIN_RECV, 2}
};

void TCPReceiver::segment_received(const TCPSegment &seg) {
    prev_state = connection_state;

    if(connection_state == LISTEN && seg.header().syn) { // handle syn and fin
        connection_state = SYN_RECV;
        _isn = seg.header().seqno;
        absolute_seqno = 0;
        stream_index = 0;
    }
    if(connection_state == SYN_RECV && seg.header().fin) {
        connection_state = FIN_WAIT;
    }

    size_t index = unwrap(seg.header().seqno, _isn, absolute_seqno);
    _reassembler.push_substring(seg.payload().copy(), index - (absolute_seqno - stream_index), seg.header().fin);

    //absolute_seqno += seg.length_in_sequence_space();
    stream_index += seg.payload().size(); // handle seq number
    absolute_seqno += (seg.header().syn ? 1 : 0) + seg.payload().size();

    if(_reassembler.empty() && connection_state == FIN_WAIT) {
        absolute_seqno += 1;
        //connection_state = FIN_RECV;
    }
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(connection_state == SYN_RECV || connection_state == FIN_WAIT) {
        return WrappingInt32(wrap(_reassembler.assembled_bytes() + (absolute_seqno - stream_index), _isn));
    }
    else {
        return {};
    }    
}

size_t TCPReceiver::window_size() const { 
    return _reassembler.stream_out().bytes_read() + _capacity - _reassembler.assembled_bytes();
}
