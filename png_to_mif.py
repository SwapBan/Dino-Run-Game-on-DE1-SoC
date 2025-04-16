from PIL import Image

WIDTH, HEIGHT = 32, 32  # Because 16×16 = 256
INPUT_IMAGE = "sprite.png"
OUTPUT_MIF = "sprite_output.mif"

def rgb_to_rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)

def generate_mif(image):
    with open(OUTPUT_MIF, "w") as f:
        f.write("DEPTH = 1024;\n")
        f.write("WIDTH = 16;\n")
        f.write("ADDRESS_RADIX = DEC;\n")
        f.write("DATA_RADIX = HEX;\n")
        f.write("CONTENT\nBEGIN\n")
        
        idx = 0
        for y in range(HEIGHT):
            for x in range(WIDTH):
                r, g, b = image.getpixel((x, y))
                rgb565 = rgb_to_rgb565(r, g, b)
                f.write(f"{idx} : {rgb565:04x};\n")
                idx += 1
        
        f.write("END;\n")

def main():
    img = Image.open(INPUT_IMAGE).convert("RGB")
    img = img.resize((WIDTH, HEIGHT))  # or .crop((x1, y1, x2, y2)) if extracting a section
    generate_mif(img)
    print("✅ 1024-depth MIF file created.")

if __name__ == "__main__":
    main()

