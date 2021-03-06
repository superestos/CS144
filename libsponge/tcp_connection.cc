#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

/*
static void debugPrintSegment(const TCPSegment &segment, std::string &&header) {
    cerr << "DEBUG: " << header;
    cerr << " len: " << segment.payload().str().size();
    cerr << " seq: " << segment.header().seqno;
    if (segment.header().ack) {
        cerr << " ack: " << segment.header().ackno;
        cerr << " win: " << segment.header().win;
    }
    if (segment.header().syn) {
        cerr << " syn ";
    }
    if (segment.header().fin) {
        cerr << " fin ";
    }
    if (segment.header().rst) {
        cerr << " rst ";
    }
    cerr << endl;
}
*/

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
    //debugPrintSegment(seg, string("recv segment"));

    _received_timer = 0;

    if(seg.header().rst) {
        reset(false);
    }

    // receiver get data
    _receiver.segment_received(seg);

    // sender get ack to continue send data
    if(seg.header().ack) {
        _sender.ack_received(seg.header().ackno, seg.header().win);
    }

    // fill window when in-stream is not empty or in-stream is eof !!!
    if (seg.header().syn || !_sender.stream_in().buffer_empty() || _sender.stream_in().input_ended()) {
        _sender.fill_window();
    }

    // ack segments that contain useful info. avoid double ack.
    if(_sender.segments_out().empty() && seg.length_in_sequence_space() > 0) {
        _sender.send_empty_segment();
    }

    while(!_sender.segments_out().empty())
        segment_sent(false);
    
    // do not linger because other side send fin first
    if(seg.header().fin) {
        if(!_fin_sent) {
            _linger_after_streams_finish = false;
        }
    }
}

void TCPConnection::segment_sent(bool rst) {
    if(_sender.segments_out().empty()) {
        return;
    }

    auto seg = _sender.segments_out().front();
    _sender.segments_out().pop();

    // append receiver ack and windows size
    if(!rst && _receiver.ackno().has_value()) {
        seg.header().ack = true;
        seg.header().ackno = _receiver.ackno().value();
        seg.header().win = _receiver.window_size();
    }
    
    seg.header().rst = rst;
    _segments_out.push(seg);
    
    if(seg.header().fin) {
        _fin_sent = true;
    }

    //debugPrintSegment(seg, string("send segment"));
}

bool TCPConnection::active() const {
    //unclean shutdown
    if (_sender.stream_in().error() && _receiver.stream_out().error()) {
        return false;
    }

    //clean shutdown
    if ( (_sender.stream_in().eof() && _sender.bytes_in_flight() == 0) &&
         (_receiver.stream_out().eof()) &&
         (!_linger_after_streams_finish || time_since_last_segment_received() >= 10 * _cfg.rt_timeout) ) {
        return false;
    }

    return true;
}

size_t TCPConnection::write(const string &data) {
    size_t len = _sender.stream_in().write(data);
    _sender.fill_window();
    while(!_sender.segments_out().empty())
        segment_sent(false);
    return len;
}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) {
    _received_timer += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    
    // too much attempts to retransmiss, abort
    if(_sender.consecutive_retransmissions() > _cfg.MAX_RETX_ATTEMPTS) {
        reset(true);
    }
    // resend segment
    else if (_sender.segments_out().size() > 0) {
        segment_sent(false);
    }
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input(); // active closed
    _sender.fill_window();
    segment_sent(false);
}

void TCPConnection::connect() {
    _sender.fill_window();
    segment_sent(false);
}

void TCPConnection::reset(bool send) {
    _receiver.stream_out().set_error();
    _sender.stream_in().set_error();
    _linger_after_streams_finish = false;

    if (send) {
        _sender.send_empty_segment();
        segment_sent(true);
    }
}

TCPConnection::~TCPConnection() {
    try {
        if (active()) {
            cerr << "Warning: Unclean shutdown of TCPConnection\n";

            // Your code here: need to send a RST segment to the peer
            reset(true);
        }
    } catch (const exception &e) {
        std::cerr << "Exception destructing TCP FSM: " << e.what() << std::endl;
    }
}
