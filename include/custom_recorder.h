#ifndef CHAT_CUSTOM_RECORDER_H
#define CHAT_CUSTOM_RECORDER_H
#include <SFML/Audio.hpp>
#include <winsock2.h>
#include <ws2tcpip.h>

#define SOUND_SIZE 44100

class NetworkRecorder : public sf::SoundRecorder
{
public:

    ////////////////////////////////////////////////////////////
    /// Constructor
    ///
    /// \param host Remote host to which send the recording data
    /// \param port Port of the remote host
    ///
    ////////////////////////////////////////////////////////////
    NetworkRecorder(SOCKET socket) : socket(socket) {}

    ////////////////////////////////////////////////////////////
    /// Destructor
    ///
    /// \see SoundRecorder::~SoundRecorder()
    ///
    ////////////////////////////////////////////////////////////
    ~NetworkRecorder()
    {
        // Make sure to stop the recording thread
        stop();
    }

private:

    ////////////////////////////////////////////////////////////
    /// \see SoundRecorder::onStart
    ///
    ////////////////////////////////////////////////////////////
    virtual bool onStart()
    {

    }

    ////////////////////////////////////////////////////////////
    /// \see SoundRecorder::onProcessSamples
    ///
    ////////////////////////////////////////////////////////////
    virtual bool onProcessSamples(const sf::Int16* samples, std::size_t sampleCount)
    {
        char sound_buffer[SOUND_SIZE];
        sound_buffer[0] = 0x02; // for sound transfer
        int len = 1;
        memcpy(sound_buffer + len, &sampleCount, sizeof(int));
        len += sizeof(int);
        memcpy(sound_buffer + len, samples, sampleCount);
        int error_code = send(socket, sound_buffer, len + sound_buffer, 0);
        return (error_code != 0) && (error_code != SOCKET_ERROR)
    }

    ////////////////////////////////////////////////////////////
    /// \see SoundRecorder::onStop
    ///
    ////////////////////////////////////////////////////////////
    virtual void onStop()
    {
        // Send a "end-of-stream" packet
        sf::Packet packet;
        packet << endOfStream;
        m_socket.send(packet);

        // Close the socket
        m_socket.disconnect();


        // Send a "end-of-stream" packet
        sf::Packet packet;
        packet << clientEndOfStream;

        if (m_socket.send(packet) != sf::Socket::Status::Done)
        {
            std::cerr << "Failed to send end-of-stream packet" << std::endl;
        }

        // Close the socket
        m_socket.disconnect();
    }

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    SOCKET  socket; ///< Socket used to communicate with the server
};

#endif //CHAT_CUSTOM_RECORDER_H
