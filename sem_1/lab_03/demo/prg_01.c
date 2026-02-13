#include <stdio.h>
#include <math.h>

#define WRONG_INPUT 1

// Подсчет значения
float calc(float a, int n)
{
    float res = 1;
    for (int i = 1; i <= n; i++)
    {
        res *= a;
    }

    return res;
}

// a ^ n
int main(void)
{
    //Ввод и объявление переменных
    float a;
    int n;
    float res;
    printf("Введите целое а, и натуральное n: ");
    if (scanf("%f%d", &a, &n) != 2)
    {
        printf("Wrong input\n");
        return WRONG_INPUT;
    }

    if (n <= 0)
    {
        printf("n должен быть положительным\n");
        return 2;
    }

    // Calculate Result
    res = calc(a, n);
    printf("Результат a^n = %f\n", res);

    return 0;
}
