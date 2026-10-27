"""
Puente serie -> WebSocket para la UI del OBC.

Lee el receptor LoRa y el LiDAR por USB y reenvía cada línea, ya traducida
a JSON, a todas las pestañas de la UI conectadas a ws://localhost:8765.

    python puente.py                       # autodetecta los dos puertos
    python puente.py --rx /dev/ttyACM0 --lidar /dev/ttyUSB0

Además sirve la UI en http://localhost:8765 por si el navegador no deja que
la página de la XIAO (http://192.168.4.1) se conecte a localhost.
"""

import argparse
import asyncio
import json
import threading
import time
from http import HTTPStatus
from pathlib import Path

import serial
from serial.tools import list_ports
from websockets.asyncio.server import broadcast, serve
from websockets.datastructures import Headers
from websockets.http11 import Response

from protocolo import parse_line

BAUD = 115200
REINTENTO_S = 2
SONDEO_S = 4

# Mensajes que delatan a cada placa durante la autodetección.
ES_RECEPTOR = {"tlm", "bad", "hb"}
ES_LIDAR = {"lidar"}
ERRORES = {"radio_init": "rx", "init_failed": "lidar"}

UI = Path(__file__).resolve().parent.parent / "UI" / "index.html"


def rol_de(msg):
    if msg["t"] in ES_RECEPTOR:
        return "rx"
    if msg["t"] in ES_LIDAR:
        return "lidar"
    if msg["t"] == "err":
        return ERRORES.get(msg["msg"])
    return None


def abrir(dispositivo):
    # Con DTR/RTS en bajo antes de abrir, las ESP32 con auto-reset no se reinician.
    s = serial.Serial()
    s.port, s.baudrate, s.timeout = dispositivo, BAUD, 1
    s.dtr = s.rts = False
    s.open()
    return s


def leer_mensaje(s):
    linea = s.readline().decode("ascii", errors="replace")
    return parse_line(linea) if linea else None


class Puertos:
    """Un hilo por rol ('rx', 'lidar'); cada uno abre, lee y reconecta su puerto."""

    def __init__(self, fijos, emitir):
        self.fijos = fijos            # {"rx": "/dev/..." | None, "lidar": ...}
        self.emitir = emitir          # callback thread-safe(dict)
        self.link = {"rx": False, "lidar": False}
        self.ocupados = set()
        self.lock = threading.Lock()

    def iniciar(self):
        for rol in self.fijos:
            threading.Thread(target=self._bucle, args=(rol,), daemon=True).start()

    def _marcar(self, rol, vivo):
        self.link[rol] = vivo
        self.emitir({"t": "link", **self.link})
        print(f"[{rol}] {'conectado' if vivo else 'desconectado'}")

    def _buscar(self, rol):
        """Prueba los puertos USB libres hasta encontrar el que habla como `rol`."""
        with self.lock:
            candidatos = [p.device for p in list_ports.comports()
                          if p.vid is not None and p.device not in self.ocupados]
        for dev in candidatos:
            with self.lock:
                if dev in self.ocupados:
                    continue
                self.ocupados.add(dev)
            try:
                s = abrir(dev)
                limite = time.monotonic() + SONDEO_S
                while time.monotonic() < limite:
                    msg = leer_mensaje(s)
                    if msg and rol_de(msg):
                        if rol_de(msg) == rol:
                            return s
                        break
                s.close()
            except (serial.SerialException, OSError):
                pass
            with self.lock:
                self.ocupados.discard(dev)
        return None

    def _bucle(self, rol):
        ultimo_error = None
        while True:
            s = None
            try:
                s = abrir(self.fijos[rol]) if self.fijos[rol] else self._buscar(rol)
            except (serial.SerialException, OSError) as e:
                if str(e) != ultimo_error:
                    print(f"[{rol}] no se pudo abrir {self.fijos[rol]}: {e} (reintentando)")
                ultimo_error = str(e)
            if s is None:
                time.sleep(REINTENTO_S)
                continue

            ultimo_error = None
            print(f"[{rol}] usando {s.port}")
            self._marcar(rol, True)
            try:
                while True:
                    msg = leer_mensaje(s)
                    if msg is None or msg["t"] == "hb":
                        continue
                    if msg["t"] == "err":
                        msg["src"] = rol
                    msg["ts"] = time.time()
                    self.emitir(msg)
            except (serial.SerialException, OSError):
                pass
            finally:
                s.close()
                with self.lock:
                    self.ocupados.discard(s.port)
                self._marcar(rol, False)
            time.sleep(REINTENTO_S)


async def main(args):
    loop = asyncio.get_running_loop()
    clientes = set()
    puertos = None

    def emitir(msg):
        texto = json.dumps(msg)
        loop.call_soon_threadsafe(broadcast, clientes, texto)

    async def atender(ws):
        clientes.add(ws)
        try:
            await ws.send(json.dumps({"t": "link", **puertos.link}))
            await ws.wait_closed()
        finally:
            clientes.discard(ws)

    def pagina(conexion, peticion):
        # Peticiones normales de navegador reciben la UI; las de WebSocket siguen su curso.
        if "Upgrade" in peticion.headers:
            return None
        cuerpo = UI.read_bytes()
        cabeceras = Headers([("Content-Type", "text/html; charset=utf-8"),
                             ("Content-Length", str(len(cuerpo)))])
        return Response(HTTPStatus.OK, "OK", cabeceras, cuerpo)

    puertos = Puertos({"rx": args.rx, "lidar": args.lidar}, emitir)
    async with serve(atender, "localhost", args.port, process_request=pagina):
        print(f"Puente escuchando en ws://localhost:{args.port}  (UI en http://localhost:{args.port})")
        puertos.iniciar()
        await asyncio.Future()


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--rx", help="puerto del receptor LoRa (si se omite, se autodetecta)")
    ap.add_argument("--lidar", help="puerto del LiDAR (si se omite, se autodetecta)")
    ap.add_argument("--port", type=int, default=8765, help="puerto WebSocket (8765)")
    try:
        asyncio.run(main(ap.parse_args()))
    except KeyboardInterrupt:
        pass
