#!/usr/bin/env python3
"""
Скрипт для конвертации PNG изображений из tests/images/png/
в форматы JPG и BMP с сохранением структуры подкаталогов.
"""

import os
from pathlib import Path

from PIL import Image

PNG_ROOT = Path("tests/images/png")
JPG_ROOT = Path("tests/images/jpg")
BMP_ROOT = Path("tests/images/bmp")

JPEG_QUALITY = 95  # качество для JPG


def convert_image(png_path: Path) -> None:
    """Конвертирует один PNG-файл в JPG и BMP, сохраняя подкаталоги."""
    # Относительный путь относительно PNG_ROOT
    rel_path = png_path.relative_to(PNG_ROOT)
    stem = rel_path.stem  # имя файла без расширения

    # Целевые пути
    jpg_path = JPG_ROOT / rel_path.with_suffix(".jpg")
    bmp_path = BMP_ROOT / rel_path.with_suffix(".bmp")

    # Создаём целевые каталоги
    jpg_path.parent.mkdir(parents=True, exist_ok=True)
    bmp_path.parent.mkdir(parents=True, exist_ok=True)

    # Открываем изображение
    img = Image.open(png_path)

    # JPG не поддерживает альфа-канал — конвертируем в RGB
    if img.mode in ("RGBA", "PA", "P"):
        img_rgb = img.convert("RGB")
    else:
        img_rgb = img

    # Сохраняем JPG
    img_rgb.save(jpg_path, "JPEG", quality=JPEG_QUALITY)
    print(f"  JPG: {jpg_path}")

    # BMP поддерживает разные режимы, сохраняем как есть
    # (для RGBA сохранится с альфа-каналом, для RGB — без)
    img.save(bmp_path, "BMP")
    print(f"  BMP: {bmp_path}")


def main() -> None:
    if not PNG_ROOT.is_dir():
        print(f"Ошибка: директория {PNG_ROOT} не найдена.")
        return

    # Собираем все PNG-файлы рекурсивно
    png_files = sorted(PNG_ROOT.rglob("*.png"))

    if not png_files:
        print(f"PNG-файлы в {PNG_ROOT} не найдены.")
        return

    print(f"Найдено {len(png_files)} PNG-файлов. Начинаю конвертацию...\n")

    for png_path in png_files:
        print(f"Обработка: {png_path}")
        try:
            convert_image(png_path)
        except Exception as e:
            print(f"  ОШИБКА: {e}")

    print("\nГотово!")


if __name__ == "__main__":
    main()