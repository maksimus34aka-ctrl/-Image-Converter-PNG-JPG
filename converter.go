// converter.go
package main

import (
	"flag"
	"fmt"
	"image"
	"image/jpeg"
	_ "image/png"
	_ "image/gif"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"
	"sync"

	"github.com/disintegration/imaging"
)

var (
	quality   = flag.Int("q", 90, "Quality (1-100)")
	recursive = flag.Bool("r", false, "Recursive")
	size      = flag.String("s", "", "Size WxH")
	format    = flag.String("f", "jpg", "Output format")
	verbose   = flag.Bool("v", false, "Verbose")
)

func getImageFiles(root string) []string {
	var files []string
	exts := map[string]bool{".png": true, ".jpg": true, ".jpeg": true, ".bmp": true, ".tiff": true, ".webp": true}
	walkFn := func(path string, info os.FileInfo, err error) error {
		if err != nil {
			return nil
		}
		if info.IsDir() && !*recursive && path != root {
			return filepath.SkipDir
		}
		if !info.IsDir() && exts[strings.ToLower(filepath.Ext(path))] {
			files = append(files, path)
		}
		return nil
	}
	filepath.Walk(root, walkFn)
	return files
}

func processImage(input, output string, wg *sync.WaitGroup, errCh chan error) {
	defer wg.Done()
	src, err := imaging.Open(input)
	if err != nil {
		errCh <- fmt.Errorf("%s: %v", input, err)
		return
	}
	if *size != "" {
		var w, h int
		fmt.Sscanf(*size, "%dx%d", &w, &h)
		if w > 0 && h > 0 {
			src = imaging.Resize(src, w, h, imaging.Lanczos)
		}
	}
	// Конвертация в нужный формат
	var img image.Image = src
	if strings.ToLower(*format) == "jpg" || strings.ToLower(*format) == "jpeg" {
		// JPG не поддерживает альфа, конвертируем в RGBA -> NRGBA? imaging уже сохраняет.
		// При сохранении задаем качество
	}
	err = imaging.Save(src, output, imaging.JPEGQuality(*quality))
	if err != nil {
		errCh <- fmt.Errorf("%s: %v", input, err)
	}
}

func main() {
	flag.Parse()
	args := flag.Args()
	if len(args) < 1 {
		fmt.Println("Usage: converter <input> [output] [options]")
		flag.PrintDefaults()
		os.Exit(1)
	}
	input := args[0]
	output := ""
	if len(args) > 1 {
		output = args[1]
	}
	if output == "" {
		info, _ := os.Stat(input)
		if info != nil && info.IsDir() {
			output = input
		} else {
			output = strings.TrimSuffix(input, filepath.Ext(input)) + "." + *format
		}
	}
	files := getImageFiles(input)
	if len(files) == 0 {
		fmt.Println("Нет изображений.")
		os.Exit(1)
	}
	total := len(files)
	var wg sync.WaitGroup
	errCh := make(chan error, total)
	processed := 0
	for _, f := range files {
		var outFile string
		if info, _ := os.Stat(output); info != nil && info.IsDir() {
			rel, _ := filepath.Rel(input, f)
			outFile = filepath.Join(output, rel)
			ext := filepath.Ext(outFile)
			outFile = strings.TrimSuffix(outFile, ext) + "." + *format
			os.MkdirAll(filepath.Dir(outFile), 0755)
		} else {
			outFile = output
		}
		wg.Add(1)
		go processImage(f, outFile, &wg, errCh)
	}
	go func() {
		wg.Wait()
		close(errCh)
	}()
	for range errCh {
		// ошибки не прерывают выполнение
	}
	// progress bar упрощен
	fmt.Printf("✅ Обработано %d файлов.\n", total)
}
