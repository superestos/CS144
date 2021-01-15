#include "stream_reassembler.hh"

#include <iostream>

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
    }

    if(total_assembled > data.length() + index) {
        return;
    }

    size_t begin = std::max(index, total_assembled);
    size_t end = std::max(std::min(index + data.length(), _output.bytes_read() + _capacity), begin);    

    if(ranges.count(begin) == 0 || ranges[begin] < end) {
        ranges[begin] = end;
    }
    //string str = data.substr(begin - index, end - begin);
    //std::cout << str << std::endl;
    for(size_t i=begin; i<end; i++) {
        buffer[(i % _capacity)] = data[i - index];
    }

    auto it = ranges.find(begin);
    if (it != ranges.begin()) {
        it--;
        if(it->second >= begin) {
            begin = it->first;
            it->second = std::max(end, it->second);
            ranges.erase(++it);
        }
    }

    it = ranges.find(begin);
    for(; it != ranges.end() && end >= it->first;) {
        end = std::max(end, it->second);
        ranges.erase(it++);
    }
    ranges[begin] = end;

    if(begin == total_assembled) { // put contiguous string in stream
        total_assembled = end;
        ranges.erase(begin);

        string s;
        for(size_t i=begin; i<total_assembled; i++) {
            s.push_back(buffer[i % _capacity]);
        }
        _output.write(s);
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
