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
from datetime import datetime
import pygame

locale.setlocale(locale.LC_TIME, "it_IT.utf8")
os.environ["SDL_AUDIODRIVER"] = "alsa"
os.environ["AUDIODEV"] = "hw:0,0"

client = OpenAI(api_key="KEY")


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


def speak(text):
    pygame.mixer.init()
    audio_buffer = BytesIO()
    tts = gTTS(text, lang="it")
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
    cam_idx = 0
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


def write_serial(ser, text):
    if ser is None:
        log("Seriale non inizializzata", level="ERROR")
        return
    try:
        ser.write(f"{text}\n".encode())
        log(f"Messaggio inviato: {text}", level="DEBUG")
    except Exception as e:
        log(f"Errore invio seriale: {e}", level="ERROR")


# ==============================================
# SAFETY ANALYSIS
# ==============================================

def openai_responses(img_path, client, sensor_data):
    prompt = (
        "Agisci come un sistema di monitoraggio della sicurezza sul lavoro, progettato per l’ASP di Catania. "
        "Analizza i dati dei sensori e l’immagine acquisita, individuando solo rischi reali "
        "per la salute o la sicurezza dei lavoratori in base al D.Lgs. 81/2008. "
        "Linee guida da seguire:\n"
        "- Fumo: rischio oltre 30 ppm (art. 27)\n"
        "- Luce: minimo 300 lux (Allegato XXXVI)\n"
        "- Temperatura: tra 18°C e 24°C (art. 72)\n"
        "Istruzioni per la risposta:\n"
        "- Frasi brevi e chiare\n"
        "- Solo pericoli reali\n"
        "- Indicazioni preventive\n"
        "- Nessun markdown\n"
        "- Concludi con un solo livello di pericolo: LOW, MEDIUM o HIGH\n"
        "Esempio: Attenzione: rilevata Temperatura di 28°C, rischio di malesseri. Raffreddare l’ambiente. MEDIUM"
    )

    response = client.responses.create(
        model="gpt-5",
        reasoning={"effort": "low"},
        instructions=prompt,
        input=[
            {
                "role": "user",
                "content": [
                    {"type": "input_text", "text": f"Dati sensori: {sensor_data}"},
                    {
                        "type": "input_image",
                        "image_url": f"data:image/jpeg;base64,{img_to_base64(img_path)}"
                    }
                ]
            }
        ]
    )

    return response.output_text


# ==============================================
# MAIN FUNCTION
# ==============================================


if __name__ == "__main__":
    speak("Avvio safer")
    serial_com = init_serial("/dev/ttyAMA0", 115200)
    TIME = 20

    try:
        while True:
            log("Avvio ciclo di monitoraggio")

            write_serial(serial_com, "START")
            time.sleep(TIME)
            write_serial(serial_com, "PHOTO")

            while True:
                log("In attesa di dati dai sensori...")
                time.sleep(1)
                data = read_serial(serial_com)

                if data and "{" in data:
                    log(f"Dati sensori ricevuti: {data}")

                    photo = take_photo()
                    if photo:
                        level = "LOW"
                        response = openai_responses(photo, client, data)

                        log(f"Analisi sicurezza: {response} - Livello: {level}")

                        if level == "LOW":
                            log("Nessun rischio significativo rilevato")
                        if level in ("MEDIUM", "HIGH"):
                            write_serial(serial_com, "WARNING")

                            speak(response)  # Annuncia l'avviso
                        break

    except KeyboardInterrupt:
        log("Monitoraggio interrotto")
        write_serial(serial_com, "STOP")

    finally:
        log("Monitoraggio terminato")
        serial_com.close()
