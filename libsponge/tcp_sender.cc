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
    , _stream(capacity) {}

uint64_t TCPSender::bytes_in_flight() const { return _bytes_in_flight; }

void TCPSender::fill_window() {
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

    if(_stream.buffer_empty() && _stream.input_ended() && !_FIN_sent){
        //sent FIN without payload
        TCPSegment seg;
        seg.header().syn = true;
        seg.header().seqno = wrap(_next_seqno,_isn);//to be comfirmed
        _segments_out.push(seg);
        _outstanding_segs.push(seg);
        _sender_window_size--;
        _next_seqno++;
        _FIN_sent = true;
        if(!_timer_is_running){
            _timer_is_running = true;
            _ms_accumulated = 0;
        }
        return;
    }
    
}

//! \param ackno The remote receiver's ackno (acknowledgment number)
//! \param window_size The remote receiver's advertised window size
void TCPSender::ack_received(const WrappingInt32 ackno, const uint16_t window_size) {
    

}

//! \param[in] ms_since_last_tick the number of milliseconds since the last call to this method
void TCPSender::tick(const size_t ms_since_last_tick) {
    if(_timer_is_running)_ms_accumulated += ms_since_last_tick;

}

unsigned int TCPSender::consecutive_retransmissions() const { return {}; }

void TCPSender::send_empty_segment() {}
