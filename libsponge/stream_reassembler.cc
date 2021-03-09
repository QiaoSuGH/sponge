#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

using namespace std;

StreamReassembler::StreamReassembler(const size_t capacity) : _output(capacity), _capacity(capacity) {
}

long StreamReassembler::merge_nodes(node &a, const node &b){
    //merge node a and b return merged bytes
    node x, y;//x.begin < y.begin
    if(a.begin > b.begin){
        x = b;
        y = a;
    }
    else {
        x = a;
        y = b;
    }

    if(x.begin + x.data.length() < y.begin)
        return -1;
    else if(x.begin + x.data.length() >= y.begin + y.data.length()){
        //x cover y
        a = x;
        return y.data.length();//y bytes merged
    }
    else {
        a.begin = x.begin;
        a.data = x.data + y.data.substr(x.begin + x.data.length() - y.begin);
        return x.begin + x.data.length() - y.begin;
    }
}

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    //determine if the index is in the range
    if(index >= _first_unread + _capacity)return;
    //if(data.length() == 0)return;
   
    if(index + data.length() <= _first_unread){
        if(eof)_eof = true;
        if(_eof && empty()){
            _output.end_input();
        }
        return;
    }

    node target;
    if(index < _first_unread){
        size_t tmp = _first_unread - index;
        target.data.assign(data.begin() + tmp, data.end());
        target.begin = _first_unread;
    }
    else{
        target.begin = index;
        target.data = data;
    }
    _unassembled_bytes += target.data.length();

    do{
        long merged_bytes = 0;
        auto iter = _nodes.lower_bound(target);
        while(iter != _nodes.end() && (merged_bytes = merge_nodes(target, *iter)) >= 0){
            _unassembled_bytes -= merged_bytes;
            _nodes.erase(iter);
            iter = _nodes.lower_bound(target);
        }

        if(iter == _nodes.begin())break;

        iter--;

        while((merged_bytes = merge_nodes(target, *iter)) >= 0){
            _unassembled_bytes -= merged_bytes;
            _nodes.erase(iter);
            iter = _nodes.lower_bound(target);
            if(iter == _nodes.begin())break;
            iter--;
        }
    }while(false);
        
    _nodes.insert(target);

    if(!_nodes.empty() && (*_nodes.begin()).begin == _first_unread){
        node tmp = *(_nodes.begin());
        size_t written_bytes = _output.write(tmp.data);
        _first_unread += written_bytes;
        _unassembled_bytes -= written_bytes;
        _nodes.erase(_nodes.begin());
    }
    
    if(eof)_eof = true;
    if(_eof && empty()){
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { return _unassembled_bytes; }

bool StreamReassembler::empty() const { return _unassembled_bytes == 0; }
