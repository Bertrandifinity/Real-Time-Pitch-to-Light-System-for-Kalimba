import sounddevice as sd
import numpy as np
import time

# --- Configuration ---
DEVICE_ID = 0           # Hardware device ID from 'arecord -l'
SAMPLE_RATE = 48000     # Required by Raspberry Pi 5 I2S driver
CHUNK_SIZE = 4096       # Buffer size for FFT analysis

def audio_callback(indata, frames, time_info, status):
    """Processes incoming audio buffers to calculate RMS volume and peak frequency."""
    if status:
        print(f"Status Error: {status}")

    # Convert raw int32 data to float32 and normalize to [-1.0, 1.0]
    raw_audio = indata[:, 0].astype(np.float32)
    normalized_audio = raw_audio / 2147483648.0
    
    # Remove DC Offset (Center the waveform at zero)
    clean_audio = normalized_audio - np.mean(normalized_audio)
    
    # Calculate Root Mean Square (RMS) Volume
    volume = np.sqrt(np.mean(clean_audio**2))
    
    # Perform Fast Fourier Transform (FFT) for frequency analysis
    fft_data = np.abs(np.fft.rfft(clean_audio))
    freqs = np.fft.rfftfreq(len(clean_audio), 1.0 / SAMPLE_RATE)
    
    # Mask low-frequency noise below 500Hz
    fft_data[freqs < 500] = 0
    
    # Identify the dominant frequency
    peak_idx = np.argmax(fft_data)
    peak_freq = freqs[peak_idx]

    # Real-time console output (Overwrites same line)
    print(f"\rVolume: {volume:.6f} | Peak Frequency: {peak_freq:7.1f} Hz", end="")

print(f"Starting Microphone Monitor on Device {DEVICE_ID}...")
try:
    with sd.InputStream(device=DEVICE_ID, channels=2, samplerate=SAMPLE_RATE, 
                        blocksize=CHUNK_SIZE, dtype='int32', callback=audio_callback):
        while True:
            time.sleep(0.1)
except KeyboardInterrupt:
    print("\nMonitor stopped by user.")