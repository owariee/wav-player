#include <cstdint>
#include <fstream>
#include <thread>
#include <iostream>

#include <AL/al.h>
#include <AL/alc.h>

using namespace std::chrono_literals;

struct RiffHeader {
    uint32_t id;
    int32_t size;
};

struct PartialWAVHeader {
    RiffHeader riffHeader;
    uint32_t riffFormat;
    RiffHeader fmtHeader;
    int16_t fmtAudioFormat;
    int16_t fmtNumChannels;
    int32_t fmtSampleRate;
    int32_t fmtByteRate;
    int16_t fmtBlockAling;
    int16_t fmtBitsPerSample;
    RiffHeader listHeader;
    uint32_t listFormat;
};

typedef RiffHeader DataBlock;

int main(int argc, char* argv[]) {
    if (argc < 2) return 0;
    ALCdevice* device = alcOpenDevice(NULL);
    ALCcontext* context = alcCreateContext(device, NULL);
    alcMakeContextCurrent(context);

    uint32_t buffers[2];
    alGenBuffers(2, &buffers[0]);
    
    std::ifstream file(argv[1]);

    PartialWAVHeader pWav;
    file.read(reinterpret_cast<char*>(&pWav), sizeof(PartialWAVHeader));
    file.seekg(pWav.listHeader.size - sizeof(pWav.listFormat), std::ios_base::cur); //Skip LIST attribs

    DataBlock data;
    file.read(reinterpret_cast<char*>(&data), sizeof(DataBlock));

    bool stereo = (pWav.fmtNumChannels > 1);

    int alFormat;
    switch (pWav.fmtBitsPerSample) {
    case 16:
       alFormat = stereo ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16; break;
    case 8:
       alFormat = stereo ? AL_FORMAT_STEREO8 : AL_FORMAT_MONO8; break;
    default:
       alFormat = -1; break;
    }

    uint32_t source;
    alGenSources(1, &source);
    
    int bufferSize = 1048576;
    char* bufferData = (char*)std::malloc(bufferSize);

    for(int i = 0; i < 2; i++)
    {
        file.read(bufferData, bufferSize);
        alBufferData(buffers[i], alFormat, bufferData, bufferSize, pWav.fmtSampleRate);
    }

    alSourceQueueBuffers( source, 2, &buffers[0] );

    alSourcePlay(source);

    while(true)
    {
        // Check how much data is processed in OpenAL's internal queue
        ALint Processed;
        alGetSourcei( source, AL_BUFFERS_PROCESSED, &Processed );
        std::this_thread::sleep_for(900ms);

        // add more buffers while we need them
        while ( Processed-- )
        {
            ALuint BufID;

            alSourceUnqueueBuffers( source, 1, &BufID );

            file.read(bufferData, bufferSize);
            alBufferData(BufID, alFormat, bufferData, bufferSize, pWav.fmtSampleRate);

            alSourceQueueBuffers( source, 1, &BufID );
        }
    }

    alDeleteBuffers(2, &buffers[0]);
    alDeleteSources(1, &source);
    alcMakeContextCurrent(NULL);
    alcDestroyContext(context);
    alcCloseDevice(device);
}

