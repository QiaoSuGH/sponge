#include "tcp_receiver.hh"
#include <iostream>

// Dummy implementation of a TCP receiver

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

using namespace std;

void TCPReceiver::segment_received(const TCPSegment &seg) {
    //if(_has_fin)return;

    const TCPHeader header = seg.header();
    if(header.syn){
        if(_has_syn)return;
        _has_syn = true;
        _ISN_peer = header.seqno;
        _checkpoint = 1;
    }

    if(header.fin){
        if(!_has_syn || _has_fin)return;
        _has_fin = true;
    }

    if(seg.payload().size() > 0){
        //with data to be pushed
        //first get the abs seqno of header.seqno the translate to index
        uint32_t tmp = 0;
        if(header.syn)tmp=1;
        size_t abs_seqno = unwrap(header.seqno + tmp, _ISN_peer, _checkpoint);
        size_t index_in_stream = abs_seqno - 1;
        _reassembler.push_substring(seg.payload().copy(), index_in_stream, header.fin);
        _checkpoint = _reassembler.stream_out().bytes_written() + 1;
    }
    if(header.fin && _reassembler.unassembled_bytes() == 0)_reassembler.stream_out().end_input();
}

optional<WrappingInt32> TCPReceiver::ackno() const {
    if(_has_syn){
        size_t tmp = 1;
        //firstly get the absolute sequence number 
            //normally just add 1 but if fin add 2 
        //then using wrap -- translating the asn to sq
        if(_has_fin && _reassembler.unassembled_bytes() == 0)tmp = 2;
        return wrap(_reassembler.stream_out().bytes_written() + tmp, _ISN_peer);
    }
    else return std::nullopt;
}


size_t TCPReceiver::window_size() const {
    return _capacity - stream_out().buffer_size(); 
}
