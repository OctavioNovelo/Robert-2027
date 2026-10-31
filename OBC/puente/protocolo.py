"""
Traduce las líneas serie del receptor LoRa y del LiDAR a mensajes para la UI.

Receptor:  T,<seq>,<temp>,<alt>,<pres>,<verif>,<rssi>,<snr>,<hum>,<ax>,<ay>,<az>,<gx>,<gy>,<gz>,<estado>,<t_ms>
             temp °C, alt m, pres hPa, verif 1 = BME y BNO OK, hum %RH,
             accel m/s², gyro rad/s, estado = bits (BME_OK, BNO_OK, REF_OK, ARMADO, SUPERCAP)
           X,<motivo>            (paquete descartado: len | magic | cksum | radio)
           H                     (latido cada segundo, aunque no lleguen paquetes)
           E,radio_init          (el SX1278 no arrancó)
LiDAR:     F,d0:s0,...,d63:s63   (distancia mm : target_status)
           E,init_failed
"""

ZONAS_LIDAR = 64
CAMPOS_TELEMETRIA = 16

ESTADO_BITS = {
    "bme_ok": 0,
    "bno_ok": 1,
    "ref_ok": 2,
    "armado": 3,
    "supercap": 4,
}


def _telemetria(campos):
    if len(campos) != CAMPOS_TELEMETRIA:
        return None
    seq, verif, rssi, estado, t_ms = (int(campos[i]) for i in (0, 4, 5, 14, 15))
    temp, alt, pres, snr, hum = (float(campos[i]) for i in (1, 2, 3, 6, 7))
    accel = [float(c) for c in campos[8:11]]
    gyro = [float(c) for c in campos[11:14]]
    return {
        "t": "tlm", "seq": seq, "temp": temp, "alt": alt, "pres": pres,
        "verif": verif, "rssi": rssi, "snr": snr, "hum": hum,
        "accel": accel, "gyro": gyro, "t_ms": t_ms,
        "estado": {nombre: bool(estado >> bit & 1) for nombre, bit in ESTADO_BITS.items()},
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
