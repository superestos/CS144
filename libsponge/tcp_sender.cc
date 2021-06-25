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
    , _rto{retx_timeout}
    , _stream(capacity) 
    , _unack_segments() {}

uint64_t TCPSender::bytes_in_flight() const {
    return _next_seqno - _ack_seqno; 
}

bool TCPSender::isSYN() {
    return connection_state == CLOSED;
}

bool TCPSender::isFIN() {
    // FIN flag may occupies windows space
    return connection_state == SYN_SENT && _stream.eof() && _windows_size > bytes_in_flight();
}

void TCPSender::setSYN(TCPSegment &segment) {
    if (isSYN()) {
        segment.header().syn = true;
        connection_state = SYN_SENT;
        _next_seqno++;
    }
}

void TCPSender::setFIN(TCPSegment &segment) {
    if (isFIN()) {
        segment.header().fin = true;
        connection_state = FIN_SENT;
        _next_seqno++;
    }
}

void TCPSender::send_new_segment() {
    TCPSegment segment;
    segment.header().seqno = wrap(_next_seqno, _isn);

    setSYN(segment);

    size_t len = std::min(_windows_size - bytes_in_flight(), TCPConfig::MAX_PAYLOAD_SIZE);
    segment.payload() = _stream.read(len);
    _next_seqno += segment.payload().size();

    setFIN(segment);

    _segments_out.push(segment);
    _unack_segments.push_back(segment);
}

void TCPSender::fill_window() {
    bool sent = false;

    while ((bytes_in_flight() < _windows_size && !_stream.buffer_empty()) || isSYN() || isFIN()) {
        sent = true;
        send_new_segment();
    }

    if (!sent && _windows_size == 0 && bytes_in_flight() == 0) {
        sent = true;
        _windows_size = 1;
        send_new_segment();
        _windows_size = 0;
    }

    if (sent && _timer == 0) {
        _timer = _rto;
    } 
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    uint64_t ack_seqno = unwrap(ackno, _isn, _stream.bytes_read());
    _windows_size = window_size;
    if (ack_seqno > _next_seqno || ack_seqno <= _ack_seqno) {
        return;
    }

    _ack_seqno = ack_seqno;
    _consecutive_retransmissions = 0;

    for (auto it = _unack_segments.begin(); it != _unack_segments.end();) {
        auto segment = *it;
        uint64_t seqno = unwrap(segment.header().seqno, _isn, _stream.bytes_read());

        if (ack_seqno >= seqno + segment.length_in_sequence_space()) {
            _unack_segments.erase(it++);
        }
        else {
            break;
        }
    }

    _rto = _initial_retransmission_timeout;
    if (_unack_segments.empty()) {
        _timer = 0;
    }  
    else {
        _timer = _rto;
    }
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) { 
    if (_timer > ms_since_last_tick) {
        _timer -= ms_since_last_tick;
        return;
    }

    _timer = 0;
    
    if (!_unack_segments.empty()) {
        _segments_out.push(_unack_segments.front());

        if (_windows_size > 0) {
            _rto *= 2;
            _consecutive_retransmissions++;
        }

        _timer = _rto;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const {
    return _consecutive_retransmissions; 
}

void TCPSender::send_empty_segment() {
    TCPSegment segment; 
    segment.header().seqno = wrap(_next_seqno, _isn);
    
    setSYN(segment);
    //setFIN(segment);
    
    _segments_out.push(segment);
    /*
    if (segment.header().syn || segment.header().fin) {
        _unack_segments.push_back(segment);
    }
    */
}
