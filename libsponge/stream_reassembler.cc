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
    if(eof) { // dealing with the end of file
        eof_end = index + data.length();
        _eof = true;
        if(_eof && eof_end == total_assembled) {
            _output.end_input();
        }
    }

    if(total_assembled > data.length() + index) { // we have assembled this string
        return;
    }

    size_t begin = std::max(index, total_assembled);
    size_t end = std::min(index + data.length(), _output.bytes_read() + _capacity);
    if (begin >= end) {
        return;
    }
    std::string s = data.substr(begin - index, end - begin);
    

    if(ranges.count(begin) == 0 || ranges[begin] < end) {
        ranges[begin] = end;
    }

    // find string that overlap the head of current data
    auto it = ranges.find(begin);
    if (it != ranges.begin()) {
        it--;
        if(it->second >= begin) {
            if(end > it->second) {
                s = buffer[it->first].substr(0, begin - it->first) + s;
            }
            else {
                
                //s = buffer[it->first];
                //end = it->second;
                
                ranges.erase(++it);
                return;
            }
            begin = it->first;
            ranges.erase(++it);
        }
    }

    // find string that overlap the tail of current data
    it = ranges.find(begin);
    for(; it != ranges.end() && end >= it->first;) {
        if(end < it->second) {
            s += buffer[it->first].substr(end - it->first);
            end = it->second;
        }
        buffer.erase(it->first);
        ranges.erase(it++);
    }

    if(begin == total_assembled) { // put contiguous string in stream
        _output.write(s);
        total_assembled = end;
    }
    else {
        ranges[begin] = end;
        buffer[begin] = s;
    }

    if(_eof && eof_end == total_assembled) {
        _output.end_input();
    }
}

size_t StreamReassembler::unassembled_bytes() const { 
    size_t sum = 0;
    
    for(auto it: ranges) {
        sum += it.second - it.first;
    }
    return sum; 
}

bool StreamReassembler::empty() const { 
    return ranges.empty(); 
}
