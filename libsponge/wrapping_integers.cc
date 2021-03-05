#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
   uint32_t tmp = static_cast<uint32_t>(n) + isn.raw_value();
   return WrappingInt32(tmp);
}

//! Transform a WrappingInt32 into an "absolute" 64-bit sequence number (zero-indexed)
//! \param n The relative sequence number
//! \param isn The initial sequence number
//! \param checkpoint A recent absolute 64-bit sequence number
//! \returns the 64-bit sequence number that wraps to `n` and is closest to `checkpoint`
//!
//! \note Each of the two streams of the TCP connection has its own ISN. One stream
//! runs from the local TCPSender to the remote TCPReceiver and has one ISN,
//! and the other stream runs from the remote TCPSender to the local TCPReceiver and
//! has a different ISN.
uint64_t unwrap(WrappingInt32 n, WrappingInt32 isn, uint64_t checkpoint) {
   /*uint64_t tmp = checkpoint & 0xFFFFFFFF00000000;
   uint64_t mid = tmp + n.raw_value() - isn.raw_value();
   uint64_t a = 1ul << 32;
   uint64_t right = mid + a;
   uint64_t left = mid -a;
   uint64_t ret = mid;
   if(abs(int64_t(right - checkpoint)) < abs(int64_t(mid - checkpoint)))
    ret = right;
   if(mid >= a && abs(int64_t(left - checkpoint)) < abs(int64_t(ret - checkpoint)))
    ret = left;
    */
   uint32_t offset = n.raw_value() - isn.raw_value();
    uint64_t t = (checkpoint & 0xFFFFFFFF00000000) + offset;
    uint64_t ret = t;
    if (abs(int64_t(t + (1ul << 32) - checkpoint)) < abs(int64_t(t - checkpoint)))
        ret = t + (1ul << 32);
    if (t >= (1ul << 32) && abs(int64_t(t - (1ul << 32) - checkpoint)) < abs(int64_t(ret - checkpoint)))
        ret = t - (1ul << 32);
    return ret;
}
