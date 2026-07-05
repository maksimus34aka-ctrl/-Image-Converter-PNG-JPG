// converter.cpp
#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <cmath>

using namespace std;
using namespace cv;
namespace fs = std::filesystem;

vector<string> getImageFiles(const string& path, bool recursive) {
    vector<string> files;
    vector<string> exts = {".png", ".jpg", ".jpeg", ".bmp", ".tiff", ".webp"};
    if (fs::is_regular_file(path)) {
        string ext = fs::path(path).extension().string();
        transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
        if (find(exts.begin(), exts.end(), ext) != exts.end())
            files.push_back(path);
        return files;
    }
    if (recursive) {
        for (const auto& entry : fs::recursive_directory_iterator(path)) {
            if (entry.is_regular_file()) {
                string ext = entry.path().extension().string();
                transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (find(exts.begin(), exts.end(), ext) != exts.end())
                    files.push_back(entry.path().string());
            }
        }
    } else {
        for (const auto& entry : fs::directory_iterator(path)) {
            if (entry.is_regular_file()) {
                string ext = entry.path().extension().string();
                transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                if (find(exts.begin(), exts.end(), ext) != exts.end())
                    files.push_back(entry.path().string());
            }
        }
    }
    return files;
}

bool processImage(const string& input, const string& output, int quality, const string& size, const string& format) {
    Mat img = imread(input, IMREAD_UNCHANGED);
    if (img.empty()) return false;
    if (!size.empty()) {
        int w, h;
        sscanf(size.c_str(), "%dx%d", &w, &h);
        if (w > 0 && h > 0) {
            Mat resized;
            resize(img, resized, Size(w, h), 0, 0, INTER_LANCZOS4);
            img = resized;
        }
    }
    vector<int> params;
    if (format == "jpg" || format == "jpeg") {
        params.push_back(IMWRITE_JPEG_QUALITY);
        params.push_back(quality);
        // Конвертация в RGB, если есть альфа
        if (img.channels() == 4) {
            Mat rgb;
            cvtColor(img, rgb, COLOR_BGRA2BGR);
            img = rgb;
        }
    }
    string ext = "." + format;
    if (output.find(ext) == string::npos) {
        // добавить расширение
    }
    return imwrite(output, img, params);
}

int main(int argc, char* argv[]) {
    string input, output;
    int quality = 90;
    bool recursive = false;
    string size, format = "jpg";
    bool verbose = false;

    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-q" && i+1 < argc) quality = stoi(argv[++i]);
        else if (arg == "-r") recursive = true;
        else if (arg == "-s" && i+1 < argc) size = argv[++i];
        else if (arg == "-f" && i+1 < argc) format = argv[++i];
        else if (arg == "-v") verbose = true;
        else if (arg == "-h" || arg == "--help") {
            cout << "Usage: converter <input> [output] [options]\n"
                 << "  -q <N>     Quality (1-100)\n"
                 << "  -r         Recursive\n"
                 << "  -s WxH     Size\n"
                 << "  -f <ext>   Output format\n"
                 << "  -v         Verbose\n";
            return 0;
        } else if (input.empty()) input = arg;
        else if (output.empty()) output = arg;
    }
    if (input.empty()) { cerr << "Укажите входной файл или папку." << endl; return 1; }
    if (output.empty()) {
        if (fs::is_directory(input)) output = input;
        else {
            string base = fs::path(input).stem().string();
            output = base + "." + format;
        }
    }

    auto files = getImageFiles(input, recursive);
    if (files.empty()) { cerr << "Нет изображений." << endl; return 1; }
    int total = files.size(), processed = 0;
    for (const string& f : files) {
        string out_file = output;
        if (fs::is_directory(output)) {
            string rel = fs::relative(f, input).string();
            string out_dir = fs::path(output) / fs::path(rel).parent_path();
            fs::create_directories(out_dir);
            string base = fs::path(rel).stem().string();
            out_file = (out_dir / (base + "." + format)).string();
        } else {
            // если output - файл, просто используем его
        }
        if (processImage(f, out_file, quality, size, format)) {
            processed++;
            if (verbose) cout << "✅ " << f << " -> " << out_file << endl;
        } else {
            cerr << "❌ Ошибка " << f << endl;
        }
        // Прогресс
        if (!verbose) {
            int pct = (int)(processed * 100.0 / total);
            cout << "\r[" << string(pct/2, '#') << string(50-pct/2, ' ') << "] " << pct << "% " << processed << "/" << total << flush;
        }
    }
    if (!verbose) cout << endl;
    cout << "✅ Обработано " << processed << " файлов." << endl;
    return 0;
}
