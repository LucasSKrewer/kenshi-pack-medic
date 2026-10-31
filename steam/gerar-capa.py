"""Capa da Steam Workshop desenhada só com formas (sem arte do jogo): cruz médica, mochila e título."""
import sys
from PIL import Image, ImageDraw, ImageFilter, ImageFont

OUT = sys.argv[1]
SIZE = 1024


def font(size, weight="Bold", width="Condensed"):
    f = ImageFont.truetype(r"C:\Windows\Fonts\bahnschrift.ttf", size)
    try:
        f.set_variation_by_name(f"{weight} {width}" if width else weight)
    except Exception:
        try:
            f.set_variation_by_name(weight)
        except Exception:
            pass
    return f


# Fundo: deserto ao entardecer (degradê vertical) com dunas
im = Image.new("RGB", (SIZE, SIZE))
px = im.load()
top, bottom = (196, 120, 62), (58, 34, 22)
for y in range(SIZE):
    t = y / (SIZE - 1)
    c = tuple(int(top[i] + (bottom[i] - top[i]) * t) for i in range(3))
    for x in range(SIZE):
        px[x, y] = c
d = ImageDraw.Draw(im)
d.ellipse((-300, 640, 700, 1300), fill=(84, 50, 30))
d.ellipse((400, 700, 1400, 1350), fill=(70, 42, 26))
d.ellipse((380, 90, 640, 350), fill=(236, 196, 128))  # sol

# Mochila (silhueta) atrás da cruz
pack = Image.new("RGBA", (SIZE, SIZE), (0, 0, 0, 0))
pd = ImageDraw.Draw(pack)
pd.rounded_rectangle((292, 330, 732, 800), radius=70, fill=(54, 40, 30, 255), outline=(30, 22, 16, 255), width=10)
pd.rounded_rectangle((342, 560, 682, 760), radius=40, fill=(66, 50, 38, 255), outline=(30, 22, 16, 255), width=8)
pd.arc((402, 240, 622, 420), 180, 360, fill=(30, 22, 16, 255), width=26)
shadow = pack.filter(ImageFilter.GaussianBlur(14))
im.paste((0, 0, 0), (12, 18), shadow.split()[3].point(lambda a: a * 0.6))
im.paste(pack, (0, 0), pack)

# Cruz médica
d = ImageDraw.Draw(im)
cx, cy, arm, thick = 512, 470, 120, 80
d.rounded_rectangle((cx - thick // 2 - 8, cy - arm - 8, cx + thick // 2 + 8, cy + arm + 8), radius=18, fill=(245, 240, 230))
d.rounded_rectangle((cx - arm - 8, cy - thick // 2 - 8, cx + arm + 8, cy + thick // 2 + 8), radius=18, fill=(245, 240, 230))
d.rounded_rectangle((cx - thick // 2, cy - arm, cx + thick // 2, cy + arm), radius=12, fill=(178, 32, 36))
d.rounded_rectangle((cx - arm, cy - thick // 2, cx + arm, cy + thick // 2), radius=12, fill=(178, 32, 36))

# Faixa escura embaixo pro texto
band = Image.new("L", (SIZE, 260))
bp = band.load()
for y in range(260):
    a = int(220 * (y / 259) ** 1.2)
    for x in range(SIZE):
        bp[x, y] = a
im.paste((18, 12, 6), (0, SIZE - 260), band)


def centered(text, f, y, fill):
    w = d.textlength(text, font=f)
    layer = Image.new("RGBA", im.size, (0, 0, 0, 0))
    ImageDraw.Draw(layer).text(((SIZE - w) / 2 + 3, y + 4), text, font=f, fill=(0, 0, 0, 200))
    layer = layer.filter(ImageFilter.GaussianBlur(4))
    im.paste(layer, (0, 0), layer)
    ImageDraw.Draw(im).text(((SIZE - w) / 2, y), text, font=f, fill=fill)


d = ImageDraw.Draw(im)
centered("PACK MEDIC", font(150), 790, (244, 230, 200))
centered("medics restock from your pack animals", font(46, "SemiBold"), 950, (226, 196, 150))

im.quantize(colors=256, method=Image.MEDIANCUT).save(OUT, "PNG", optimize=True)
print(OUT)
