from protocolo import parse_line, seq_perdidos


def linea_lidar(n):
    return "F," + ",".join(f"{100 + i}:5" for i in range(n))


def test_telemetria_valida():
    assert parse_line("T,42,21,120,1013,1,-87,9.5") == {
        "t": "tlm", "seq": 42, "temp": 21, "alt": 120, "pres": 1013,
        "verif": 1, "rssi": -87, "snr": 9.5,
    }


def test_telemetria_con_salto_de_linea():
    assert parse_line("T,0,-5,0,900,0,-120,-3.25\r\n")["temp"] == -5


def test_paquete_invalido():
    assert parse_line("X,cksum") == {"t": "bad", "why": "cksum"}


def test_lidar_valido():
    m = parse_line(linea_lidar(64))
    assert m["t"] == "lidar"
    assert m["d"][0] == 100 and m["d"][63] == 163
    assert m["s"] == [5] * 64


def test_lidar_cortado():
    assert parse_line(linea_lidar(63)) is None


def test_lidar_par_malformado():
    assert parse_line(linea_lidar(63) + ",abc") is None


def test_latido():
    assert parse_line("H") == {"t": "hb"}


def test_error():
    assert parse_line("E,init_failed") == {"t": "err", "msg": "init_failed"}


def test_lineas_basura():
    for linea in ["", "basura", "T,1,2", "T,a,b,c,d,e,f,g", "X", "E", "H,1", "\x00\xff"]:
        assert parse_line(linea) is None, linea


def test_primera_linea_parcial():
    # al abrir el puerto se suele leer la cola de un frame
    assert parse_line("12:5,130:5,140:9") is None


def test_seq_perdidos():
    assert seq_perdidos(None, 5) == 0
    assert seq_perdidos(10, 11) == 0
    assert seq_perdidos(10, 13) == 2
    assert seq_perdidos(255, 0) == 0
    assert seq_perdidos(254, 1) == 2
    assert seq_perdidos(10, 10) == 0  # paquete repetido
