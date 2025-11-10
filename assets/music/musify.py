import os, sys

infile = sys.argv[1]
outfile = "raw/" + os.path.basename(infile).replace(".wav", ".raw")

if os.path.exists(outfile):
    os.remove(outfile)

os.system(f'ffmpeg -i "{infile}" -ar 22050 -ac 1 -f s8 "{outfile}"')
