#include "tcp_sender.hh"

#include "tcp_config.hh"

#include <random>

// Dummy implementation of a TCP sender

// For Lab 3, please replace with a real implementation that passes the
// automated checks run by `make check_lab3`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \param[in] capacity the capacity of the outgoing byte stream
//! \param[in] retx_timeout the initial amount of time to wait before retransmitting the oldest outstanding segment
//! \param[in] fixed_isn the Initial Sequence Number to use, if set (otherwise uses a random ISN)
TCPSender::TCPSender(const size_t capacity, const uint16_t retx_timeout, const std::optional<WrappingInt32> fixed_isn)
    : _isn(fixed_isn.value_or(WrappingInt32{random_device()()}))
    , _initial_retransmission_timeout{retx_timeout}
    , _retransmission_timeout{retx_timeout}
    , _stream(capacity) 
    , _unack_segments() {}

uint64_t TCPSender::bytes_in_flight() const {
    return _next_seqno - _ack_seqno; 
}

bool TCPSender::SYN_condition() {
    return connection_state == CLOSED;
}

bool TCPSender::FIN_condition() {
    return connection_state == SYN_SENT && _stream.eof() && _windows_size > bytes_in_flight();
}

void TCPSender::fill_window() {
    while((bytes_in_flight() < _windows_size && !_stream.buffer_empty()) || SYN_condition() || FIN_condition()) {
        TCPSegment segment;
        segment.header().seqno = wrap(_next_seqno, _isn);

        if(SYN_condition()) {
            segment.header().syn = true;
            connection_state = SYN_SENT;
            _next_seqno++;
        }

        size_t len = std::min(_windows_size - bytes_in_flight(), TCPConfig::MAX_PAYLOAD_SIZE);
        segment.payload() = _stream.read(len);
        _next_seqno += segment.payload().size();

        if(FIN_condition()) {
            segment.header().fin = true;
            connection_state = FIN_SENT;
            _next_seqno++;
        }

        _segments_out.push(segment);
        _unack_segments.insert({_timer + _retransmission_timeout, segment});
    }
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    _ack_seqno = unwrap(ackno, _isn, _stream.bytes_read());
    _windows_size = window_size;
    _consecutive_retransmissions = 0;

    _retransmission_timeout = _initial_retransmission_timeout;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    _timer += ms_since_last_tick;
    unsigned int double_rto = _retransmission_timeout * 2;
    
    for(auto it = _unack_segments.begin(); it != _unack_segments.end() && it->first <= _timer;) {
        auto segment = it->second;
        uint64_t seqno = unwrap(segment.header().seqno, _isn, _stream.bytes_read());
        if(seqno >= _ack_seqno) {
            _retransmission_timeout = double_rto;

            _consecutive_retransmissions++;
            _segments_out.push(segment);
            _unack_segments.insert({_timer + _retransmission_timeout, segment});
        }
        
        _unack_segments.erase(it++);
    }
}

unsigned int TCPSender::consecutive_retransmissions() const {
    return _consecutive_retransmissions; 
}

void TCPSender::send_empty_segment() {
    TCPSegment segment; 
    segment.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(segment);
}
