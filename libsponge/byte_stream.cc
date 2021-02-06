#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream(const size_t capacity):
    _BStream(""),
    _capacity(capacity),
    _totalNumWritten(0), 
    _totalNumPopped(0),
    _inputEnded(0),
    _error(0){}


size_t ByteStream::write(const string &data) {
    if(_inputEnded == true){return 0;}

    //3 cases none/all/partly
    if(_BStream.length() == _capacity)return 0;
    size_t tmp = min(_capacity - _BStream.size(), data.size());
    _BStream += data.substr(0, tmp);
    _totalNumWritten += tmp;
    return tmp;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    size_t tmp = min(len, _BStream.size());
    return _BStream.substr(0, tmp);
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) {
    size_t tmp = min(len, _BStream.size());
    _totalNumPopped += tmp;
    _BStream.erase(0, tmp);
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    size_t tmp = min(len, _BStream.size());
    string ret = _BStream.substr(0, tmp);
    _BStream.erase(0, tmp);
    _totalNumPopped += tmp;
    return ret;
}

void ByteStream::end_input() {
    _inputEnded = true;
}

bool ByteStream::input_ended() const {
     return _inputEnded; 
}

size_t ByteStream::buffer_size() const { return _BStream.size(); }

bool ByteStream::buffer_empty() const { 
    if(_BStream.size() == 0)return true;
    return false; 
}

bool ByteStream::eof() const { 
    if(_inputEnded == true && _BStream.size() == 0)return true;
    return false;
}

size_t ByteStream::bytes_written() const { return _totalNumWritten; }

size_t ByteStream::bytes_read() const { return _totalNumPopped; }

size_t ByteStream::remaining_capacity() const { return _capacity - _BStream.size(); }
