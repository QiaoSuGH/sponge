#include "tcp_connection.hh"

#include <iostream>

// Dummy implementation of a TCP connection

// For Lab 4, please replace with a real implementation that passes the
// automated checks run by `make check`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t TCPConnection::remaining_outbound_capacity() const { return _sender.stream_in().remaining_capacity(); }

size_t TCPConnection::bytes_in_flight() const { return _sender.bytes_in_flight();}

size_t TCPConnection::unassembled_bytes() const { return _receiver.unassembled_bytes(); }

size_t TCPConnection::time_since_last_segment_received() const { return _time_since_last_seg_received; }

void TCPConnection::clean_shutdown(){
    if(_receiver.stream_out().input_ended()){
        if(_sender.stream_in().input_ended()){
            _linger_after_streams_finish = false;
        }
        else{
            if(_sender.bytes_in_flight() == 0){
                if(_linger_after_streams_finish == false || _time_since_last_seg_received >= 10 * _cfg.rt_timeout){
                    _active = false;
                }
            }
        }
    }
}

void TCPConnection::unclean_shutdown(){
    _sender.stream_in().set_error();
    _receiver.stream_out().set_error();
    _active = false;

    //send RST
    if(_sender.segments_out().size() == 0)_sender.send_empty_segment();//make sure a seg existed
    TCPSegment seg = _sender.segments_out().front();
    _sender.segments_out().pop();

    seg.header().ack = true;
    if(_receiver.ackno().has_value())
        seg.header().ackno = _receiver.ackno().value();
    seg.header().win = _receiver.window_size();
    seg.header().rst = true;
    _segments_out.push(seg);
}


void TCPConnection::segment_received(const TCPSegment &seg) { 
    if(!_active)return;
    if(seg.header().rst){
        unclean_shutdown();
        return;
    }

    _time_since_last_seg_received = 0;

    //according to diff state to process
    //get first SYN
    if(_receiver.ackno().has_value() == false && _sender.next_seqno_absolute() == 0){
        //haven't got a SYN
        if(seg.header().syn == false)return;
        _receiver.segment_received(seg);
        connect();
        return;
    }

    //SYN sent
    if(_sender.next_seqno_absolute() > 0 && _receiver.ackno().has_value() == false &&
       _sender.bytes_in_flight() == _sender.next_seqno_absolute())
    {
        //try to establish connection with endpoints
        if(seg.payload().size() > 0)return;//SYN should not carry data
        if(seg.header().ack == false){
            //possible when trying to establish conn at the same time
            if(seg.header().syn){
                _receiver.segment_received(seg);
                _sender.send_empty_segment();//
            }
        }
        else{
            _receiver.segment_received(seg);
            _sender.ack_received(seg.header().ackno, seg.header().win);
        }
        return ;
    }

    //established
     _receiver.segment_received(seg);
    _sender.ack_received(seg.header().ackno, seg.header().win);

    send_segs_in_sender();
}

bool TCPConnection::active() const { return _active; }

void TCPConnection::send_segs_in_sender(){
    TCPSegment seg;
    while(_sender.segments_out().size())
    {
        seg = _sender.segments_out().front();
        _sender.segments_out().pop();
        if (_receiver.ackno().has_value()) {
            seg.header().ack = true;
            seg.header().ackno = _receiver.ackno().value();
            seg.header().win = _receiver.window_size();
        }
        _segments_out.push(seg);    
    }
    
}


size_t TCPConnection::write(const string &data) {
    if(data.size() == 0)return 0;

    size_t num_written_bytes = _sender.stream_in().write(data);
    _sender.fill_window();

    send_segs_in_sender();
    return num_written_bytes;

}

//! \param[in] ms_since_last_tick number of milliseconds since the last call to this method
void TCPConnection::tick(const size_t ms_since_last_tick) { 
    if (!_active)return;
    _time_since_last_seg_received += ms_since_last_tick;
    _sender.tick(ms_since_last_tick);
    if (_sender.consecutive_retransmissions() > TCPConfig::MAX_RETX_ATTEMPTS)
        unclean_shutdown();
    send_segs_in_sender();
}

void TCPConnection::end_input_stream() {
    _sender.stream_in().end_input();
    _sender.fill_window();
    send_segs_in_sender();
}

void TCPConnection::connect() {
    _sender.fill_window();
    send_segs_in_sender();
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
