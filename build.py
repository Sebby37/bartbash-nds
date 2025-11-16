#!/usr/bin/env python3

from architectds import *

nitrofs = NitroFS()
nitrofs.add_nflib_bg_tiled(['assets/bgtiled'], 'bg')
nitrofs.add_nflib_font(['assets/font'], 'fnt')
nitrofs.add_nflib_sprite_256(['assets/spr256'], 'spr')
nitrofs_soundbank_header = nitrofs.add_mmutil(['assets/sfx'])
nitrofs.add_files_unchanged(["assets/music/raw"], "mus") # Music is manually converted and all
nitrofs.generate_image()

arm9 = Arm9Binary(
    sourcedirs=['source'],
    libs=['nds9', 'nflib', "mm9"],
    libdirs=['${BLOCKSDS}/libs/libnds', '${BLOCKSDSEXT}/nflib', '${BLOCKSDS}/libs/maxmod']
)
arm9.add_header_dependencies([nitrofs_soundbank_header])
arm9.generate_elf()

nds = NdsRom(
    binaries=[arm9, nitrofs],
    game_title='Jolly Jab!',
    game_subtitle="Merry Christmas!",
    game_author="By SebC (demake'd SH2K)",
    game_icon="assets/jolly.bmp"
)
nds.generate_nds()

nds.run_command_line_arguments()
