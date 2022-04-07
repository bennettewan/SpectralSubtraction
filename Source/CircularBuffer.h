/*
  ==============================================================================

    CircularBuffer.h
    Created: 28 Feb 2022 11:24:00am
    Author:  Bennett

  ==============================================================================
*/

#pragma once
#include <JuceHeader.h>

template <class T>
class CircularBuffer
{
public: 

    CircularBuffer(int max_size) : buffer(std::make_unique<T[]>(max_size)), maxSize(max_size) {}

    CircularBuffer(const CircularBuffer& other) : CircularBuffer(other.maxSize) {}

    void enqueue(T item) {
        if (isFull())
            throw std::runtime_error("buffer is full");

        buffer[tail] = item;

        tail = (tail + 1) % maxSize;
    }

    T dequeue() {
        if (isEmpty())
            throw std::runtime_error("buffer is empty");

        T item = buffer[head];

        buffer[head] = emptyItem;

        head = (head + 1) % maxSize;

        return item;
    }

    void enqueueBuffer(T* samples, int size) {
        jassert(size() + size <= maxSize);
        
        // base case
        if (size <= 0)
            return;

        // Number of samples until end of buffer, or until we hit the head
        const unsigned numSamplesLeft = (tail >= head) ? maxSize - tail : head - tail;

        // Take the minimum of the samples left or the number of samples requested to copy
        unsigned numSamples = std::min(numSamplesLeft, size);

        // Copy to tail of buffer
        std::memcpy(buffer + tail, samples, numSamples);

        // Move tail
        tail = (tail + numSamples) % maxSize;

        // Recursive call
        enqueueBuffer(samples + numSamples, size - numSamples);
    }

    void clear() {
        head = 0;
        tail = 0;
        std::memset(buffer.get(), emptyItem, maxSize);
    }

    const T& front() {
        return buffer[head];
    }

    bool isEmpty() {
        return head == tail;
    }

    bool isFull() {
        return (tail + 1) % maxSize == head;
    }

    int size() {
        if (tail >= head)
            return tail - head;
        return maxSize - (head - tail);
    }

    T* getBuffer() {
        return buffer.get();
    }

    std::vector<T> toVector()
    {
        std::vector<T> result;
        int curr = head;
        while (curr != tail)
        {
            result.push_back(buffer[curr]);
            curr = (curr + 1) % maxSize;
        }
        return result;
    }

    T& operator[](unsigned i)
    {
        int index = (head + i) % maxSize;
        return buffer[index];
    }

    //const T& operator[](unsigned i)
    //{
    //    int index = (head + i) % maxSize;
    //    return buffer[index];
    //}

private:
    std::unique_ptr<T[]> buffer;

    int head = 0;
    int tail = 0;
    int maxSize;
    T emptyItem = 0;
};


