#ifndef CIRCULAR_BUFFER_H_
#define CIRCULAR_BUFFER_H_

#include <cstddef>

/**
 * The circular buffer class is a ring of data where
 * values can be pushed onto it and will begin overwriting
 * old values once size items have been pushed on.
 *
 * Items can be accessed by index, starting from the most recently
 * pushed item and moving back in reverse chronological order.
 * If the index passed in is larger than the size of the ring,
 * it will return to the start of the ring.
 */
template<class T, size_t size>
class CircularBuffer {
public:
    CircularBuffer(): _marker(0) {
        // Buffer initialized to identity operator using copy operator
        for(size_t i=0; i<size; i++) {
            _buffer[i] = T();
        }
    }

    T& operator[](size_t index) {
        return _buffer[(_marker+index)%size];
    }

    // Move back one space and place the data in the current position
    void push(T data) {
        _marker = (_marker+size-1)%size;
        _buffer[_marker] = data;
    }

private:
    T _buffer[size];
    size_t _marker;  // marks beginning and end of buffer
};

#endif  // CIRCULAR_BUFFER_H_
