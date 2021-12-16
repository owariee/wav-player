#!/usr/bin/env python3

from ctypes import *
import sys
import pyaudio
import math

class WavHeader(Structure):
    _fields_ = [
        # Should be "RIFF"
        ("riff_id", c_char * 4),
        # Size of the data chunk
        ("riff_size", c_int32),
        # Should be "WAVE"
        ("riff_type", c_char * 4),
        # Should be "fmt "
        ("format_id", c_char * 4),
        # Should be 16 for PCM
        ("format_size", c_int32),
        # Should be 1 for PCM or 3 for IEEE Float
        ("format_audio_type", c_int16),
        # Number os channels
        ("format_channel_num", c_int16),
        # Sample rate (normally 44Khz, 48Khz, 98Khz)
        ("format_sample_rate", c_int32),
        # Number of bytes per second = format_sampleRate * format_channel_num * format_bit_depth/8
        ("format_byte_rate", c_int32),
        # format_channel_num * format_bit_depth/8
        ("format_sample_alignment", c_int16),
         # Number of bits per sample
        ("format_bit_depth", c_int16),
        # Should be "data"
        ("data_id", c_char * 4),
        # Number of bytes in data = Number of samples * (format_channel_num * (format_bit_depth/8))
        ("data_size", c_int32)
        ]

def main():
    if len(sys.argv) != 2:
        print("At least one argument is required.")

    with open(sys.argv[1], 'rb') as wav_file:
        buffer = wav_file.read(sizeof(WavHeader))
        if len(buffer) != sizeof(WavHeader):
            print("Error reading file!")

        wav_header = WavHeader.from_buffer_copy(buffer)

        audio_device = pyaudio.PyAudio()

        audio_format = None

        if wav_header.format_audio_type == 1:
            if wav_header.format_size == 8:
                audio_format = pyaudio.paInt8
            elif wav_header.format_size == 16:
                audio_format = pyaudio.paInt16
            elif wav_header.format_size == 24:
                audio_format = pyaudio.paInt24
        elif wav_header.format_audio_type == 2:
            audio_format = pyaudio.paFloat32

        stream = audio_device.open(format=audio_format,
                                   channels=wav_header.format_channel_num,
                                   rate=wav_header.format_sample_rate,
                                   output=True)

        while wav_file.tell() < wav_header.riff_size + 8:
            if wav_file.tell() + 1024 > wav_header.riff_size + 8:
                buffer = wav_file.read(wav_header.riff_size + 8 - wav_file.tell())
            else:
                buffer = wav_file.read(1024)
            playback_percentage = math.floor((wav_file.tell()*100)/(wav_header.riff_size + 8))

            playback_bar = ""
            for i in range((30*playback_percentage)//100):
                playback_bar += "-"

            playback_bar_space = ""
            for i in range(30 - ((30*playback_percentage)//100)):
                playback_bar_space += " "

            print(" [" + playback_bar + playback_bar_space + "]", end='\r')
            stream.write(buffer)

if __name__ == "__main__":
    main()

