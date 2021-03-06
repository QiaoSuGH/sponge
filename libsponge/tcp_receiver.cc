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

    /*
    if (seg.header().syn && _has_syn)
        return;
    if (seg.header().syn) {
        _has_syn = true;
        _ISN_peer = seg.header().seqno;
    }
    */
    /*if(seg.header().fin){
        if(!_has_syn || _has_fin)return;
        _has_syn = true;
    }*/
    /*
    if(_has_syn && seg.header().fin)_has_syn = true;

    size_t absolute_seqnum = unwrap(seg.header().seqno, _ISN_peer, _checkpoint);
    _reassembler.push_substring(seg.payload().copy(), seg.header().syn ? 0 : absolute_seqnum - 1, 
                                seg.header().fin);
    _checkpoint = absolute_seqnum;
    */

   const TCPHeader header = seg.header();
    WrappingInt32 seqno(header.seqno);
    if (header.syn) {
        _ISN_peer = WrappingInt32(header.seqno); 
        _has_syn = true;
        seqno = WrappingInt32(header.seqno) + 1;
    }
    size_t checkpoint = _reassembler.stream_out().bytes_written();
    size_t abs_seqno_64 = unwrap(seqno, _ISN_peer, checkpoint); // first unassembled
    size_t stream_index = abs_seqno_64 - 1;
    string data = seg.payload().copy();
    _reassembler.push_substring(data, stream_index, header.fin);

}

optional<WrappingInt32> TCPReceiver::ackno() const {
    /*if(_has_syn){
        size_t tmp = 1;
        //firstly get the absolute sequence number 
            //normally just add 1 but if fin add 2 
        //then using wrap -- translating the asn to sq
        if(_has_fin && _reassembler.unassembled_bytes() == 0)tmp = 2;
        //return wrap(_reassembler.left_window() + tmp, _ISN_peer);
        return wrap(_reassembler.stream_out().bytes_written()  + tmp, _ISN_peer);
    }
    else return std::nullopt;*/
     if (!_has_syn) return {};
    if (_reassembler.stream_out().input_ended()) { // 如果Reassembler已经组装完毕，返回fin之后的那个序号
        return WrappingInt32(wrap(_reassembler.stream_out().bytes_written() + 1, _ISN_peer)) + 1;
    }
    // first_unassembled是stream的索引，要先转换成64-bit absolute seqno（简单加1），再包装成32-bit seqno
    return WrappingInt32(wrap(_reassembler.stream_out().bytes_written() + 1, _ISN_peer)); 
}


size_t TCPReceiver::window_size() const {
    //return _capacity - stream_out().buffer_size(); 
    return _reassembler.stream_out().bytes_read() + _capacity - _reassembler.stream_out().bytes_written();
}
