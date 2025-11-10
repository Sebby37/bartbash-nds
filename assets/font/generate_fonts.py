from PIL import Image, ImageDraw, ImageFont

COLS = 32

def gen_img(ttf: str, char_height: int):
    # Comic sans first
    img = Image.new("RGB", (256, 256), color=(255,0,255))
    draw = ImageDraw.Draw(img)
    font = ImageFont.truetype(ttf, char_height)
    
    ascii_start = 32
    ascii_end = 127

    for i, code in enumerate(range(ascii_start, ascii_end)):
        x = (i %  COLS) * 8
        y = (i // COLS) * char_height
        draw.text((x,y), chr(code), fill=(255,255,255), font=font)

    threshold = 100
    pixels = img.load()
    for y in range(256):
        for x in range(256):
            r,g,b = pixels[x,y]
            if (r,g,b) == (255,0,255):
                continue
            if g > threshold:
                pixels[x,y] = (255,255,255)
            else:
                pixels[x,y] = (255,0,255)

    paletted = img.convert("P", palette=Image.ADAPTIVE, colors=16)  # 16 colors max
    paletted.save(ttf + ".png")

def main():
    gen_img("ComicMono.ttf", 8)

if __name__ == "__main__":
    main()
