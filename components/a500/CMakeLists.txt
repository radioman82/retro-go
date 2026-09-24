set(COMPONENT_SRCDIRS "." "m68k" "m68k/fame" "../retro-go/libs/miniz")
set(COMPONENT_ADD_INCLUDEDIRS "." "include" "m68k" "m68k/fame" "../retro-go/libs/miniz")
set(COMPONENT_ADD_LDFRAGMENTS "linker.lf")
set(COMPONENT_REQUIRES "retro-go")

register_component()

rg_setup_compile_options(
    -O2
    -fomit-frame-pointer
    -fno-strict-aliasing
    -fno-exceptions
    -Wno-unused-variable
    -Wno-unused-function
    -Wno-format
    -Wno-narrowing
    -Wno-char-subscripts
    -Wno-parentheses
    -Wno-register
    -Wno-implicit-fallthrough
    -Wno-sequence-point
    -Wno-error
    -DUSE_FAME_CORE
    -DUSE_FAME_CORE_C
    -DFAME_IRQ_CLOCKING
    -DFAME_CHECK_BRANCHES
    -DFAME_DIRECT_MAPPING
    -DFAME_BYPASS_TAS_WRITEBACK
    -DFAME_GLOBAL_CONTEXT
    -DFAME_FETCHBITS=8
    -DFAME_DATABITS=8
    -DFAME_NO_RESTORE_PC_MASKED_BITS
    -DNO_MAIN_IN_MAIN_C
    -DNO_THREADS
    -DUSE_ALL_LINES
    -DEMULATED_JOYSTICK
    -DFAME_INTERRUPTS_PATCH
    -DNO_VKBD
    -DOS_WITHOUT_MEMORY_MANAGEMENT
    -DRETRO_GO_EMU
    -DROM_PATH_PREFIX=\"/sd/retro-go/bios/\"
    -DDATA_PREFIX=\"/sd/retro-go/data/\"
    -DSAVE_PREFIX=\"/sd/retro-go/saves/a500/\"
)
