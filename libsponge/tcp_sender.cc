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
    , _current_retransmission_timeout{retx_timeout}
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
    //cases that return directly
    if(_sender_window_size == 0 || _FIN_sent || (_stream.buffer_empty() && _SYN_sent && !_stream.input_ended()))
        return ;

    if(!_SYN_sent){
        //send SYN now
        TCPSegment seg;
        seg.header().syn = true;
        seg.header().seqno = _isn;
        _outstanding_segs.push(seg);
        _segments_out.push(seg);
        _bytes_in_flight = 1;
        _next_seqno = 1;
        _sender_window_size = 0;
        _timer_is_running = true;
        _ms_accumulated = 0;
        _SYN_sent = true;
        return;
    }

    if(_stream.buffer_empty() && _stream.input_ended() && !_FIN_sent && _sender_window_size > 0){
        //sent FIN without payload
        TCPSegment seg;
        seg.header().fin = true;
        seg.header().seqno = wrap(_next_seqno,_isn);//to be comfirmed
        _segments_out.push(seg);
        _outstanding_segs.push(seg);
        _sender_window_size--;
        _next_seqno++;
        _FIN_sent = true;
        _bytes_in_flight++;
        if(!_timer_is_running){
            _timer_is_running = true;
            _ms_accumulated = 0;
        }
        return;
    }

    //normal case : send seg with payload
    //_sender_window_size = _receiver_window_size;
    while(!_stream.buffer_empty() && _sender_window_size > 0){
        //the max length of next seg
        size_t _bytes_length = TCPConfig::MAX_PAYLOAD_SIZE;
        if(_bytes_length > _sender_window_size)_bytes_length = _sender_window_size;
        TCPSegment seg;
        seg.header().seqno = wrap(_next_seqno,_isn);
        seg.payload() = _stream.read(_bytes_length);

        _sender_window_size -= seg.length_in_sequence_space();
        _next_seqno += seg.length_in_sequence_space();
        
        //if _stream.eof and buffer_empty then send FIN
        if(_stream.eof() && _stream.buffer_empty() && _sender_window_size > 0){
            seg.header().fin = true;
            _next_seqno += 1;
            _FIN_sent = true;
            _sender_window_size--;
        }
        _bytes_in_flight += seg.length_in_sequence_space();
        _outstanding_segs.push(seg);
        _segments_out.push(seg);
    }
    if(!_timer_is_running){
        _timer_is_running = true;
        _ms_accumulated = 0;
    }
    return;
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    size_t abs_ackno = unwrap(ackno, _isn, _previous_ackno);
    if(abs_ackno > _next_seqno || abs_ackno < _previous_ackno)return;

    _previous_ackno = abs_ackno;
    bool _new_seg_acked = false;
    while(_outstanding_segs.size() > 0){
        TCPSegment tmp = _outstanding_segs.front();
        if(unwrap(tmp.header().seqno, _isn, _next_seqno) + tmp.length_in_sequence_space() <= abs_ackno){
            _bytes_in_flight -= tmp.length_in_sequence_space();
            _outstanding_segs.pop();
            _new_seg_acked = true;
        }else break;
    }

    if(_new_seg_acked){
        _timer_is_running = true;
        _ms_accumulated = 0;
    }
    if(_outstanding_segs.size() == 0){
        _timer_is_running = false;
        _ms_accumulated = 0;
    }

    _receiver_window_size = window_size;
    _sender_window_size = window_size + abs_ackno - _next_seqno;
    if(window_size == 0)_sender_window_size = 1;
    fill_window();

    _current_retransmission_timeout = _initial_retransmission_timeout;
    _consecutive_retransmission = 0;

    return;
}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    _ms_accumulated += ms_since_last_tick;
    if(_outstanding_segs.size() > 0 && _timer_is_running && _ms_accumulated >= _current_retransmission_timeout){
        //retransmit the front
        _segments_out.push(_outstanding_segs.front());
        _consecutive_retransmission++;
        if(_receiver_window_size != 0)_current_retransmission_timeout *= 2;
        _ms_accumulated = 0;
    }
}

unsigned int TCPSender::consecutive_retransmissions() const { return _consecutive_retransmission;}

void TCPSender::send_empty_segment() {
    TCPSegment seg;
    seg.header().seqno = wrap(_next_seqno, _isn);
    _segments_out.push(seg);
}
