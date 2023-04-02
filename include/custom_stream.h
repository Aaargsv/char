#ifndef CUSTOM_STREAM_H
#define CUSTOM_STREAM_H

#include "chat.h"
#include <SFML/Audio.hpp>
#include <mutex>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <cstring>

#define SOUND_SIZE 44100

class CustomStream : public sf::SoundStream
{
public:
    ////////////////////////////////////////////////////////////
    /// Default constructor
    ///
    ////////////////////////////////////////////////////////////
    CustomStream(SOCKET socket):
    socket(socket),
    m_offset(0),
    m_hasFinished(false)

    {
        // Set the sound parameters
        initialize(1, 44100);
    }

    ////////////////////////////////////////////////////////////
    /// Run the server, stream audio data from the client
    ///
    ////////////////////////////////////////////////////////////
    void start(unsigned short port)
    {
        if (!m_hasFinished)
        {
            // Start playback
            play();

            // Start receiving audio data
            receiveLoop();
        }
        else
        {
            // Start playback
            play();
        }
    }

private:
    ////////////////////////////////////////////////////////////
    /// /see SoundStream::OnGetData
    ///
    ////////////////////////////////////////////////////////////
    virtual bool onGetData(sf::SoundStream::Chunk& data) override
    {
        // We have reached the end of the buffer and all audio data have been played: we can stop playback
        if ((m_offset >= m_samples.size()) && m_hasFinished)
            return false;

        // No new data has arrived since last update: wait until we get some
        while ((m_offset >= m_samples.size()) && !m_hasFinished)
            sf::sleep(sf::milliseconds(10));

        // Copy samples into a local buffer to avoid synchronization problems
        // (don't forget that we run in two separate threads)
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_tempBuffer.assign(m_samples.begin() + static_cast<std::vector<std::int16_t>::difference_type>(m_offset),
                                m_samples.end());
        }

        // Fill audio data to pass to the stream
        data.samples     = m_tempBuffer.data();
        data.sampleCount = m_tempBuffer.size();

        // Update the playing offset
        m_offset += m_tempBuffer.size();

        return true;
    }

    ////////////////////////////////////////////////////////////
    /// /see SoundStream::OnSeek
    ///
    ////////////////////////////////////////////////////////////
    void onSeek(sf::Time timeOffset) override
    {
        m_offset = static_cast<std::size_t>(timeOffset.asMilliseconds()) * getSampleRate() * getChannelCount() / 1000;
    }

    ////////////////////////////////////////////////////////////
    /// Get audio data from the client until playback is stopped
    ///
    ////////////////////////////////////////////////////////////
    void receiveLoop()
    {
        while (!m_hasFinished)
        {
            // Get waiting audio data from the network

            char sound_buffer[SOUND_SIZE];
            int error_code = recv(socket, sound_buffer, SOUND_SIZE, 0);
            if (error_code == 0 || error_code == SOCKET_ERROR)
                break;

            // Extract the message ID
            int len = 0;
            std::uint8_t id;
            id = sound_buffer[0];
            len++;
            len += 2 * ID_NAME_SIZE;
            int record_size;
            memcpy(&record_size, sound_buffer + len, sizeof(int));
            len += sizeof(int);


            if (id == SOUND_PACK)
            {
                // Extract audio samples from the packet, and append it to our samples buffer
                std::size_t sampleCount = record_size / sizeof(std::int16_t);

                // Don't forget that the other thread can access the sample array at any time
                // (so we protect any operation on it with the mutex)
                {
                    std::lock_guard<std::mutex> lock(m_mutex);
                    std::size_t     oldSize = m_samples.size();
                    m_samples.resize(oldSize + sampleCount);
                    memcpy(&(m_samples[oldSize]),
                                sound_buffer + len,
                                sampleCount * sizeof(std::int16_t));
                }
            }
            else if (id == SOUND_OFF)
            {
                // End of stream reached: we stop receiving audio data
                std::cout << "Audio data has been 100% received!" << std::endl;
                m_hasFinished = true;
            }
            else
            {
                // Something's wrong...
                std::cout << "Invalid packet received..." << std::endl;
                m_hasFinished = true;
            }
        }
    }

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////

    std::mutex                m_mutex;
    std::vector<std::int16_t> m_samples;
    std::vector<std::int16_t> m_tempBuffer;
    std::size_t               m_offset{};
    bool                      m_hasFinished{};
    SOCKET                    socket;
};
#endif //CUSTOM_STREAM_H
