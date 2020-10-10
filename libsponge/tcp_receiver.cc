#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    if(seg.header().syn) {
        _isn = seg.header().seqno;
    }

    absolute_seqno += seg.length_in_sequence_space();
    stream_index += seg.payload().size();

    size_t index = unwrap(seg.header().seqno, _isn + absolute_seqno - stream_index, absolute_seqno);
    _reassembler.push_substring(seg.payload().copy(), index, seg.header().fin);
}

optional<WrappingInt32> TCPReceiver::ackno() const { 
    if(absolute_seqno > 0) {
        return WrappingInt32(wrap(_reassembler.assembled_bytes(), _isn + absolute_seqno - stream_index));
    }
    else {
        return {};
    }    
}

size_t TCPReceiver::window_size() const { 
    return _reassembler.stream_out().bytes_read() + _capacity - _reassembler.assembled_bytes();
}
