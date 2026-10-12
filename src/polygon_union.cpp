#include <iostream>
#include <vector>
#include <cmath>
#include <algorithm>
#include <png.h> 
#include <string>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include <sys/stat.h>

//  Структуры данных 

const double EPS = 1e-6;

struct Pixel {
    unsigned char r, g, b;
};

struct Point {
    double x, y;
    
    // Для сравнения координат с погрешностью
    bool operator==(const Point& other) const {
        return std::abs(x - other.x) < EPS && std::abs(y - other.y) < EPS;
    }
    
    // Для сортировки (лексикографический порядок)
    bool operator<(const Point& other) const {
        if (std::abs(x - other.x) > EPS) return x < other.x;
        return y < other.y;
    }

    Point operator+(const Point& other) const { return {x + other.x, y + other.y}; }
    Point operator-(const Point& other) const { return {x - other.x, y - other.y}; }
    Point operator*(double k) const { return {x * k, y * k}; }
    Point operator/(double k) const { return {x / k, y / k}; }
};

struct Segment {
    Point p1, p2;
    bool valid = true; // Флаг для удаления сегмента
};

class Canvas {
    int width, height;
    std::vector<Pixel> buffer;

public:
    Canvas(int w, int h) : width(w), height(h) {
        buffer.resize(w * h, {255, 255, 255});
    }

    void setPixel(int x, int y, Pixel color) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            buffer[y * width + x] = color;
        }
    }

    void drawLine(int x0, int y0, int x1, int y1, Pixel color) {
        int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
        int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
        int err = dx + dy, e2;

        while (true) {
            setPixel(x0, y0, color);
            if (x0 == x1 && y0 == y1) break;
            e2 = 2 * err;
            if (e2 >= dy) { err += dy; x0 += sx; }
            if (e2 <= dx) { err += dx; y0 += sy; }
        }
    }

    void drawPolygon(const std::vector<Point>& poly, Pixel color) {
        if (poly.size() < 2){
            int radius = 2;
            for(int dy=-radius; dy<=radius; dy++)
                for(int dx=-radius; dx<=radius; dx++)
                    setPixel((int)poly[0].x+dx, (int)poly[0].y+dy, color);
            return;
        } 
        for (size_t i = 0; i < poly.size(); i++) {
            Point p1 = poly[i];
            Point p2 = poly[(i + 1) % poly.size()];
            drawLine((int)p1.x, (int)p1.y, (int)p2.x, (int)p2.y, color);
            
            // Рисуем вершины жирнее
            for(int dy=-1; dy<=1; dy++)
                for(int dx=-1; dx<=1; dx++)
                    setPixel((int)p1.x+dx, (int)p1.y+dy, color);
        }
    }

    void savePNG(const std::string& filename) {
        FILE *fp = fopen(filename.c_str(), "wb");
        if (!fp) {
            std::cerr << "Error: Could not open " << filename << "\n";
            return;
        }

        png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
        if (!png) return;
        png_infop info = png_create_info_struct(png);
        if (!info) return;
        if (setjmp(png_jmpbuf(png))) return;

        png_init_io(png, fp);
        png_set_IHDR(png, info, width, height, 8, PNG_COLOR_TYPE_RGB,
                     PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
        png_write_info(png, info);

        std::vector<png_bytep> row_pointers(height);
        for (int y = 0; y < height; y++) {
            row_pointers[y] = (png_bytep)&buffer[y * width];
        }

        png_write_image(png, row_pointers.data());
        png_write_end(png, NULL);
        fclose(fp);
        png_destroy_write_struct(&png, &info);
        std::cout << "Saved: " << filename << std::endl;
    }
};

//  Геометрия 

double cross_product(Point a, Point b, Point c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

double distSq(Point a, Point b) {
    return (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y);
}

// Лежит ли точка P на отрезке AB 
bool onSegment(Point p, Point a, Point b) {
    return std::abs(cross_product(a, b, p)) < EPS &&
           (p.x - a.x) * (p.x - b.x) <= EPS &&
           (p.y - a.y) * (p.y - b.y) <= EPS;
}

// Строго ли внутри точка P относительно полигона (Ray casting или Winding number не нужен для выпуклых, но надежнее)
bool isStrictlyInside(const std::vector<Point>& poly, Point p) {
    if (poly.size() < 3) return false;
    for (size_t i = 0; i < poly.size(); i++) {
        // Если точка справа или на линии ребра (при обходе CCW), то она не строго внутри
        if (cross_product(poly[i], poly[(i + 1) % poly.size()], p) <= EPS) return false;
    }
    return true;
}

bool isPointCovered(const Point& p, const std::vector<Point>& shape) {
    if (shape.empty()) return false;
    if (shape.size() == 1) {
        return shape[0] == p;
    }
    if (shape.size() == 2) {
        return onSegment(p, shape[0], shape[1]);
    }
    // Для полигона: или строго внутри, или на границе
    if (isStrictlyInside(shape, p)) return true;
    for (size_t i = 0; i < shape.size(); i++) {
        if (onSegment(p, shape[i], shape[(i + 1) % shape.size()])) return true;
    }
    return false;
}

// Пересечение двух прямых 
bool getLineIntersection(Point p1, Point p2, Point p3, Point p4, Point &res) {
    double d = (p1.x - p2.x) * (p3.y - p4.y) - (p1.y - p2.y) * (p3.x - p4.x);
    if (std::abs(d) < EPS) return false; // Параллельны

    double t = ((p1.x - p3.x) * (p3.y - p4.y) - (p1.y - p3.y) * (p3.x - p4.x)) / d;
    double u = -((p1.x - p2.x) * (p1.y - p3.y) - (p1.y - p2.y) * (p1.x - p3.x)) / d;

    if (t >= -EPS && t <= 1.0 + EPS && u >= -EPS && u <= 1.0 + EPS) {
        res.x = p1.x + t * (p2.x - p1.x);
        res.y = p1.y + t * (p2.y - p1.y);
        return true;
    }
    return false;
}

// Обеспечить обход против часовой стрелки
void ensureCCW(std::vector<Point>& poly) {
    double area = 0;
    for (size_t i = 0; i < poly.size(); i++) {
        area += (poly[(i+1)%poly.size()].x - poly[i].x) * (poly[(i+1)%poly.size()].y + poly[i].y);
    }
    if (area > 0) { 
        std::reverse(poly.begin(), poly.end());
    }
}

// Выпуклая оболочка 
std::vector<Point> getConvexHull(std::vector<Point>& pts) {
    size_t n = pts.size(), k = 0;
    if (n <= 2) return pts;
    std::vector<Point> h(2 * n);
    std::sort(pts.begin(), pts.end());
    for (size_t i = 0; i < n; ++i) {
        while (k >= 2 && cross_product(h[k - 2], h[k - 1], pts[i]) <= 0) k--;
        h[k++] = pts[i];
    }
    for (size_t i = n - 1, t = k + 1; i > 0; --i) {
        while (k >= t && cross_product(h[k - 2], h[k - 1], pts[i - 1]) <= 0) k--;
        h[k++] = pts[i - 1];
    }
    h.resize(k - 1);
    return h;
}

std::vector<Point> generateRandomPolygon(int cx, int cy, int r, int n) {
    std::vector<Point> cloud;
    for(int i=0; i<n; i++) {
        double angle = (rand()%360) * 3.14159 / 180.0;
        double rad = rand() % r + 10;
        cloud.push_back({cx + rad*cos(angle), cy + rad*sin(angle)});
    }
    return getConvexHull(cloud);
}

//  Основная логика 

// Разбивает ребра полигона p1 точками пересечения с ребрами p2
std::vector<Segment> shatterPolygon(const std::vector<Point>& subject, const std::vector<Point>& cutter) {
    std::vector<Segment> result;
    if (subject.size() < 2) return result; // Точки не бьются на сегменты

    // Если это отрезок (2 точки), мы не должны идти от 2-й точки обратно к 1-й.
    // Если это полигон (>2 точек), мы должны замкнуть контур.
    size_t edgesCount = (subject.size() == 2) ? 1 : subject.size();

    for (size_t i = 0; i < edgesCount; ++i) {
        Point a = subject[i];
        Point b = subject[(i + 1) % subject.size()];
        
        std::vector<Point> cuts;
        cuts.push_back(a);
        cuts.push_back(b);

        // Ищем пересечения с ребрами cutter
        size_t cutterEdges = (cutter.size() == 2) ? 1 : cutter.size();
        for (size_t j = 0; j < cutterEdges; ++j) {
            Point c = cutter[j];
            Point d = cutter[(j + 1) % cutter.size()];
            Point inter;
            
            if (getLineIntersection(a, b, c, d, inter)) {
                cuts.push_back(inter);
            }
            if (onSegment(c, a, b)) cuts.push_back(c);
            if (onSegment(d, a, b)) cuts.push_back(d);
        }

        std::sort(cuts.begin(), cuts.end(), [&](Point p1, Point p2){
            return distSq(a, p1) < distSq(a, p2);
        });

        // Удаляем дубликаты точек, которые слишком близко (важно для корректности)
        auto last = std::unique(cuts.begin(), cuts.end(), [](const Point& l, const Point& r){
            return l == r;
        });
        cuts.erase(last, cuts.end());

        for (size_t k = 0; k < cuts.size() - 1; ++k) {
            result.push_back({cuts[k], cuts[k+1], true});
        }
    }
    return result;
}


std::vector<std::vector<Point>> getUnion(std::vector<Point> p1, std::vector<Point> p2) {
    std::vector<std::vector<Point>> resultPolys;

    // 0. Обработка вырожденных случаев (пустые входные данные)
    if (p1.empty()) return {p2};
    if (p2.empty()) return {p1};

    // 1. Сохраняем изолированные точки (размер 1)
    // Если точка из p1 не покрыта p2, она идет в результат. И наоборот.
    if (p1.size() == 1) {
        if (!isPointCovered(p1[0], p2)) resultPolys.push_back(p1);
    }
    if (p2.size() == 1) {
        if (!isPointCovered(p2[0], p1)) resultPolys.push_back(p2);
    }

    // Если оба входа были точками, мы закончили (shatterPolygon вернет пустоту)
    if (p1.size() < 2 && p2.size() < 2) return resultPolys;

    // Подготовка для полигонов
    if (p1.size() > 2) ensureCCW(p1);
    if (p2.size() > 2) ensureCCW(p2);

    // 2. Разбиваем ребра
    auto segs1 = shatterPolygon(p1, p2);
    auto segs2 = shatterPolygon(p2, p1);

    std::vector<Segment> allSegs = segs1;
    allSegs.insert(allSegs.end(), segs2.begin(), segs2.end());

    // 3. Фильтрация
    for (auto& s : allSegs) {
        Point mid = (s.p1 + s.p2) / 2.0;

        // Важно: для Union мы удаляем сегмент только если он СТРОГО ВНУТРИ другой фигуры.
        // Если фигура - полигон, isStrictlyInside работает.
        // Если фигура - отрезок, isStrictlyInside вернет false (площади нет), 
        // значит сегмент не удалится. Это верно, так как объединение линий - это сумма линий.
        
        bool inP1 = (p1.size() > 2) && isStrictlyInside(p1, mid);
        bool inP2 = (p2.size() > 2) && isStrictlyInside(p2, mid);

        if (inP1 || inP2) {
            s.valid = false;
        }
    }

    // 4. Удаление дубликатов
    for (size_t i = 0; i < allSegs.size(); ++i) {
        if (!allSegs[i].valid) continue;
        for (size_t j = i + 1; j < allSegs.size(); ++j) {
            if (!allSegs[j].valid) continue;

            bool sameDir = (allSegs[i].p1 == allSegs[j].p1 && allSegs[i].p2 == allSegs[j].p2);
            bool oppDir = (allSegs[i].p1 == allSegs[j].p2 && allSegs[i].p2 == allSegs[j].p1);

            if (sameDir) {
                allSegs[j].valid = false; 
            } else if (oppDir) {
                // Если векторы направлены навстречу друг другу и лежат точно друг на друге,
                // для Union это один и тот же геометрический отрезок. Оставляем один.
                // Но лучше нормализовать направление при сборке.
                allSegs[j].valid = false;
            }
        }
    }

    // 5. Сборка (Исправленная логика)
    std::vector<Segment> validSegs;
    for(const auto& s : allSegs) {
        if (s.valid) validSegs.push_back(s);
    }

    if (validSegs.empty()) return resultPolys; // Возвращаем то, что уже накопили (точки)

    std::vector<bool> used(validSegs.size(), false);
    
    // Жадный алгоритм сборки
    for (size_t i = 0; i < validSegs.size(); ++i) {
        if (used[i]) continue;

        std::vector<Point> currentPoly;
        currentPoly.push_back(validSegs[i].p1);
        Point currPt = validSegs[i].p2;
        used[i] = true;

        bool closed = false;
        bool stuck = false;

        while (!closed && !stuck) {
            bool foundNext = false;
            
            // Сначала ищем идеальное продолжение (начало следующего == конец текущего)
            for (size_t j = 0; j < validSegs.size(); ++j) {
                if (!used[j]) {
                    if (validSegs[j].p1 == currPt) {
                        currentPoly.push_back(currPt);
                        currPt = validSegs[j].p2;
                        used[j] = true;
                        foundNext = true;
                        break;
                    } 
                    // Можно добавить проверку на обратное направление, если сегменты не ориентированы
                    else if (validSegs[j].p2 == currPt) {
                         currentPoly.push_back(currPt);
                         currPt = validSegs[j].p1;
                         used[j] = true;
                         foundNext = true;
                         break;
                    }
                }
            }

            if (!foundNext) {
                // Если вернулись в начало -> замкнуто
                if (currPt == currentPoly[0]) {
                    closed = true;
                } else {
                    // Цепочка оборвалась (это нормально для отрезков)
                    currentPoly.push_back(currPt);
                    stuck = true; 
                }
            } else {
                if (currPt == currentPoly[0]) closed = true;
            }
        }
        /*
        if (currentPoly.size() >= 2) {
            resultPolys.push_back(currentPoly);
        }*/
        resultPolys.push_back(currentPoly);
    }

    return resultPolys;
}


//тестирование

void runTest(int testId, std::string name, std::vector<Point> p1, std::vector<Point> p2) {
    std::cout << "Test " << testId << ": " << name << " ... ";
    
    Canvas cvIn(800, 800);
    // Рисуем исходные чуть прозрачными линиями (здесь просто цвета)
    cvIn.drawPolygon(p1, {0,0,255}); // Синий
    cvIn.drawPolygon(p2, {0,255,0}); // Зеленый
    cvIn.savePNG("output/test_" + std::to_string(testId) + "_input.png");

    auto res = getUnion(p1, p2);

    Canvas cvOut(800, 800);
    cvOut.drawPolygon(p1, {220,220,255});
    cvOut.drawPolygon(p2, {220,255,220});

    // Результат красным
    for(auto& poly : res) {
        cvOut.drawPolygon(poly, {255, 0, 0});
    }
    cvOut.savePNG("output/test_" + std::to_string(testId) + "_output.png");
}

int main() {
    srand(time(0));

    
    // 1. Обычное пересечение
    runTest(1, "Cross", 
        {{200,200}, {500,200}, {500,500}, {200,500}},
        {{350,350}, {650,350}, {650,650}, {350,650}}
    );

    // 2. Совпадение ребра 
    runTest(2, "Coinciding Edge", 
        {{200,200}, {500,200}, {500,500}, {200,500}},
        {{500,200}, {700,200}, {700,500}, {500,500}} 
    );

    // 3. Маленький треугольник касается основания
    runTest(3, "Vertex on Edge (Inner)", 
        {{100,600}, {500,600}, {300,300}}, 
        {{300,600}, {300,450}, {400,450}} 
    );

    // 4. Дизъюнктные
    runTest(4, "Disjoint",
        {{100,100}, {200,100}, {200,200}, {100,200}},
        {{600,600}, {700,600}, {700,700}, {600,700}});

    // 5. Полная вложенность 
    runTest(5, "Strictly Nested",
        {{100,100}, {700,100}, {700,700}, {100,700}},
        {{300,300}, {500,300}, {500,500}, {300,500}});


    // 6. Вершина одного лежит на ребре другого 
    runTest(6, "Vertex Touching Edge (Outer)", 
        {{100, 100}, {300, 100}, {300, 300}, {100, 300}},
        {{300, 200}, {500, 200}, {400, 100}} // Треугольник носом касается правой стороны квадрата
    );

    // 7. Касание только одной вершиной 
    runTest(7, "Corner Touch",
        {{100, 100}, {200, 100}, {200, 200}, {100, 200}},
        {{200, 200}, {300, 200}, {300, 300}, {200, 300}}
    );

    // 8. "Звезда Давида" 
    runTest(8, "Star Shape",
        {{400, 100}, {600, 500}, {200, 500}}, // Треугольник вверх
        {{400, 600}, {600, 200}, {200, 200}}  // Треугольник вниз
    );

    // 9. Совпадение ребра с частичным перекрытием 
    runTest(9, "Partial Edge Overlap",
        {{200, 200}, {400, 200}, {400, 400}, {200, 400}}, // Квадрат 1
        {{400, 300}, {600, 300}, {600, 500}, {400, 500}}  // Квадрат 2, сдвинут вправо и вверх
    );

    // 10. Почти совпадающие
    runTest(10, "Epsilon Shift",
        {{200, 200}, {400, 200}, {400, 400}, {200, 400}},
        {{202, 202}, {402, 202}, {402, 402}, {202, 402}}
    );
    /*
    // Случайные тесты
    for(int i=0; i<10; i++) {
        auto p1 = generateRandomPolygon(300 + rand()%100, 300 + rand()%100, 100 + rand()%100, 10);
        auto p2 = generateRandomPolygon(400 + rand()%100, 400 + rand()%100, 100 + rand()%100, 10);
        runTest(11+i, "Random_" + std::to_string(i), p1, p2);
    }*/

    ///Вырожденные случаи////

    runTest(11, "Polygon + Point Outside",
        {{200,200}, {400,200}, {400,400}, {200,400}}, // Квадрат
        {{500, 300}} // Точка справа
    );

    // 12. Полигон + Точка внутри (Точка должна исчезнуть)
    runTest(12, "Polygon + Point Inside",
        {{200,200}, {400,200}, {400,400}, {200,400}}, // Квадрат
        {{300, 300}} // Точка в центре
    );

    // 13. Полигон + Отрезок, торчащий наружу ("Антенна")
    // Ожидается: Квадрат + кусок отрезка снаружи
    runTest(13, "Polygon + Protruding Segment",
        {{200,200}, {400,200}, {400,400}, {200,400}}, // Квадрат
        {{300,300}, {600,300}} // Отрезок из центра наружу вправо
    );

    // 14. Два пересекающихся отрезка (Крест)
    // Ожидается: Крестообразная фигура (в векторе может быть как два отрезка или 4 сегмента)
    runTest(14, "Cross Segments",
        {{100,300}, {700,300}}, // Горизонтальный
        {{400,100}, {400,500}}  // Вертикальный
    );

    // 15. Отрезок внутри полигона (Полное поглощение)
    runTest(15, "Segment Inside Polygon",
        {{200,200}, {400,200}, {400,400}, {200,400}}, 
        {{250,250}, {350,350}}
    );
    
    // 16. Отрезок частично касается края
    runTest(16, "Segment Touching Edge",
        {{200,200}, {400,200}, {400,400}, {200,400}}, 
        {{200,200}, {100,100}} // Из угла наружу
    );

    // 17. Точка + Точка (Разные)
    runTest(17, "Point + Point",
        {{100,100}}, 
        {{200,200}}
    );


    return 0;
}