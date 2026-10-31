# Antes de compilar copia la UI fuente (OBC/UI/index.html) a data/index.html,
# que es el archivo que se embebe en el firmware.
import shutil
from pathlib import Path

Import("env")

proyecto = Path(env["PROJECT_DIR"])
fuente = proyecto / ".." / "OBC" / "UI" / "index.html"
destino = proyecto / "data" / "index.html"

if fuente.exists():
    destino.parent.mkdir(exist_ok=True)
    shutil.copyfile(fuente, destino)
    print(f"UI copiada: {fuente.resolve()} -> {destino}")
else:
    print(f"Aviso: no se encontró {fuente}; se usa el data/index.html existente")
