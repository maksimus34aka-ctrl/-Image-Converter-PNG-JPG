// converter.cs
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using SixLabors.ImageSharp;
using SixLabors.ImageSharp.Processing;
using SixLabors.ImageSharp.Formats.Jpeg;

class Converter
{
    static int Main(string[] args)
    {
        string input = null, output = null;
        int quality = 90;
        bool recursive = false;
        string size = null;
        string format = "jpg";
        bool verbose = false;

        for (int i = 0; i < args.Length; i++)
        {
            if (args[i] == "-q" && i+1 < args.Length) quality = int.Parse(args[++i]);
            else if (args[i] == "-r") recursive = true;
            else if (args[i] == "-s" && i+1 < args.Length) size = args[++i];
            else if (args[i] == "-f" && i+1 < args.Length) format = args[++i];
            else if (args[i] == "-v") verbose = true;
            else if (args[i] == "-h" || args[i] == "--help")
            {
                Console.WriteLine("Usage: converter <input> [output] [options]\n  -q <N>     Quality\n  -r         Recursive\n  -s WxH     Size\n  -f <ext>   Output format\n  -v         Verbose");
                return 0;
            }
            else if (input == null) input = args[i];
            else if (output == null) output = args[i];
        }
        if (input == null) { Console.Error.WriteLine("Укажите входной файл или папку."); return 1; }
        if (output == null)
        {
            if (Directory.Exists(input)) output = input;
            else output = Path.ChangeExtension(input, format);
        }

        var files = GetImageFiles(input, recursive);
        if (files.Count == 0) { Console.WriteLine("Нет изображений."); return 1; }
        int total = files.Count, processed = 0;
        foreach (var f in files)
        {
            string outFile = output;
            if (Directory.Exists(output))
            {
                string rel = Path.GetRelativePath(input, f);
                string outDir = Path.Combine(output, Path.GetDirectoryName(rel));
                Directory.CreateDirectory(outDir);
                string baseName = Path.GetFileNameWithoutExtension(f);
                outFile = Path.Combine(outDir, baseName + "." + format);
            }
            else
            {
                // если output - файл, используем его
            }
            try
            {
                using var img = Image.Load(f);
                if (!string.IsNullOrEmpty(size))
                {
                    var parts = size.Split('x');
                    if (parts.Length == 2 && int.TryParse(parts[0], out int w) && int.TryParse(parts[1], out int h))
                        img.Mutate(ctx => ctx.Resize(w, h));
                }
                var encoder = GetEncoder(format, quality);
                img.Save(outFile, encoder);
                processed++;
                if (verbose) Console.WriteLine($"✅ {f} -> {outFile}");
                else
                {
                    int pct = processed * 100 / total;
                    Console.Write($"\r[{new string('█', pct/2)}{new string('░', 50-pct/2)}] {pct}% {processed}/{total}");
                }
            }
            catch (Exception e)
            {
                Console.WriteLine($"\n❌ Ошибка {f}: {e.Message}");
            }
        }
        if (!verbose) Console.WriteLine();
        Console.WriteLine($"✅ Обработано {processed} файлов.");
        return 0;
    }

    static List<string> GetImageFiles(string path, bool recursive)
    {
        var exts = new HashSet<string>{".png",".jpg",".jpeg",".bmp",".tiff",".webp"};
        var list = new List<string>();
        if (File.Exists(path))
        {
            if (exts.Contains(Path.GetExtension(path).ToLower())) list.Add(path);
            return list;
        }
        if (Directory.Exists(path))
        {
            var options = recursive ? SearchOption.AllDirectories : SearchOption.TopDirectoryOnly;
            foreach (var f in Directory.GetFiles(path, "*.*", options))
                if (exts.Contains(Path.GetExtension(f).ToLower()))
                    list.Add(f);
        }
        return list;
    }

    static IImageEncoder GetEncoder(string format, int quality)
    {
        format = format.ToLower();
        if (format == "jpg" || format == "jpeg")
            return new JpegEncoder { Quality = quality };
        if (format == "png")
            return new SixLabors.ImageSharp.Formats.Png.PngEncoder();
        if (format == "webp")
            return new SixLabors.ImageSharp.Formats.Webp.WebpEncoder { Quality = quality };
        // fallback
        return new JpegEncoder { Quality = quality };
    }
}
