#include "wrapping_integers.hh"

// Dummy implementation of a 32-bit wrapping integer

// For Lab 2, please replace with a real implementation that passes the
// automated checks run by `make check_lab2`.

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! Transform an "absolute" 64-bit sequence number (zero-indexed) into a WrappingInt32
//! \param n The input absolute 64-bit sequence number
//! \param isn The initial sequence number
WrappingInt32 wrap(uint64_t n, WrappingInt32 isn) {
    return isn + static_cast<uint32_t>(n);
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
    uint64_t result = n.raw_value() - isn.raw_value();
    result = (checkpoint & (0xFFFFFFFFUL << 32)) | (result & 0xFFFFFFFFUL);

    //get nearest seq
    uint32_t result_cut = result & 0xFFFFFFFFUL;
    uint32_t check_cut = checkpoint & 0xFFFFFFFFUL;

    if(result >> 32 && result_cut > check_cut && result_cut - check_cut > (1UL << 31)) {
        result -= 1UL << 32;
    }
    else if(result >> 32 != 0xFFFFFFFFUL && result_cut < check_cut && check_cut - result_cut > (1UL << 31)) {
        result += 1UL << 32;
    }
    return result;
}
