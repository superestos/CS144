#include "byte_stream.hh"

// Dummy implementation of a flow-controlled in-memory byte stream.

// For Lab 0, please replace with a real implementation that passes the
// automated checks run by `make check_lab0`.

// You will need to add private members to the class declaration in `byte_stream.hh`

template <typename... Targs>
void DUMMY_CODE(Targs &&... /* unused */) {}

using namespace std;

size_t ByteStream::write(const string &data) {
    size_t copy_size = std::min(remaining_capacity(), data.length());
    const char *s = data.data();
    for(size_t i=0; i<copy_size; i++) {
        buffer[front] = s[i];
        front = (front + 1) % size;
    }

    total_written += copy_size;
    return copy_size;
}

//! \param[in] len bytes will be copied from the output side of the buffer
string ByteStream::peek_output(const size_t len) const {
    std::string s;
    size_t copy_size = std::min(len, buffer_size());
    for(size_t i=0; i<copy_size; i++) {
        s.push_back(buffer[(back + i) % size]);
    }

    return s;
}

//! \param[in] len bytes will be removed from the output side of the buffer
void ByteStream::pop_output(const size_t len) { 
    size_t pop_size = std::min(len, buffer_size());
    back = (back + pop_size) % size;
    total_read += pop_size;
}

//! Read (i.e., copy and then pop) the next "len" bytes of the stream
//! \param[in] len bytes will be popped and returned
//! \returns a string
std::string ByteStream::read(const size_t len) {
    std::string s;
    size_t copy_size = std::min(len, buffer_size());
    for(size_t i=0; i<copy_size; i++) {
        s.push_back(buffer[back]);
        back = (back + 1) % size;
    }

    total_read += copy_size;
    return s;
}

void ByteStream::end_input() {
    end = true;
}

bool ByteStream::input_ended() const { 
    return end;
}

size_t ByteStream::buffer_size() const { 
    return (front - back + size) % size; 
}

bool ByteStream::buffer_empty() const { 
    return front == back;
}

bool ByteStream::eof() const { 
    return end && buffer_empty(); 
}

size_t ByteStream::bytes_written() const { 
    return total_written; 
}

size_t ByteStream::bytes_read() const { 
    return total_read; 
}

size_t ByteStream::remaining_capacity() const { 
    return size - 1 - buffer_size(); 
}
