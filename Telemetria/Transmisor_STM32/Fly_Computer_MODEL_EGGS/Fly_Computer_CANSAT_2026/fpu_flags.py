# FPU por hardware (igual que en STM32CubeIDE): se aplica a compilacion, ensamblado y enlazado.
Import("env")

fpu = ["-mfpu=fpv4-sp-d16", "-mfloat-abi=hard"]
env.Append(
    CCFLAGS=fpu,
    ASFLAGS=fpu,
    LINKFLAGS=fpu + ["--specs=nano.specs", "--specs=nosys.specs"],
)
