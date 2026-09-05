"""生成溪字红底白字托盘图标"""
from PIL import Image, ImageDraw, ImageFont
import struct
import os
import glob


def find_chinese_font():
    """查找系统中的中文字体"""
    font_paths = [
        "C:/Windows/Fonts/msyh.ttc",
        "C:/Windows/Fonts/msyh.ttf",
        "C:/Windows/Fonts/simhei.ttf",
        "C:/Windows/Fonts/simsun.ttc",
        "C:/Windows/Fonts/Deng.ttf",
        "C:/Windows/Fonts/Dengb.ttf",
        "C:/Windows/Fonts/STXIHEI.TTF",
    ]
    for p in font_paths:
        if os.path.exists(p):
            return p
    return None


def create_seal_icon(size):
    """创建红底白字"溪"印章风格图标"""
    # 抗锯齿：用 4x 大画布绘制
    scale = 4
    canvas_size = size * scale
    img = Image.new('RGBA', (canvas_size, canvas_size), (0, 0, 0, 0))
    draw = ImageDraw.Draw(img)
    
    cx, cy = canvas_size // 2, canvas_size // 2
    
    # ── 圆角红色背景 ──
    radius = int(canvas_size * 0.15)
    red_color = (200, 25, 35, 255)  # 朱砂红 #C81923
    red_dark = (160, 15, 25, 255)   # 深红阴影
    
    # 圆角矩形背景
    draw.rounded_rectangle(
        [(0, 0), (canvas_size - 1, canvas_size - 1)],
        radius=radius, fill=red_color
    )
    
    # 添加阴影边框（增强立体感）
    border_w = max(2, canvas_size // 60)
    draw.rounded_rectangle(
        [(border_w, border_w), (canvas_size - 1 - border_w, canvas_size - 1 - border_w)],
        radius=radius - border_w, outline=(140, 10, 20, 255), width=border_w
    )
    
    # ── 白色"溪"字 ──
    font_path = find_chinese_font()
    if font_path:
        # 字体大小占图标 70%
        font_size = int(canvas_size * 0.72)
        font = ImageFont.truetype(font_path, font_size)
        text = "溪"
        
        # 居中绘制
        bbox = draw.textbbox((0, 0), text, font=font)
        text_w = bbox[2] - bbox[0]
        text_h = bbox[3] - bbox[1]
        tx = (canvas_size - text_w) // 2 - bbox[0]
        ty = (canvas_size - text_h) // 2 - bbox[1] - canvas_size // 30
        
        # 阴影（轻微偏移）
        shadow_offset = max(2, canvas_size // 100)
        draw.text((tx + shadow_offset, ty + shadow_offset), text,
                 font=font, fill=(120, 5, 15, 180))
        
        # 主字
        draw.text((tx, ty), text, font=font, fill=(255, 255, 255, 255))
    
    # 缩放回目标尺寸
    img = img.resize((size, size), Image.Resampling.LANCZOS)
    return img


def build_ico_entry(img):
    size = img.width
    rgba = img.convert('RGBA')
    pixels = list(rgba.get_flattened_data())
    
    header = struct.pack('<IIIHHIIIIII',
        40, size, size * 2, 1, 32, 0,
        len(pixels) * 4, 0, 0, 0, 0)
    
    pixel_data = b''
    for y in range(size - 1, -1, -1):
        for x in range(size):
            r, g, b, a = pixels[y * size + x]
            pixel_data += struct.pack('<BBBB', b, g, r, a)
    
    and_mask_row = (size + 31) // 32 * 4
    and_mask = b'\x00' * (and_mask_row * size)
    return header + pixel_data + and_mask


def save_ico(filename, images_pil):
    entries = [(img, build_ico_entry(img)) for img in images_pil]
    header_size = 6 + len(entries) * 16
    offsets = []
    cur = header_size
    for img, data in entries:
        offsets.append(cur)
        cur += len(data)
    
    with open(filename, 'wb') as f:
        f.write(struct.pack('<HHH', 0, 1, len(entries)))
        for (img, data), off in zip(entries, offsets):
            w = img.width if img.width < 256 else 0
            h = img.height if img.height < 256 else 0
            f.write(struct.pack('<BBBBHHII', w, h, 0, 0, 1, 32, len(data), off))
        for img, data in entries:
            f.write(data)


if __name__ == '__main__':
    os.chdir('F:/project/iceClean')
    
    sizes = [16, 32, 48, 64, 128, 256]
    images = [create_seal_icon(s) for s in sizes]
    save_ico('src/gui/resources/tray.ico', images)
    for img, s in zip(images, sizes):
        img.save(f'src/gui/resources/tray_icon_{s}.png')
    print(f"tray.ico: {os.path.getsize('src/gui/resources/tray.ico'):,} bytes")
    
    # app.ico 也用同样的设计
    save_ico('src/gui/resources/app.ico', images)
    for img, s in zip(images, sizes):
        img.save(f'src/gui/resources/app_icon_{s}.png')
    print(f"app.ico: {os.path.getsize('src/gui/resources/app.ico'):,} bytes")
