#include "stream_reassembler.hh"

// Dummy implementation of a stream reassembler.

// For Lab 1, please replace with a real implementation that passes the
// automated checks run by `make check_lab1`.

// You will need to add private members to the class declaration in `stream_reassembler.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

//! \details This function accepts a substring (aka a segment) of bytes,
//! possibly out-of-order, from the logical stream, and assembles any newly
//! contiguous substrings and writes them into the output stream in order.
void StreamReassembler::push_substring(const string &data, const size_t index, const bool eof) {
    size_t begin = std::max(index, total_assembled);
    size_t end = std::min(index + data.length(), _output.bytes_read() + _capacity);
    if(ranges.count(begin) == 0 || ranges[begin] < end) {
        ranges[begin] = end;
    }

    for(size_t i=begin; i<end; i++) {
        buffer[(i % _capacity)] = data[i - index];
    }

    if(begin == total_assembled) { // put contiguous string in stream
        auto it = ranges.begin();
        for(; it != ranges.end() && total_assembled >= it->first; ++it) {
            total_assembled = std::max(total_assembled, it->second);         
        }
        ranges.erase(ranges.begin(), it);

        string s;
        for(size_t i=begin; i<total_assembled; i++) {
            s.push_back(buffer[i % _capacity]);
        }
        _output.write(s);
    }

    if(eof) { // dealing with the end of file
        eof_end = index + data.length();
        _eof = true;
    }
    if(_eof && eof_end == total_assembled) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { 
    size_t sum = 0;
    auto it = ranges.begin();
    size_t end = it->first;
    for(; it != ranges.end(); ++it) {
        if(it->second > end) {
            sum += it->second - std::max(it->first, end);
        }
        end = std::max(it->second, end);
    }
    return sum; 
}

bool StreamReassembler::empty() const { 
    return ranges.empty(); 
}
