import sounddevice as sd
import numpy as np
import time

# ================= DSP Parameters Configuration =================
SAMPLE_RATE = 44100      # Sampling rate in Hz
CHUNK_SIZE = 4096        # Audio block size (larger = better frequency resolution, but higher latency)
VOLUME_THRESHOLD = 5.0   # Minimum volume threshold to ignore background noise

# Define the center frequencies for target notes (Hz)
# You can easily add more notes (e.g., full piano keys) here
TARGET_NOTES = {
    261.63: "C4",
    293.66: "D4",
    329.63: "E4",
    349.23: "F4",
    392.00: "G4",
    440.00: "A4",
    493.88: "B4"
}
NOTE_FREQS = list(TARGET_NOTES.keys())

# ================= Core Algorithm =================
def closest_freq(val, freq_list):
    """
    Find the closest matching frequency from the target list.
    (Adapted from the closest_freq function in your DSP report)
    """
    return min(freq_list, key=lambda x: abs(x-val))

def audio_callback(indata, frames, time_info, status):
    """
    Callback function for the microphone audio stream.
    Triggered automatically whenever CHUNK_SIZE samples are collected.
    """
    if status:
        print(f"Audio status warning: {status}")
        
    # Extract mono waveform data
    audio_data = indata[:, 0]
    
    # Calculate current volume level
    volume = np.linalg.norm(audio_data) * 10
    
    # Skip processing if it's too quiet (below threshold)
    if volume < VOLUME_THRESHOLD:
        return
        
    # Perform Fast Fourier Transform (FFT) to convert time domain to frequency domain
    # rfft is optimized for real-valued signals
    fft_mag = np.abs(np.fft.rfft(audio_data))
    freqs = np.fft.rfftfreq(len(audio_data), 1.0 / SAMPLE_RATE)
    
    # Find the frequency peak with the maximum energy
    peak_idx = np.argmax(fft_mag)
    peak_freq = freqs[peak_idx]
    
    # Filter out low-frequency noise (e.g., AC hum below 100Hz)
    if peak_freq > 100:
        # Match the detected peak frequency to the closest standard note
        matched_freq = closest_freq(peak_freq, NOTE_FREQS)
        
        # Calculate the error margin (only accept if within 15Hz tolerance)
        if abs(peak_freq - matched_freq) < 15.0:
            note_name = TARGET_NOTES[matched_freq]
            print(f"🔊 Vol: {volume:5.1f} | Peak Freq: {peak_freq:6.1f} Hz -> 🎵 Detected Note: {note_name}")
        else:
            # Print only frequency if it doesn't match our target notes closely
            print(f"🔊 Vol: {volume:5.1f} | Peak Freq: {peak_freq:6.1f} Hz -> (No matching note)")

# ================= Main Execution =================
if __name__ == "__main__":
    print("=== Raspberry Pi High-Precision Audio Frequency Detector ===")
    print(f"Sample Rate: {SAMPLE_RATE} Hz | Chunk Size: {CHUNK_SIZE}")
    print("Listening to the microphone... (Press Ctrl+C to stop)\n")
    
    try:
        # Open audio input stream (mono channel)
        with sd.InputStream(channels=1, callback=audio_callback, blocksize=CHUNK_SIZE, samplerate=SAMPLE_RATE):
            # Main thread sleeps while the background thread handles the audio callback
            while True:
                time.sleep(1)
                
    except KeyboardInterrupt:
        print("\nProgram stopped manually.")
    except Exception as e:
        print(f"\nAn error occurred: {e}")