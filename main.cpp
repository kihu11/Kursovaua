#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <queue>
#include <map>
#include <bitset>
#include <windows.h>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <commdlg.h>  // Для диалога выбора файла

using namespace std;

struct LZ77Token {
    int offset;
    int length;
    char next;
};

vector<LZ77Token> compressLZ77(const string& input, int windowSize = 256, int lookAheadBufferSize = 15) {
    vector<LZ77Token> tokens;
    int i = 0;

    while (i < input.size()) {
        int maxLength = 0, bestOffset = 0;

        for (int offset = 1; offset <= min(i, windowSize); ++offset) {
            int length = 0;
            while (length < lookAheadBufferSize &&
                   input[i - offset + length] == input[i + length] &&
                   i + length < input.size()) {
                ++length;
            }
            if (length > maxLength) {
                maxLength = length;
                bestOffset = offset;
            }
        }

        char nextChar = (i + maxLength < input.size()) ? input[i + maxLength] : '\0';
        tokens.push_back({bestOffset, maxLength, nextChar});
        i += maxLength + 1;
    }

    return tokens;
}

string decompressLZ77(const vector<LZ77Token>& tokens) {
    string result;
    for (const auto& token : tokens) {
        if (token.offset == 0 && token.length == 0) {
            result += token.next;
        } else {
            int start = result.size() - token.offset;
            for (int i = 0; i < token.length; ++i) {
                result += result[start + i];
            }
            if (token.next != '\0') result += token.next;
        }
    }
    return result;
}

struct HuffNode {
    unsigned char byte;
    int freq;
    HuffNode* left;
    HuffNode* right;

    HuffNode(unsigned char b, int f) : byte(b), freq(f), left(nullptr), right(nullptr) {}
    HuffNode(HuffNode* l, HuffNode* r) : byte(0), freq(l->freq + r->freq), left(l), right(r) {}
};

struct Compare {
    bool operator()(HuffNode* a, HuffNode* b) {
        return a->freq > b->freq;
    }
};

void buildCodeMap(HuffNode* node, string path, map<unsigned char, string>& codeMap) {
    if (!node->left && !node->right) {
        codeMap[node->byte] = path;
        return;
    }
    if (node->left) buildCodeMap(node->left, path + "0", codeMap);
    if (node->right) buildCodeMap(node->right, path + "1", codeMap);
}

void deleteTree(HuffNode* node) {
    if (!node) return;
    deleteTree(node->left);
    deleteTree(node->right);
    delete node;
}

vector<unsigned char> serializeTokens(const vector<LZ77Token>& tokens) {
    vector<unsigned char> data;
    for (const auto& token : tokens) {
        data.push_back((token.offset >> 8) & 0xFF);
        data.push_back(token.offset & 0xFF);
        data.push_back(token.length);
        data.push_back(token.next);
    }
    return data;
}

vector<LZ77Token> deserializeTokens(const vector<unsigned char>& data) {
    vector<LZ77Token> tokens;
    for (size_t i = 0; i + 3 < data.size(); i += 4) {
        int offset = (data[i] << 8) | data[i + 1];
        int length = data[i + 2];
        char next = data[i + 3];
        tokens.push_back({offset, length, next});
    }
    return tokens;
}

void compressWithHuffman(const vector<unsigned char>& inputData, const string& outFile) {
    map<unsigned char, int> freq;
    for (auto b : inputData) freq[b]++;

    priority_queue<HuffNode*, vector<HuffNode*>, Compare> pq;
    for (auto& [ch, fr] : freq)
        pq.push(new HuffNode(ch, fr));

    while (pq.size() > 1) {
        HuffNode* left = pq.top(); pq.pop();
        HuffNode* right = pq.top(); pq.pop();
        pq.push(new HuffNode(left, right));
    }

    HuffNode* root = pq.top();
    map<unsigned char, string> codeMap;
    buildCodeMap(root, "", codeMap);

    ofstream out(outFile, ios::binary);
    string bitStream;
    for (auto b : inputData) bitStream += codeMap[b];
    while (bitStream.size() % 8 != 0) bitStream += '0';
    for (size_t i = 0; i < bitStream.size(); i += 8) {
        bitset<8> bits(bitStream.substr(i, 8));
        out.put((unsigned char)bits.to_ulong());
    }
    out.close();
    deleteTree(root);
}

string compressRLE(const string& input) {
    string result;
    int n = input.size();
    for (int i = 0; i < n; ++i) {
        int count = 1;
        while (i + 1 < n && input[i] == input[i + 1]) {
            ++count;
            ++i;
        }
        result += input[i];
        result += to_string(count);
    }
    return result;
}

string decompressRLE(const string& input) {
    string result;
    for (size_t i = 0; i < input.length(); ++i) {
        char ch = input[i];
        string num;
        while (i + 1 < input.length() && isdigit(input[i + 1])) {
            num += input[++i];
        }
        int count = stoi(num);
        result.append(count, ch);
    }
    return result;
}

string readFile(const string& filename) {
    ifstream in(filename);
    if (!in) {
        cerr << "Ошибка открытия файла: " << filename << endl;
        return "";
    }
    return string((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
}

void writeFile(const string& filename, const string& content) {
    ofstream out(filename);
    out << content;
    out.close();
}

long getFileSize(const string& filename) {
    ifstream in(filename, ios::binary | ios::ate);
    if (!in) return -1;
    return in.tellg();
}

// ✅ Диалог выбора файла (открытие)
string openFileDialog() {
    char filename[MAX_PATH] = "";

    OPENFILENAME ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = "Text Files\0*.txt\0All Files\0*.*\0";
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrTitle = "Выберите файл для сжатия";

    if (GetOpenFileName(&ofn)) {
        return string(filename);
    } else {
        cerr << "Файл не выбран.\n";
        return "";
    }
}

int main() {
    setlocale(LC_ALL, "ru");
    SetConsoleOutputCP(CP_UTF8);

    string inputFile = openFileDialog();
    if (inputFile.empty()) return 1;

    string compressedFile = "compressed.txt";
    string decompressedFile = "decompressed.txt";

    string text = readFile(inputFile);
    if (text.empty()) return 1;

    int method = -1;
    while (method != 0) {
        cout << "Выберите метод сжатия:\n1 - LZ77 + Huffman (аналог ZIP)\n2 - RLE\n0 - Выход из программы\nВаш выбор: ";
        cin >> method;

        if (method == 1) {
            vector<LZ77Token> tokens = compressLZ77(text);
            vector<unsigned char> rawData = serializeTokens(tokens);
            compressWithHuffman(rawData, compressedFile);
            string restoredText = decompressLZ77(tokens);
            writeFile(decompressedFile, restoredText);

        } else if (method == 2) {
            string compressed = compressRLE(text);
            string decompressed = decompressRLE(compressed);
            writeFile(compressedFile, compressed);
            writeFile(decompressedFile, decompressed);
        } else if (method == 0) {
            cout << "Конец программы.\n";
            return 0;
        } else {
            cout << "Неверный выбор. Повторите.\n";
            continue;
        }

        long originalSize = getFileSize(inputFile);
        long compressedSize = getFileSize(compressedFile);

        if (originalSize > 0 && compressedSize > 0) {
            double ratio = (double)compressedSize / originalSize;
            cout << fixed << setprecision(2);
            cout << "\nРазмер исходного файла: " << originalSize << " байт\n";
            cout << "Размер сжатого файла: " << compressedSize << " байт\n";
            cout << "Процент от первоначального файла: " << ratio * 100 << "%\n";
            cout << "Степень сжатия: " << (100 - ratio * 100) << "% уменьшение\n\n";
        } else {
            cerr << "Ошибка при измерении размеров файлов.\n";
        }
    }
}
