#include <stdio.h>
#include <math.h>

#define ERROR 1
#define EPS 0.00001

// 24 variant
// Поиск периметра
int main(void)
{
    float a, b, h;
    double x, bok, p;
    printf(">>Input integer lenghts of a, b and height: ");
    scanf("%f%f%f", &a, &b, &h);

    x = (a - b) / 2.0;
    bok = x;
    bok = sqrt(pow(x, 2) + pow(h, 2)); // Считаем боковую сторону
    p = a + b + 2 * bok;               // считаем периметр
    printf("%f\n", p);

    return 0;
}
