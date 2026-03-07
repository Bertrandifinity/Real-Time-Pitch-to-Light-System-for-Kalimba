import sounddevice as sd
import numpy as np
from gpiozero import Buzzer, LED
import time
import threading

# --- Hardware & Signal Constants ---
SAMPLE_RATE = 48000
CHUNK_SIZE = 4096
DEVICE_ID = 0

# Thresholds for detection
VOLUME_THRESHOLD = 0.005
TARGET_FREQ_MIN = 2100
TARGET_FREQ_MAX = 2500

# GPIO Pins (BCM Numbering)
BUZZER_PIN = 26
LED_PINS = [17, 27, 22, 23, 24]

# Global state for thread safety
detection_event = threading.Event()

# Initialize Hardware
buzzer = Buzzer(BUZZER_PIN)
leds = [LED(pin) for pin in LED_PINS]

def audio_callback(indata, frames, time_info, status):
    """Callback to analyze audio and flag specific alarm frequencies."""
    raw_audio = indata[:, 0].astype(np.float32)
    normalized_audio = raw_audio / 2147483648.0
    clean_audio = normalized_audio - np.mean(normalized_audio)
    
    volume = np.sqrt(np.mean(clean_audio**2))
    
    if volume > VOLUME_THRESHOLD:
        fft_data = np.abs(np.fft.rfft(clean_audio))
        freqs = np.fft.rfftfreq(len(clean_audio), 1.0 / SAMPLE_RATE)
        fft_data[freqs < 500] = 0
        
        peak_freq = freqs[np.argmax(fft_data)]
        
        # Check if peak frequency matches the active buzzer
        if TARGET_FREQ_MIN < peak_freq < TARGET_FREQ_MAX:
            detection_event.set()

def run_buzzer():
    """Background thread to cycle the active buzzer."""
    while True:
        buzzer.on()
        time.sleep(1.0)
        buzzer.off()
        time.sleep(3.0)

def main():
    print("System Active: Monitoring for 2.3kHz Alarm Signal...")
    
    # Start buzzer thread
    buzzer_thread = threading.Thread(target=run_buzzer, daemon=True)
    buzzer_thread.start()

    try:
        with sd.InputStream(device=DEVICE_ID, channels=2, samplerate=SAMPLE_RATE, 
                            blocksize=CHUNK_SIZE, dtype='int32', callback=audio_callback):
            while True:
                if detection_event.is_set():
                    print("[ALERT] Alarm sound detected! Activating LEDs.")
                    for led in leds: led.on()
                    time.sleep(0.5)
                    for led in leds: led.off()
                    detection_event.clear()
                time.sleep(0.1)
    except KeyboardInterrupt:
        print("\nShutting down system.")
    finally:
        buzzer.off()
        for led in leds: led.off()

if __name__ == "__main__":
    main()