"""
Traduce las líneas serie del receptor LoRa y del LiDAR a mensajes para la UI.

Receptor:  T,<seq>,<temp>,<alt>,<pres>,<verif>,<rssi>,<snr>
           X,<motivo>            (paquete descartado: len | magic | cksum)
           H                     (latido cada segundo, aunque no lleguen paquetes)
           E,radio_init          (el SX1278 no arrancó)
LiDAR:     F,d0:s0,...,d63:s63   (distancia mm : target_status)
           E,init_failed
"""

ZONAS_LIDAR = 64


def _telemetria(campos):
    if len(campos) != 7:
        return None
    seq, temp, alt, pres, verif, rssi = (int(c) for c in campos[:6])
    return {
        "t": "tlm", "seq": seq, "temp": temp, "alt": alt, "pres": pres,
        "verif": verif, "rssi": rssi, "snr": float(campos[6]),
    }


def _lidar(campos):
    if len(campos) != ZONAS_LIDAR:
        return None
    d, s = [], []
    for par in campos:
        dist, status = par.split(":")
        d.append(int(dist))
        s.append(int(status))
    return {"t": "lidar", "d": d, "s": s}


def parse_line(line):
    """Devuelve el mensaje (dict) de una línea, o None si no es válida."""
    campos = line.strip().split(",")
    tipo, resto = campos[0], campos[1:]
    try:
        if tipo == "T":
            return _telemetria(resto)
        if tipo == "F":
            return _lidar(resto)
        if tipo == "H" and not resto:
            return {"t": "hb"}
        if tipo == "X" and len(resto) == 1 and resto[0]:
            return {"t": "bad", "why": resto[0]}
        if tipo == "E" and len(resto) == 1 and resto[0]:
            return {"t": "err", "msg": resto[0]}
    except ValueError:
        return None
    return None


def seq_perdidos(prev, seq):
    """Paquetes que faltan entre dos números de secuencia (8 bits, da la vuelta)."""
    if prev is None or seq == prev:
        return 0
    return (seq - prev - 1) % 256
