// converter.java
import java.awt.*;
import java.awt.image.*;
import java.io.*;
import java.nio.file.*;
import java.util.*;
import javax.imageio.*;
import javax.imageio.stream.*;

public class converter {
    public static void main(String[] args) throws Exception {
        String input = null, output = null;
        int quality = 90;
        boolean recursive = false;
        String size = null;
        String format = "jpg";
        boolean verbose = false;

        for (int i = 0; i < args.length; i++) {
            if (args[i].equals("-q") && i+1 < args.length) quality = Integer.parseInt(args[++i]);
            else if (args[i].equals("-r")) recursive = true;
            else if (args[i].equals("-s") && i+1 < args.length) size = args[++i];
            else if (args[i].equals("-f") && i+1 < args.length) format = args[++i];
            else if (args[i].equals("-v")) verbose = true;
            else if (args[i].equals("-h") || args[i].equals("--help")) {
                System.out.println("Usage: converter <input> [output] [options]\n  -q <N>     Quality\n  -r         Recursive\n  -s WxH     Size\n  -f <ext>   Output format\n  -v         Verbose");
                return;
            } else if (input == null) input = args[i];
            else if (output == null) output = args[i];
        }
        if (input == null) { System.err.println("Укажите входной файл или папку."); System.exit(1); }
        if (output == null) {
            if (Files.isDirectory(Paths.get(input))) output = input;
            else output = input.substring(0, input.lastIndexOf('.')) + "." + format;
        }

        String[] files = getImageFiles(input, recursive);
        if (files.length == 0) { System.out.println("Нет изображений."); System.exit(1); }
        int total = files.length, processed = 0;
        for (String f : files) {
            String outFile = output;
            if (Files.isDirectory(Paths.get(output))) {
                Path rel = Paths.get(input).relativize(Paths.get(f));
                Path outDir = Paths.get(output, rel.getParent().toString());
                Files.createDirectories(outDir);
                String base = com.google.common.io.Files.getNameWithoutExtension(f);
                outFile = outDir.resolve(base + "." + format).toString();
            }
            try {
                BufferedImage img = ImageIO.read(new File(f));
                if (img == null) throw new Exception("Unsupported format");
                if (size != null) {
                    String[] parts = size.split("x");
                    int w = Integer.parseInt(parts[0]), h = Integer.parseInt(parts[1]);
                    Image scaled = img.getScaledInstance(w, h, Image.SCALE_SMOOTH);
                    img = new BufferedImage(w, h, BufferedImage.TYPE_INT_RGB);
                    Graphics2D g = img.createGraphics();
                    g.drawImage(scaled, 0, 0, null);
                    g.dispose();
                }
                // Конвертация для JPG
                if (format.equalsIgnoreCase("jpg") || format.equalsIgnoreCase("jpeg")) {
                    BufferedImage rgb = new BufferedImage(img.getWidth(), img.getHeight(), BufferedImage.TYPE_INT_RGB);
                    rgb.getGraphics().drawImage(img, 0, 0, null);
                    img = rgb;
                }
                ImageIO.write(img, format, new File(outFile));
                processed++;
                if (verbose) System.out.println("✅ " + f + " -> " + outFile);
                else {
                    int pct = processed * 100 / total;
                    System.out.printf("\r[%s] %d%% %d/%d", "█".repeat(pct/2) + "░".repeat(50-pct/2), pct, processed, total);
                }
            } catch (Exception e) {
                System.err.println("\n❌ Ошибка " + f + ": " + e.getMessage());
            }
        }
        if (!verbose) System.out.println();
        System.out.println("✅ Обработано " + processed + " файлов.");
    }

    static String[] getImageFiles(String path, boolean recursive) {
        Set<String> exts = new HashSet<>(Arrays.asList(".png",".jpg",".jpeg",".bmp",".tiff",".webp"));
        List<String> files = new ArrayList<>();
        Path p = Paths.get(path);
        if (Files.isRegularFile(p)) {
            if (exts.contains(com.google.common.io.Files.getFileExtension(path).toLowerCase()))
                files.add(path);
            return files.toArray(new String[0]);
        }
        if (Files.isDirectory(p)) {
            try {
                Files.walk(p)
                    .filter(Files::isRegularFile)
                    .forEach(f -> {
                        String ext = com.google.common.io.Files.getFileExtension(f.toString()).toLowerCase();
                        if (exts.contains("." + ext)) files.add(f.toString());
                    });
            } catch (IOException e) {}
        }
        return files.toArray(new String[0]);
    }
}
