#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream(const size_t capacity) { 
    _BStream = "";
    _BStream.reserve(capacity);
    _capacity = capacity;
    _totalNumPopped = _totalNumWritten = 0;
    _error = 0;
    _endInput = 0;
}

size_t ByteStream::write(const string &data) {
    if(_endInput == 1){
        set_error();
        return 0;
    }

    //3 cases none/all/partly
    if(_BStream.length() == _capacity)return 0;
    if(_BStream.length() + data.size() <= _capacity)
    {
        _BStream += data;
        _totalNumWritten += data.size();
        return data.size();
    }   
    string tmp = data.substr(0, _capacity - _BStream.size());
    _BStream += tmp;
    _totalNumWritten += tmp.size();
    return tmp.size();
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    //what if len > _BStream.size()?
    return _BStream.substr(0, len);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    /*if(len > _BStream.size())
    {
        set_error();
        return;
    }
    */
    if(len <= _BStream.size()){   
        _BStream.erase(0,len);
        _totalNumPopped += len;
    }
    else{
        _totalNumPopped += _BStream.size();
        _BStream = "";
    } 
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    /*if(len > _BStream.size())
    {
        set_error();
        return "";
    }*/
    size_t num = min(len, _BStream.size());
    string tmp = _BStream.substr(0,num);
    pop_output(len);
    return tmp;
}

void ByteStream::end_input() {
    _endInput = 1;
}

bool ByteStream::input_ended() const {
     return _endInput; 
}

size_t ByteStream::buffer_size() const { return _BStream.size(); }

bool ByteStream::buffer_empty() const { return _BStream.size() == 0; }

bool ByteStream::eof() const { 
    if(_endInput && _BStream.size() == 0)return 1;
    else return 0;
}

size_t ByteStream::bytes_written() const { return _totalNumWritten; }

size_t ByteStream::bytes_read() const { return _totalNumPopped; }

size_t ByteStream::remaining_capacity() const { return _capacity - _BStream.size(); }
