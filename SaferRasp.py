import cv2
import base64
from openai import OpenAI
import datetime
import os
import serial
import time
from gtts import gTTS
from io import BytesIO
import inspect
import locale
import platform
from datetime import datetime
import threading
import pygame

locale.setlocale(locale.LC_TIME, "it_IT.utf8")

def get_timestamp():
    return datetime.now().strftime("%A %d %B %Y - %H:%M:%S")

# ==============================================
# UTILITIES
# ==============================================

def log(msg, level="INFO", end="\n"):
    colors = {
        "INFO": "\033[94m",  # blue
        "WARNING": "\033[93m",  # yellow
        "ERROR": "\033[91m",  # red
        "DEBUG": "\033[92m",  # green
        "RESET": "\033[0m",
    }

    caller = inspect.stack()[1].function
    color = colors.get(level.upper(), colors["INFO"])

    print(
        f"({get_timestamp()}) - {color}[{level.upper()}] {caller}(): {msg}{colors['RESET']}",
        end=end,
    )

# ==============================================
# AUDIO FUNCTIONS
# ==============================================

def speak(text, language="it"):
    pygame.mixer.init()
    audio_buffer = BytesIO()
    tts = gTTS(text, lang=language)
    tts.write_to_fp(audio_buffer)
    audio_buffer.seek(0)
    pygame.mixer.music.load(audio_buffer)
    pygame.mixer.music.play()
    while pygame.mixer.music.get_busy():
        pygame.time.Clock().tick(10)



def play_shutter_sound():
    sound = pygame.mixer.Sound("camera_shutter_sound.wav")
    sound.play()

# ==============================================
# CAMERA FUNCTIONS
# ==============================================

def find_camera(max_index=10):
    for i in range(max_index):
        try:
            print(i)
            cam = cv2.VideoCapture(i)

            if not cam.isOpened():
                raise Exception(f"Impossibile aprire la fotocamera su /dev/video{i}")

            success, frame = cam.read()
            if success and frame is not None:
                cam.release()
                log(f"Camera trovata su /dev/video{i}")
                return i
            else:
                cam.release()
                log(f"Errore: impossibile catturare il frame da /dev/video{i}")

        except Exception as e:
            log(f"Errore durante l'accesso alla fotocamera {i}: {str(e)}")
            continue

    log("Nessuna webcam disponibile trovata.")
    return None

def take_photo():
    cam_idx = find_camera()
    if cam_idx is None:
        return None

    cam = cv2.VideoCapture(cam_idx)
    if not cam.isOpened():
        log("Errore: Impossibile aprire la fotocamera")
        return None

    cam.set(cv2.CAP_PROP_FRAME_WIDTH, 1920)
    cam.set(cv2.CAP_PROP_FRAME_HEIGHT, 1080)

    success, frame = cam.read()
    if not success or frame is None:
        log("Errore: Impossibile catturare la foto")
        cam.release()
        return None

    success, img_buffer = cv2.imencode(".jpeg", frame)
    if not success:
        log("Errore: Impossibile convertire l'immagine")
        cam.release()
        return None

    photos_dir = "workplace_photos"
    os.makedirs(photos_dir, exist_ok=True)

    timestamp = get_timestamp()
    photo_path = os.path.join(photos_dir, f"workplace_{timestamp}.jpeg")

    with open(photo_path, "wb") as f:
        f.write(img_buffer.tobytes())

    cam.release()
    log(f"Foto salvata in: {photo_path}")

    return photo_path

def img_to_base64(img_path):
    with open(img_path, "rb") as f:
        return base64.b64encode(f.read()).decode("utf-8")

# ==============================================
# SERIAL FUNCTIONS
# ==============================================

def init_serial(port, baud_rate):
    try:
        ser = serial.Serial(port, baud_rate, timeout=1)
        log(f"Connessione seriale stabilita su {port}")
        return ser
    except serial.SerialException as e:
        log(f"Errore di connessione seriale: {e}")
        return None

def read_serial(ser):
    try:
        if ser.in_waiting > 0:
            return ser.readline().decode("utf-8").strip()
    except OSError as e:
        log(f"Errore di comunicazione seriale: {e}")
        return None

def serial_monitor(ser):
    while True:
        try:
            if ser.in_waiting > 0:
                line = ser.readline().decode("utf-8", errors="ignore").strip()
                if line:
                    log(line, level="DEBUG")
        except Exception as e:
            log(f"Errore lettura seriale: {e}", level="ERROR")
            break

# ==============================================
# SAFETY ANALYSIS
# ==============================================

def analyze_safety(img_path, client, sensor_data):
    prompt = (
        "Sei un sistema di monitoraggio per la sicurezza sul lavoro, progettato per l'ASP di Catania. "
        "Analizza i dati dei sensori e l'immagine per rilevare rischi secondo il D.Lgs. 81/2008. "
        "Individua solo pericoli reali per la salute o sicurezza dei lavoratori. "
        "Rispondi in modo breve e sintetico: indica il problema e come risolverlo o prevenirlo. "
        "Non usare markdown. "
        "Limiti da rispettare:\n"
        "- Fumo: rischio oltre 30 ppm (art. 27)\n"
        "- Luce: minimo 300 lux (Allegato XXXVI)\n"
        "- Temperatura: tra 18°C e 24°C (art. 72)\n"
        "Se rilevi condizioni pericolose, descrivile. "
        "Alla fine, indica un solo livello di pericolo: LOW, MEDIUM o HIGH. "
        "Esempio: Attenzione: Rilevata una Temperatura di 28°C, rischio di malesseri. Raffreddare l'ambiente. MEDIUM\n"
        f"Dati sensori:\n{sensor_data}"
    )

    response = client.responses.create(
        model="gpt-4o",
        input=[
            {
                "role": "user",
                "content": [
                    {"type": "input_text", "text": prompt},
                    {
                        "type": "input_image",
                        "image_url": f"data:image/jpeg;base64,{img_to_base64(img_path)}",
                    },
                ],
            }
        ],
        temperature=0.0,
    )

    content = response.output_text
    last_word = content.split()[-1].upper()
    danger = last_word if last_word in {"LOW", "MEDIUM", "HIGH"} else "UNKNOWN"
    message = " ".join(content.split()[:-1])

    return message, danger

# ==============================================
# MAIN FUNCTION
# ==============================================

def safety_monitor():
    SERIAL_MAIN = "/dev/ttyACM0"
    SERIAL_DEBUG = "/dev/ttyAMA0"
    client = OpenAI(api_key="API KEY")

    main_ser = init_serial(SERIAL_DEBUG, 9600)

    if main_ser is None:
        return


    try:
        while True:
            log("Avvio ciclo di monitoraggio")
            main_ser.write(bytes([1]))  # Start sensors

            time.sleep(10)

            main_ser.write(bytes([0]))  # Stop sensors

            while True:
                log("In attesa di dati dai sensori...")
                time.sleep(10)
                data = read_serial(main_ser)

                if data:
                    log(f"Dati sensori ricevuti: {data}")

                    photo = take_photo()
                    if photo:
                        analysis, level = analyze_safety(photo, client, data)

                        log(f"Analisi sicurezza: {analysis} - Livello: {level}")

                        if level == "LOW":
                            log("Nessun rischio significativo rilevato")
                        elif level in ("MEDIUM", "HIGH"):
                            main_ser.write(bytes([2]))  # Attiva allarme
                            speak(analysis)  # Annuncia l'avviso
                        break

    except KeyboardInterrupt:
        log("Monitoraggio interrotto")
        main_ser.write(bytes([0]))  # Ferma sensori

    finally:
        log("Monitoraggio terminato")
        main_ser.close()

if __name__ == "__main__":
    safety_monitor()