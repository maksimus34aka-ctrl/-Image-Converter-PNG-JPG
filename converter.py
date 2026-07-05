# converter.py
#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import sys
import argparse
import time
from PIL import Image, ImageOps
from pathlib import Path

def get_images(path, recursive=False):
    exts = ('.png', '.jpg', '.jpeg', '.bmp', '.tiff', '.webp')
    files = []
    if os.path.isfile(path):
        if path.lower().endswith(exts):
            files.append(path)
        return files
    if recursive:
        for root, _, filenames in os.walk(path):
            for f in filenames:
                if f.lower().endswith(exts):
                    files.append(os.path.join(root, f))
    else:
        for f in os.listdir(path):
            full = os.path.join(path, f)
            if os.path.isfile(full) and f.lower().endswith(exts):
                files.append(full)
    return files

def process_image(input_path, output_path, quality, size, output_format):
    img = Image.open(input_path)
    if size:
        w, h = map(int, size.split('x'))
        img.thumbnail((w, h), Image.Resampling.LANCZOS)
    # Конвертация в RGB для JPG
    if output_format.lower() in ('jpg', 'jpeg'):
        if img.mode in ('RGBA', 'LA', 'P'):
            img = img.convert('RGB')
    save_kwargs = {'quality': quality, 'optimize': True} if output_format.lower() in ('jpg', 'jpeg') else {}
    img.save(output_path, format=output_format.upper(), **save_kwargs)

def main():
    parser = argparse.ArgumentParser(description="Image Converter")
    parser.add_argument('input', help='Входной файл или папка')
    parser.add_argument('output', nargs='?', help='Выходной файл или папка')
    parser.add_argument('-q', '--quality', type=int, default=90, help='Качество JPG (1-100)')
    parser.add_argument('-r', '--recursive', action='store_true', help='Рекурсивный обход')
    parser.add_argument('-s', '--size', help='Размер WxH')
    parser.add_argument('-f', '--format', default='jpg', help='Выходной формат')
    parser.add_argument('-v', '--verbose', action='store_true', help='Подробный вывод')
    args = parser.parse_args()

    input_path = args.input
    output_path = args.output
    if not output_path:
        if os.path.isdir(input_path):
            output_path = input_path
        else:
            base = os.path.splitext(input_path)[0]
            output_path = f"{base}.{args.format}"

    files = get_images(input_path, args.recursive)
    if not files:
        print("Нет изображений для обработки.", file=sys.stderr)
        sys.exit(1)

    total = len(files)
    processed = 0
    for f in files:
        rel = os.path.relpath(f, input_path) if os.path.isdir(input_path) else os.path.basename(f)
        out_dir = output_path if os.path.isdir(output_path) else os.path.dirname(output_path)
        if os.path.isdir(input_path) and os.path.isdir(output_path):
            out_file = os.path.join(output_path, rel)
            out_dir = os.path.dirname(out_file)
        else:
            out_file = output_path
        os.makedirs(out_dir, exist_ok=True)
        # Если выходной файл не задан, генерируем
        if os.path.isdir(output_path):
            base = os.path.splitext(os.path.basename(f))[0]
            out_file = os.path.join(output_path, f"{base}.{args.format}")
            if args.recursive:
                rel_dir = os.path.relpath(os.path.dirname(f), input_path)
                if rel_dir != '.':
                    out_dir = os.path.join(output_path, rel_dir)
                    os.makedirs(out_dir, exist_ok=True)
                    out_file = os.path.join(out_dir, f"{base}.{args.format}")
        try:
            process_image(f, out_file, args.quality, args.size, args.format)
            processed += 1
            if args.verbose:
                print(f"✅ {f} -> {out_file}")
            else:
                # Прогресс
                percent = int(processed / total * 100)
                bar = '█' * (percent // 2) + '░' * (50 - percent // 2)
                print(f"\r[{bar}] {percent}% {processed}/{total}", end='', flush=True)
        except Exception as e:
            print(f"\n❌ Ошибка {f}: {e}", file=sys.stderr)
    if not args.verbose:
        print()
    print(f"✅ Обработано {processed} файлов.")

if __name__ == '__main__':
    main()
