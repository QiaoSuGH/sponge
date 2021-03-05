#include "tcp_receiver.hh"

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    /*if(seg.header().syn){
        if(_has_syn)return;
        _has_syn = true;
        _ISN_peer = seg.header().seqno;
    }*/
    if (seg.header().syn && _has_syn)
        return;
    if (seg.header().syn) {
        _has_syn = true;
        _ISN_peer = seg.header().seqno;
    }
    /*if(seg.header().fin){
        if(!_has_syn || _has_fin)return;
        _has_syn = true;
    }*/
    if(_has_syn && seg.header().fin)_has_syn = true;

    size_t absolute_seqnum = unwrap(seg.header().seqno, _ISN_peer, _checkpoint);
    _reassembler.push_substring(seg.payload().copy(), seg.header().syn ? 0 : absolute_seqnum - 1, 
                                seg.header().fin);
    _checkpoint = absolute_seqnum;
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if(_has_syn){
        size_t tmp = 1;
        //firstly get the absolute sequence number 
            //normally just add 1 but if fin add 2 
        //then using wrap -- translating the asn to sq
        if(_has_fin && _reassembler.unassembled_bytes() == 0)tmp = 2;
        return wrap(_reassembler.left_window() + tmp, _ISN_peer);
    }
    else return std::nullopt;
 }

size_t TCPReceiver::window_size() const {return _capacity - stream_out().buffer_size(); }
