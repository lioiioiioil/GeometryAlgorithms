#include <png.h>
#include <algorithm>
#include <cmath>
#include <vector>
#include <filesystem>
#include <iostream>
#include <random>
#include <string>
#include <iomanip>
#include <sstream> 

// Структуры данных
struct Point {
    float x, y;
};

struct Image {
    int width, height;
    std::vector<unsigned char> pixels;
    Image(int w, int h) : width(w), height(h), pixels(w * h, 255) {}
};

enum TestType {
    TEST_BEZIER,
    TEST_CLIP_LINE,
    TEST_CLIP_POLY 
};

struct TestCase {
    std::string description;
    TestType type;
    std::vector<Point> bezierPoints; 
    std::vector<Point> clip_polygon; 
    Point lineStart, lineEnd;        
    std::vector<Point> subject_polygon; 
};

// Генератор случайных чисел
std::random_device rd;
std::mt19937 gen(rd());

float randomFloat(float min, float max) {
    std::uniform_real_distribution<float> dis(min, max);
    return dis(gen);
}

//  Математика

float distance(Point p1, Point p2) {
    return std::sqrt((p2.x - p1.x) * (p2.x - p1.x) + (p2.y - p1.y) * (p2.y - p1.y));
}

Point interpolatePoints(const Point& a, const Point& b, float t) {
    return {a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t};
}

Point calculateDisplacement(const Point& p2, const Point& p1) {
    return {p2.x - p1.x, p2.y - p1.y};
}

float scalarProduct(const Point& v1, const Point& v2) {
    return v1.x * v2.x + v1.y * v2.y;
}

// Площадь полигона (знаковая). 
// В экранных координатах (Y вниз): 
// CW (по часовой) -> Площадь > 0
// CCW (против часовой) -> Площадь < 0
float signedPolygonArea(const std::vector<Point>& polygon) {
    float area = 0.0f;
    int n = static_cast<int>(polygon.size());
    if (n < 3) return 0.0f;
    for (int i = 0; i < n; ++i) {
        Point p1 = polygon[i];
        Point p2 = polygon[(i + 1) % n];
        area += (p1.x * p2.y - p2.x * p1.y);
    }
    return area / 2.0f;
}

//  ГЕНЕРАЦИЯ ПОЛИГОНОВ 

std::vector<Point> generateRandomConvexPolygon(float centerX, float centerY, float radius, int numVertices) {
    std::vector<float> angles;
    for (int i = 0; i < numVertices; ++i) {
        angles.push_back(randomFloat(0.0f, 6.28318f)); 
    }
    std::sort(angles.begin(), angles.end());

    std::vector<Point> poly;
    for (float ang : angles) {
        poly.push_back({
            centerX + radius * std::cos(ang),
            centerY + radius * std::sin(ang)
        });
    }
    return poly;
}

std::vector<Point> generateRandomSimplePolygon(float centerX, float centerY, float minR, float maxR, int numVertices) {
    std::vector<float> angles;
    for (int i = 0; i < numVertices; ++i) {
        angles.push_back(randomFloat(0.0f, 6.28318f));
    }
    std::sort(angles.begin(), angles.end());

    std::vector<Point> poly;
    for (float ang : angles) {
        float r = randomFloat(minR, maxR);
        poly.push_back({
            centerX + r * std::cos(ang),
            centerY + r * std::sin(ang)
        });
    }
    return poly;
}

//  ГЕНЕРАЦИЯ ТЕСТОВ 

std::vector<TestCase> generateAllTests() {
    std::vector<TestCase> tests;
/*
        // 1. БАЗОВЫЕ ТЕСТЫ БЕЗЬЕ
tests.push_back({"Bez_01_Linear", TEST_BEZIER, {{10, 10}, {30, 30}, {60, 60}, {90, 90}}, {}, {}, {}});
tests.push_back({"Bez_02_Arch", TEST_BEZIER, {{10, 10}, {10, 90}, {90, 90}, {90, 10}}, {}, {}, {}});
tests.push_back({"Bez_03_S_Shape_Vert", TEST_BEZIER, {{50, 10}, {90, 10}, {10, 90}, {50, 90}}, {}, {}, {}});
tests.push_back({"Bez_04_Loop", TEST_BEZIER, {{10, 10}, {90, 10}, {10, 90}, {90, 90}}, {}, {}, {}});
tests.push_back({"Bez_05_Cusp", TEST_BEZIER, {{10, 90}, {90, 90}, {10, 10}, {90, 10}}, {}, {}, {}});
tests.push_back({"Bez_06_U_Shape", TEST_BEZIER, {{10, 90}, {10, 10}, {90, 10}, {90, 90}}, {}, {}, {}});
tests.push_back({"Bez_07_Teardrop", TEST_BEZIER, {{50, 10}, {90, 50}, {10, 50}, {50, 10}}, {}, {}, {}});
tests.push_back({"Bez_08_ZigZag", TEST_BEZIER, {{10, 50}, {30, 90}, {70, 10}, {90, 50}}, {}, {}, {}});
tests.push_back({"Bez_09_Swirl", TEST_BEZIER, {{10, 10}, {90, 10}, {90, 90}, {50, 50}}, {}, {}, {}});
tests.push_back({"Bez_10_Asymmetric", TEST_BEZIER, {{10, 10}, {20, 90}, {90, 90}, {90, 80}}, {}, {}, {}});
*/

// 2. БАЗОВЫЕ ТЕСТЫ ОТСЕЧЕНИЯ ОТРЕЗКОВ
std::vector<Point> square = {{20, 20}, {80, 20}, {80, 80}, {20, 80}};
tests.push_back({"ClipLine_01_Inside", TEST_CLIP_LINE, {}, square, {30, 30}, {70, 70}, {}});
tests.push_back({"ClipLine_02_Outside", TEST_CLIP_LINE, {}, square, {85, 20}, {85, 80}, {}});
tests.push_back({"ClipLine_03_Through_Horiz", TEST_CLIP_LINE, {}, square, {10, 50}, {90, 50}, {}});
tests.push_back({"ClipLine_04_Through_Vert", TEST_CLIP_LINE, {}, square, {50, 10}, {50, 90}, {}});
tests.push_back({"ClipLine_05_Diagonal", TEST_CLIP_LINE, {}, square, {10, 10}, {90, 90}, {}});
tests.push_back({"ClipLine_06_Half_In", TEST_CLIP_LINE, {}, square, {50, 50}, {90, 50}, {}});
tests.push_back({"ClipLine_07_Near_Edge", TEST_CLIP_LINE, {}, square, {15, 20}, {85, 20}, {}});
std::vector<Point> tri = {{10, 10}, {40, 40}, {70, 10}, {40, 30}};
tests.push_back({"ClipLine_08_Tri_Through", TEST_CLIP_LINE, {}, tri, {0, 20}, {80, 20}, {}});
/*
// 3. РАНДОМНЫЕ ТЕСТЫ БЕЗЬЕ
for (int i = 1; i <= 10; ++i) {
    TestCase t;
    std::stringstream ss;
    ss << "Rand_Bez_" << std::setw(2) << std::setfill('0') << i;
    t.description = ss.str();
    t.type = TEST_BEZIER;
    for(int j=0; j<4; ++j) {
        t.bezierPoints.push_back({randomFloat(10, 140), randomFloat(10, 140)});
    }
    tests.push_back(t);
}

// 4. РАНДОМНЫЕ ТЕСТЫ ОТСЕЧЕНИЯ ОТРЕЗКОВ
for (int i = 1; i <= 10; ++i) {
    TestCase t;
    std::stringstream ss;
    ss << "Rand_ClipLine_" << std::setw(2) << std::setfill('0') << i;
    t.description = ss.str();
    t.type = TEST_CLIP_LINE;
    int vertices = (int)randomFloat(3, 9); 
    t.clip_polygon = generateRandomConvexPolygon(75, 75, randomFloat(30, 50), vertices);
    t.lineStart = {randomFloat(0, 150), randomFloat(0, 150)};
    t.lineEnd = {randomFloat(0, 150), randomFloat(0, 150)};
    tests.push_back(t);
}

    // 5. ТЕСТЫ ОТСЕЧЕНИЯ ПОЛИГОНОВ 
    
   //Квадрат
std::vector<Point> clipSq = {{30, 30}, {70, 30}, {70, 70}, {30, 70}};

// 1. Треугольник полностью внутри
tests.push_back({"ClipPoly_01_TriInside", TEST_CLIP_POLY, {}, clipSq, {}, {}, 
    {{40, 60}, {60, 60}, {50, 40}}});

// 2. Звезда Давида 
std::vector<Point> clipTri = {{50, 20}, {80, 80}, {20, 80}};
std::vector<Point> subjInvTri = {{20, 40}, {80, 40}, {50, 90}};
tests.push_back({"ClipPoly_02_Star", TEST_CLIP_POLY, {}, clipTri, {}, {}, subjInvTri});

// 3. Вогнутый полигон, частично внутри квадрата
std::vector<Point> vShape = {{10, 10}, {50, 50}, {90, 10}, {90, 40}, {50, 80}, {10, 40}};
tests.push_back({"ClipPoly_03_ConcaveV", TEST_CLIP_POLY, {}, clipSq, {}, {}, vShape});

// 4. Субъект полностью снаружи
std::vector<Point> outsidePoly = {{10, 10}, {20, 10}, {20, 20}, {10, 20}};
tests.push_back({"ClipPoly_04_Outside", TEST_CLIP_POLY, {}, clipSq, {}, {}, outsidePoly});

// 5. Субъект окружает клиппер (большой квадрат вокруг маленького)
std::vector<Point> bigSq = {{10, 10}, {90, 10}, {90, 90}, {10, 90}};
tests.push_back({"ClipPoly_05_Surround", TEST_CLIP_POLY, {}, clipSq, {}, {}, bigSq});


// 6. РАНДОМНЫЕ ТЕСТЫ ОТСЕЧЕНИЯ ПОЛИГОНОВ (10 штук)
for (int i = 1; i <= 10; ++i) {
    TestCase t;
    std::stringstream ss;
    ss << "Rand_ClipPoly_" << std::setw(2) << std::setfill('0') << i;
    t.description = ss.str();
    t.type = TEST_CLIP_POLY;
    
    // Окно - выпуклый полигон
    int clipVerts = (int)randomFloat(3, 7);
    t.clip_polygon = generateRandomConvexPolygon(75, 75, randomFloat(30, 50), clipVerts);
    
    // Субъект - простой случайный полигон
    int subjVerts = (int)randomFloat(3, 10);
    float rMin = randomFloat(10, 30);
    float rMax = randomFloat(40, 80);
    t.subject_polygon = generateRandomSimplePolygon(75, 75, rMin, rMax, subjVerts);
    
    tests.push_back(t);
}

    // 4. ГРАНИЧНЫЙ: Совпадающие полигоны
    tests.push_back({"ClipPoly_06_Identical", TEST_CLIP_POLY, {}, clipSq, {}, {}, clipSq});

    // 5. ГРАНИЧНЫЙ: Касание ребром (Subject внутри, касается правой грани)
    // Clip: x=30..70, Subj: Triangle touching x=70 edge
    std::vector<Point> touchEdgeTri = {{40, 40}, {70, 40}, {70, 60}}; 
    tests.push_back({"ClipPoly_07_TouchEdge", TEST_CLIP_POLY, {}, clipSq, {}, {}, touchEdgeTri});

    // 6. ГРАНИЧНЫЙ: Касание вершиной (изнутри)
    // Касается точки (70, 70)
    std::vector<Point> touchVertTri = {{40, 40}, {70, 70}, {40, 60}};
    tests.push_back({"ClipPoly_08_TouchVertex", TEST_CLIP_POLY, {}, clipSq, {}, {}, touchVertTri});

    // 7. ВЫРОЖДЕННЫЙ: Полигон-линия (площадь 0)
    // Должен корректно отсечься (остаться линией или исчезнуть)
    std::vector<Point> degenerateLine = {{20, 50}, {80, 50}, {80, 50}}; // Фактически отрезок
    tests.push_back({"ClipPoly_09_DegenerateLine", TEST_CLIP_POLY, {}, clipSq, {}, {}, degenerateLine});

    // 8. ОРИЕНТАЦИЯ: Субъект задан против часовой (обратный порядок)
    // Квадрат {{40, 60}, {60, 60}, {50, 40}} реверсированный
    std::vector<Point> reverseTri = {{50, 40}, {60, 60}, {40, 60}}; // CCW
    tests.push_back({"ClipPoly_10_ReversedSubj", TEST_CLIP_POLY, {}, clipSq, {}, {}, reverseTri});

    // 9. ОРИЕНТАЦИЯ: Отсекатель задан против часовой
    // Должен автоматически исправиться и работать как обычный квадрат
    std::vector<Point> revClipSq = clipSq;
    std::reverse(revClipSq.begin(), revClipSq.end());
    tests.push_back({"ClipPoly_11_ReversedClip", TEST_CLIP_POLY, {}, revClipSq, {}, {}, {{40, 60}, {60, 60}, {50, 40}}});
*/
    return tests;
}


//  ГРАФИКА (Без изменений) 
void setPixel(Image &img, int x, int y, unsigned char color) {
    if (x >= 0 && x < img.width && y >= 0 && y < img.height)
        img.pixels[y * img.width + x] = color;
}

void writePng(const std::string &filename, const Image &img) {
    FILE *fp = fopen(filename.c_str(), "wb");
    if (!fp) return;
    png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (setjmp(png_jmpbuf(png_ptr))) return;
    png_init_io(png_ptr, fp);
    png_set_IHDR(png_ptr, info_ptr, img.width, img.height, 8, PNG_COLOR_TYPE_GRAY,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(png_ptr, info_ptr);
    std::vector<png_bytep> rows(img.height);
    for (int y = 0; y < img.height; y++)
        rows[y] = (png_bytep)&img.pixels[y * img.width];
    png_write_image(png_ptr, rows.data());
    png_write_end(png_ptr, nullptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(fp);
}

void drawLine(Image &img, Point p1, Point p2, unsigned char color) {
    int x1 = static_cast<int>(p1.x), y1 = static_cast<int>(p1.y);
    int x2 = static_cast<int>(p2.x), y2 = static_cast<int>(p2.y);
    bool steep = abs(y2 - y1) > abs(x2 - x1);
    if (steep) { std::swap(x1, y1); std::swap(x2, y2); }
    if (x1 > x2) { std::swap(x1, x2); std::swap(y1, y2); }
    int dx = x2 - x1, dy = abs(y2 - y1), err = dx / 2, ystep = (y1 < y2 ? 1 : -1), y = y1;
    for (int x = x1; x <= x2; x++) {
        if (steep) setPixel(img, y, x, color); else setPixel(img, x, y, color);
        err -= dy; if (err < 0) { y += ystep; err += dx; }
    }
}

void drawPolygonLines(Image &img, const std::vector<Point> &points, unsigned char color, bool closed) {
    int n = static_cast<int>(points.size());
    if (n < 2) return;
    for (int i = 0; i < n - 1; ++i) drawLine(img, points[i], points[i + 1], color);
    if (closed) drawLine(img, points.back(), points.front(), color);
}

void drawRectangleLines(Image &img, float x, float y, float width, float height, unsigned char color) {
    std::vector<Point> rect = {{x, y}, {x + width, y}, {x + width, y + height}, {x, y + height}};
    drawPolygonLines(img, rect, color, true);
}

Point TransformWorldToScreen(Point worldPoint, float minX, float minY, float scaleX, float scaleY,
                             float viewportX, float viewportY) {
    return {
        viewportX + (worldPoint.x - minX) * scaleX,
        viewportY + (worldPoint.y - minY) * scaleY
    };
}



// 1. Кривые Безье
int estimateBezierSegments(const Point& p0, const Point& p1, const Point& p2, const Point& p3) {
    float approx_length = distance(p0, p1) + distance(p1, p2) + distance(p2, p3);
    return std::max(20, std::min(500, static_cast<int>(approx_length * 2.0f)));
}

std::vector<Point> calculateCubicBezierCurve(const Point& p0, const Point& p1,
                                             const Point& p2, const Point& p3) {
    int num_segments = estimateBezierSegments(p0, p1, p2, p3);
    std::vector<Point> curve_points;
    for (int i = 0; i <= num_segments; ++i) {
        float t = static_cast<float>(i) / num_segments;
        Point p01 = interpolatePoints(p0, p1, t);
        Point p12 = interpolatePoints(p1, p2, t);
        Point p23 = interpolatePoints(p2, p3, t);
        Point p012 = interpolatePoints(p01, p12, t);
        Point p123 = interpolatePoints(p12, p23, t);
        curve_points.push_back(interpolatePoints(p012, p123, t));
    }
    return curve_points;
}

// 2. Отсечение Кируса-Бека (Линии)
bool cyrusBeckClip(Point& p0, Point& p1, const std::vector<Point>& clip_polygon_input) {
    std::vector<Point> clip_polygon = clip_polygon_input;
    if (signedPolygonArea(clip_polygon) > 0) {
        std::reverse(clip_polygon.begin(), clip_polygon.end());
    }

    float t_enter = 0.0f, t_leave = 1.0f;
    Point D = calculateDisplacement(p1, p0); 

    for (size_t i = 0; i < clip_polygon.size(); ++i) {
        Point p_edge_start = clip_polygon[i];
        Point p_edge_end = clip_polygon[(i + 1) % clip_polygon.size()];
        
        // Внутренняя нормаль (зависит от направления обхода).
        // Если обход по часовой (для Y вниз это стандарт), нормаль (-dy, dx).
        Point n = {p_edge_end.y - p_edge_start.y, -(p_edge_end.x - p_edge_start.x)};
        
        Point w = calculateDisplacement(p0, p_edge_start);
        
        float n_scalarProduct_d = scalarProduct(n, D);
        float n_scalarProduct_w = scalarProduct(n, w);

        if (std::abs(n_scalarProduct_d) < 1e-6) {
            if (n_scalarProduct_w < 0) return false; 
            continue;
        }

        float t = -n_scalarProduct_w / n_scalarProduct_d;

        if (n_scalarProduct_d > 0) {
            t_enter = std::max(t_enter, t); 
        } else {
            t_leave = std::min(t_leave, t); 
        }
    }

    if (t_enter > t_leave) return false; 

    Point new_p1 = {p0.x + D.x * t_leave, p0.y + D.y * t_leave};
    Point new_p0 = {p0.x + D.x * t_enter, p0.y + D.y * t_enter};
    
    p0 = new_p0;
    p1 = new_p1;
    return true;
}

// 3. Отсечение Сазерленда-Ходжмана (Полигон по Выпуклому полигону)
bool isInside(Point p, Point a, Point b) {
    return ((b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x)) >= 0; 
}

Point intersectLines(Point a, Point b, Point edgeStart, Point edgeEnd) {
    float A1 = b.y - a.y, B1 = a.x - b.x, C1 = A1 * a.x + B1 * a.y;
    float A2 = edgeEnd.y - edgeStart.y, B2 = edgeStart.x - edgeEnd.x, C2 = A2 * edgeStart.x + B2 * edgeStart.y;
    float det = A1 * B2 - A2 * B1;
    if (std::abs(det) < 1e-6) return a; // Защита от деления на 0
    return { (B2 * C1 - B1 * C2) / det, (A1 * C2 - A2 * C1) / det };
}

std::vector<Point> sutherlandHodgmanClip(const std::vector<Point>& subjectPolyInput, const std::vector<Point>& clipperPolyInput) {
    std::vector<Point> outputList = subjectPolyInput;
    std::vector<Point> clipper = clipperPolyInput;

    // 1. Нормализация ОТСЕКАТЕЛЯ 
    if (signedPolygonArea(clipper) < 0) { 
        std::reverse(clipper.begin(), clipper.end());
    }

    // 2. Нормализация СУБЪЕКТА 
    if (signedPolygonArea(outputList) < -1e-5) {
        std::reverse(outputList.begin(), outputList.end());
    }

    Point clipEdgeStart = clipper.back();

    for (const auto& clipEdgeEnd : clipper) {
        std::vector<Point> inputList = outputList;
        outputList.clear();

        if (inputList.empty()) break;

        Point S = inputList.back();

        for (const auto& E : inputList) {
            bool E_in = isInside(E, clipEdgeStart, clipEdgeEnd);
            bool S_in = isInside(S, clipEdgeStart, clipEdgeEnd);

            if (E_in) {
                if (!S_in) {
                    outputList.push_back(intersectLines(S, E, clipEdgeStart, clipEdgeEnd));
                }
                outputList.push_back(E);
            } else if (S_in) {
                outputList.push_back(intersectLines(S, E, clipEdgeStart, clipEdgeEnd));
            }
            S = E;
        }
        clipEdgeStart = clipEdgeEnd;
    }

    return outputList;
}


int main() {
    const unsigned char ColorBackground = 255;
    const unsigned char ColorControlPolygon = 180;
    const unsigned char ColorBezierCurve = 0;
    const unsigned char ColorClipPolygon = 100; 
    const unsigned char ColorOriginalLine = 180; 
    const unsigned char ColorClippedLine = 0;    
    const unsigned char ColorSubjPolygon = 160; 
    const unsigned char ColorResultPoly = 0;    
    const unsigned char ColorViewportBorder = 200;

    const int ImageWidth = 800;
    const int ImageHeight = 650;

    std::vector<TestCase> tests = generateAllTests();
    
    if (!std::filesystem::exists("output")) {
        std::filesystem::create_directory("output");
    }

    std::cout << "Generated " << tests.size() << " test cases." << std::endl;

    for (const auto& currentTest : tests) {
        std::cout << "Processing: " << currentTest.description << std::endl;
        
        Image img(ImageWidth, ImageHeight);
        std::fill(img.pixels.begin(), img.pixels.end(), ColorBackground);

        //  Расчет границ для Viewport 
        float minX = 0, maxX = 1, minY = 0, maxY = 1;
        std::vector<Point> allPoints;
        
        if (currentTest.type == TEST_BEZIER) {
            allPoints = currentTest.bezierPoints;
        } else if (currentTest.type == TEST_CLIP_LINE) {
            allPoints = currentTest.clip_polygon;
            allPoints.push_back(currentTest.lineStart);
            allPoints.push_back(currentTest.lineEnd);
        } else if (currentTest.type == TEST_CLIP_POLY) {
            allPoints = currentTest.clip_polygon;
            allPoints.insert(allPoints.end(), currentTest.subject_polygon.begin(), currentTest.subject_polygon.end());
        }
        
        if (!allPoints.empty()) {
            minX = maxX = allPoints[0].x; minY = maxY = allPoints[0].y;
            for (const auto& p : allPoints) {
                minX = std::min(minX, p.x); maxX = std::max(maxX, p.x);
                minY = std::min(minY, p.y); maxY = std::max(maxY, p.y);
            }
        }
        float w = maxX - minX; if(w==0) w=1;
        float h = maxY - minY; if(h==0) h=1;
        float padX = std::max(w * 0.1f, 10.0f), padY = std::max(h * 0.1f, 10.0f);
        minX -= padX; maxX += padX; minY -= padY; maxY += padY;
        w = maxX - minX; h = maxY - minY;
        const float scale = std::min(ImageWidth / w, ImageHeight / h);
        const float viewportWidth = w * scale, viewportHeight = h * scale;
        const float viewportX = (ImageWidth - viewportWidth) / 2.0f, viewportY = (ImageHeight - viewportHeight) / 2.0f;

        drawRectangleLines(img, viewportX, viewportY, viewportWidth, viewportHeight, ColorViewportBorder);

        //  Отрисовка 

        if (currentTest.type == TEST_BEZIER) {
            std::vector<Point> screenPoints;
            for (const auto& p : currentTest.bezierPoints) 
                screenPoints.push_back(TransformWorldToScreen(p, minX, minY, scale, scale, viewportX, viewportY));
            drawPolygonLines(img, screenPoints, ColorControlPolygon, false);
            auto curveWorldPoints = calculateCubicBezierCurve(currentTest.bezierPoints[0], currentTest.bezierPoints[1], currentTest.bezierPoints[2], currentTest.bezierPoints[3]);
            std::vector<Point> curveScreenPoints;
            for (const auto& p : curveWorldPoints) 
                curveScreenPoints.push_back(TransformWorldToScreen(p, minX, minY, scale, scale, viewportX, viewportY));
            drawPolygonLines(img, curveScreenPoints, ColorBezierCurve, false);
        }
        else if (currentTest.type == TEST_CLIP_LINE) {
            std::vector<Point> clipPolyScreen;
            for (const auto& p : currentTest.clip_polygon) 
                clipPolyScreen.push_back(TransformWorldToScreen(p, minX, minY, scale, scale, viewportX, viewportY));
            drawPolygonLines(img, clipPolyScreen, ColorClipPolygon, true);
            Point screenLineStart = TransformWorldToScreen(currentTest.lineStart, minX, minY, scale, scale, viewportX, viewportY);
            Point screenLineEnd = TransformWorldToScreen(currentTest.lineEnd, minX, minY, scale, scale, viewportX, viewportY);
            drawLine(img, screenLineStart, screenLineEnd, ColorOriginalLine);
            Point p0Clip = currentTest.lineStart, p1Clip = currentTest.lineEnd;
            if (cyrusBeckClip(p0Clip, p1Clip, currentTest.clip_polygon)) {
                Point screenClipStart = TransformWorldToScreen(p0Clip, minX, minY, scale, scale, viewportX, viewportY);
                Point screenClipEnd = TransformWorldToScreen(p1Clip, minX, minY, scale, scale, viewportX, viewportY);
                drawLine(img, screenClipStart, screenClipEnd, ColorClippedLine);
                // Thick line
                drawLine(img, {screenClipStart.x+1, screenClipStart.y}, {screenClipEnd.x+1, screenClipEnd.y}, ColorClippedLine);
            }
        }
        else if (currentTest.type == TEST_CLIP_POLY) {
            std::vector<Point> clipPolyScreen;
            for (const auto& p : currentTest.clip_polygon) 
                clipPolyScreen.push_back(TransformWorldToScreen(p, minX, minY, scale, scale, viewportX, viewportY));
            drawPolygonLines(img, clipPolyScreen, ColorClipPolygon, true);

            std::vector<Point> subjPolyScreen;
            for (const auto& p : currentTest.subject_polygon) 
                subjPolyScreen.push_back(TransformWorldToScreen(p, minX, minY, scale, scale, viewportX, viewportY));
            drawPolygonLines(img, subjPolyScreen, ColorSubjPolygon, true);

            std::vector<Point> clippedPoly = sutherlandHodgmanClip(currentTest.subject_polygon, currentTest.clip_polygon);

            if (!clippedPoly.empty()) {
                std::vector<Point> resScreen;
                for (const auto& p : clippedPoly) 
                    resScreen.push_back(TransformWorldToScreen(p, minX, minY, scale, scale, viewportX, viewportY));
                
                drawPolygonLines(img, resScreen, ColorResultPoly, true);
                // Thick line
                std::vector<Point> shiftX, shiftY;
                for(auto p : resScreen) shiftX.push_back({p.x+1, p.y});
                for(auto p : resScreen) shiftY.push_back({p.x, p.y+1});
                drawPolygonLines(img, shiftX, ColorResultPoly, true);
                drawPolygonLines(img, shiftY, ColorResultPoly, true);
            }
        }

        std::string output_filename = "output/" + currentTest.description + ".png";
        try { writePng(output_filename, img); } catch (...) {}
    }

    return 0;
}