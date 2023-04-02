#ifndef CHAT_CUSTOM_RECORDER_H
#define CHAT_CUSTOM_RECORDER_H

#include "chat.h"
#include <SFML/Audio.hpp>
#include <winsock2.h>
#include <ws2tcpip.h>



class CustomRecorder : public sf::SoundRecorder
{
public:

    ////////////////////////////////////////////////////////////
    /// Constructor
    ///
    /// \param host Remote host to which send the recording data
    /// \param port Port of the remote host
    ///
    ////////////////////////////////////////////////////////////
    CustomRecorder(SOCKET socket, std::string sender_name, std::string receiver_name) : socket(socket) {}

    ////////////////////////////////////////////////////////////
    /// Destructor
    ///
    /// \see SoundRecorder::~SoundRecorder()
    ///
    ////////////////////////////////////////////////////////////
    ~CustomRecorder()
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
        sound_buffer[0] = SOUND_PACK; // for sound transfer
        int len = 1;
        memcpy(sound_buffer + len, sender_name.data(), ID_NAME_SIZE);
        len += ID_NAME_SIZE;
        memcpy(sound_buffer + len, receiver_name.data(), ID_NAME_SIZE);
        len += ID_NAME_SIZE;
        memcpy(sound_buffer + len, &sampleCount, sizeof(int));
        len += sizeof(int);
        memcpy(sound_buffer + len, samples, sampleCount);
        int error_code = send(socket, sound_buffer, SOUND_SIZE, 0);
        return (error_code != 0) && (error_code != SOCKET_ERROR);
    }

    ////////////////////////////////////////////////////////////
    /// \see SoundRecorder::onStop
    ///
    ////////////////////////////////////////////////////////////
    virtual void onStop()
    {
        char sound_buffer[SOUND_SIZE];
        // Send a "end-of-stream" packet
        int len = 0;
        sound_buffer[0] = SOUND_OFF;
        len++;
        memcpy(sound_buffer + len, sender_name.data(), ID_NAME_SIZE);
        len += ID_NAME_SIZE;
        memcpy(sound_buffer + len, receiver_name.data(), ID_NAME_SIZE);
        len += ID_NAME_SIZE;
        send(socket, sound_buffer, SOUND_SIZE, 0);

    }

    ////////////////////////////////////////////////////////////
    // Member data
    ////////////////////////////////////////////////////////////
    SOCKET  socket; ///< Socket used to communicate with the server
    std::string  sender_name;
    std::string  receiver_name;
};

#endif //CHAT_CUSTOM_RECORDER_H
