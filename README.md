# Метод очистки цифровых изображений от стеганографических внедрений для предотвращения утечек информации

Инструментарий для исследования методов стеганографического внедрения информации в цифровые изображения. Проект выполнен в рамках научно-исследовательской работы (НИР) и включает реализацию базовых алгоритмов стеганографии, метрик оценки качества изображений и набор тестовых данных.

**Ссылка на репозиторий:** [github.com/V4L3R1K/NIR](https://github.com/V4L3R1K/NIR)

---

## Архитектура

Проект построен на архитектурном паттерне **MVC (Model-View-Controller)**:

```
src/
├── main.cpp                       # Точка входа
├── controller/                    # Контроллер (Facade)
│   └── Controller.cpp/h
├── model/
│   ├── core/                      # Базовые модели
│   │   ├── Image.cpp/h            # Работа с изображениями
│   │   └── Secret.cpp/h           # Обработка секретных данных
│   ├── steganography/             # Алгоритмы стеганографии
│   │   ├── base/StegoAlgorithm.h  # Абстрактный базовый класс
│   │   ├── lsb/LSBEncoder.cpp/h
│   │   ├── pvd/PVDEncoder.cpp/h
│   │   ├── dct/DCTEncoder.cpp/h
│   │   └── dwt/DWTEncoder.cpp/h
│   └── compare/                   # Метрики сравнения
│       ├── base/CompareAlgorithm.h
│       ├── psnr/PSNR.cpp/h
│       └── ssim/SSIM.cpp/h
└── view/
    └── cli/                       # CLI-интерфейс
        ├── CLIParser.cpp/h
        └── Command.h
```

**Ключевые архитектурные решения:**

- **Паттерн «Стратегия»** — все алгоритмы стеганографии и сравнения реализуют общий интерфейс, что позволяет легко добавлять новые алгоритмы
- **Паттерн «Фасад»** — контроллер `Controller` предоставляет единую точку входа для всех операций
- **Помехоустойчивое кодирование** — секретные данные защищаются кодами Рида — Соломона (Reed-Solomon) с возможностью коррекции до 16 ошибок на блок

---

## Реализованные алгоритмы

### Стеганографические алгоритмы

| Алгоритм | Метод | Особенности |
|----------|-------|-------------|
| **LSB** (Least Significant Bit) | Замена младших бит пикселей | Регулируемое количество бит (1–8), максимальная скорость |
| **PVD** (Pixel Value Differencing) | Внедрение в разность соседних пикселей | Адаптивная ёмкость: больше данных в текстурированных областях |
| **DCT** (Discrete Cosine Transform) | Внедрение в частотную область (JPEG) | Устойчивость к сжатию, управление качеством JPEG |
| **DWT** (Discrete Wavelet Transform) | Вейвлет-преобразование Хаара | Многоуровневая декомпозиция, выбор субдиапазонов |

### Метрики качества

| Метрика | Описание |
|---------|----------|
| **PSNR** (Peak Signal-to-Noise Ratio) | Классическая метрика на основе среднеквадратической ошибки (MSE) |
| **SSIM** (Structural Similarity Index) | Метрика структурного сходства, коррелирующая с субъективным восприятием |

---

## Использование

```
Usage: NIR [options]

Options:
  -h, --help          Show this help message and exit

Mode (one required):
  -E                  Encode mode — encode secret into container
  -D                  Decode mode — decode secret from container
  -C                  Compare mode — compare two images

Algorithm:
  -A <name>           Steganography algorithm: LSB, PVD, DCT, DWT
                      Compare algorithm:       PSNR, SSIM

Input / Output:
  -i <path>           Input container image path (for -E, -D)
                      Second -i is used as second image for -C
  -s <path>           Secret file path (for -E)
  -o <path>           Output file path (stego image / extracted secret)

Tuning (optional):
  --lsb-count <N>     Number of LSB bits to use (default: 2)
  --pvd-count <N>     Number of PVD bits to use (default: 1)
  --jpeg-quality <N>  JPEG quality for output for DCT (default: 90)
  --capacity          Print container capacity in bytes and exit

Examples:
  # Encode secret into container using LSB
  NIR -E -A LSB -i container.png -s secret.txt -o stego.png

  # Decode secret from stego image
  NIR -D -A LSB -i stego.png -o extracted.txt

  # Compare two images
  NIR -C -A PSNR -i original.png -i modified.png

  # Show container capacity
  NIR -E -A LSB -i container.png --capacity
```

---

## Сборка

### Требования

- CMake ≥ 3.20
- Компилятор с поддержкой C++17 (GCC, Clang, MSVC)
- Библиотека libjpeg
- OpenMP

### Сборка

```bash
mkdir build && cd build
cmake ..
make
```

Или с использованием скрипта:

```bash
./build.sh
```

После сборки бинарный файл `NIR` будет доступен в корневой директории проекта.

---

## Внешние зависимости

| Библиотека | Назначение | Лицензия |
|------------|------------|----------|
| **[stb_image.h](https://github.com/nothings/stb)** | Загрузка изображений (BMP, JPEG, PNG, TGA, PSD, GIF, HDR, PIC, PNM) | Public Domain (MIT) |
| **[stb_image_write.h](https://github.com/nothings/stb)** | Сохранение изображений (BMP, JPEG, PNG, TGA) | Public Domain (MIT) |
| **[libcorrect](https://github.com/quiet/libcorrect)** | Коды Рида — Соломона для помехоустойчивого кодирования | BSD-3-Clause |
| **libjpeg** | Кодирование/декодирование JPEG | Custom (BSD-like) |
| **OpenMP** | Распараллеливание вычислений | — |

Все нужные части кода (кроме libjpeg и OpenMP) включены в репозиторий в директории `third_party/`.

---

## Тестовые изображения

Сформирован репрезентативный набор тестовых изображений, охватывающий 6 категорий и 13 подкатегорий:

- **benchmark** — классические тестовые изображения (Baboon, Lenna, Peppers)
- **city** — городские сцены (архитектура, линии зданий)
- **documents** — отсканированные документы и текст (scans, text)
- **natural** — природные сцены (flowers, landscapes, water)
- **smooth** — однородные изображения и градиенты (solid colors, gradients)
- **textured** — текстуры (brick, fabric, foliage, wood)

Каждое изображение представлено в трёх форматах: **BMP**, **JPEG** и **PNG**.

Структура хранения:

```
tests/images/
├── bmp/
│   ├── benchmark/
│   ├── city/
│   ├── documents/
│   ├── natural/
│   ├── smooth/
│   └── textured/
├── jpg/
│   └── ...
└── png/
    └── ...
```

---

## Лицензия

Проект распространяется под лицензией **GNU General Public License v3.0**. Подробнее см. файл [LICENSE](LICENSE).
