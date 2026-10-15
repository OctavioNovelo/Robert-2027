import cv2
import numpy as np


# Valores iniciales
H_MIN_INICIAL = 0
H_MAX_INICIAL = 179
S_MIN_INICIAL = 0
S_MAX_INICIAL = 255
V_MIN_INICIAL = 0
V_MAX_INICIAL = 255


def nothing(x):
    pass


def reset_trackbars():
    cv2.setTrackbarPos("H Min", "Original", H_MIN_INICIAL)
    cv2.setTrackbarPos("H Max", "Original", H_MAX_INICIAL)
    cv2.setTrackbarPos("S Min", "Original", S_MIN_INICIAL)
    cv2.setTrackbarPos("S Max", "Original", S_MAX_INICIAL)
    cv2.setTrackbarPos("V Min", "Original", V_MIN_INICIAL)
    cv2.setTrackbarPos("V Max", "Original", V_MAX_INICIAL)



# Abrir cámara
cap = cv2.VideoCapture(0)

if not cap.isOpened():
    print("Error: no se pudo abrir la cámara.")
    raise SystemExit


# Crear ventanas
cv2.namedWindow("Original", cv2.WINDOW_NORMAL)
cv2.namedWindow("Mascara Binaria", cv2.WINDOW_NORMAL)


# Crear trackbars
cv2.createTrackbar(
    "H Min", "Original",
    H_MIN_INICIAL, 179, nothing
)

cv2.createTrackbar(
    "H Max", "Original",
    H_MAX_INICIAL, 179, nothing
)

cv2.createTrackbar(
    "S Min", "Original",
    S_MIN_INICIAL, 255, nothing
)

cv2.createTrackbar (
    "S Max", "Original",
    S_MAX_INICIAL, 255, nothing
)

cv2.createTrackbar(
    "V Min", "Original",
    V_MIN_INICIAL, 255, nothing
)

cv2.createTrackbar(
    "V Max", "Original",
    V_MAX_INICIAL, 255, nothing
)


while True:

    ret, frame = cap.read()

    if not ret:
        print("Error: no se pudo leer la cámara.")
        break

    # Convertir imagen de BGR a HSV
    hsv = cv2.cvtColor(frame, cv2.COLOR_BGR2HSV)

    # Leer los valores actuales de los trackbars
    h_min = cv2.getTrackbarPos("H Min", "Original")
    h_max = cv2.getTrackbarPos("H Max", "Original")

    s_min = cv2.getTrackbarPos("S Min", "Original")
    s_max = cv2.getTrackbarPos("S Max", "Original")

    v_min = cv2.getTrackbarPos("V Min", "Original")
    v_max = cv2.getTrackbarPos("V Max", "Original")

    # Límites HSV
    lower = np.array([h_min, s_min, v_min])
    upper = np.array([h_max, s_max, v_max])

    # Crear máscara binaria
    mask = cv2.inRange(hsv, lower, upper)

    # Mostrar imágenes
    cv2.imshow("Original", frame)
    cv2.imshow("Mascara Binaria", mask)

    key = cv2.waitKey(1) & 0xFF

    # P -> imprimir valores
    if key == ord("p") or key == ord("P"):

        print(f"H_MIN = {h_min}")
        print(f"H_MAX = {h_max}")
        print(f"S_MIN = {s_min}")
        print(f"S_MAX = {s_max}")
        print(f"V_MIN = {v_min}")
        print(f"V_MAX = {v_max}")

    # R -> restablecer valores
    elif key == ord("r") or key == ord("R"):
        reset_trackbars()

    # Q o ESC -> salir
    elif key == ord("q") or key == ord("Q") or key == 27:
        break


cap.release()
cv2.destroyAllWindows()