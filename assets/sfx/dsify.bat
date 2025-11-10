@echo off
ffmpeg -i %1 -ar 22050 -ac 1 -acodec pcm_u8 %1.wav