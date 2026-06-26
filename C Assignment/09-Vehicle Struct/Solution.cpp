#include <stdio.h>

typedef struct {
    char model[20];
    int year;
    float mileage;
    int is_electric;
} Vehicle;

int main() {
    int n;
    scanf("%d", &n);

    Vehicle v[10];  // array of vehicles
    for (int i = 0; i < n; i++) {
        scanf("%s %d %f %d", v[i].model, &v[i].year, &v[i].mileage, &v[i].is_electric);
    }

    // print each vehicle
    for (int i = 0; i < n; i++) {
        printf("Model: %s, Year: %d, Mileage: %.2f km [%s]\n",
               v[i].model,
               v[i].year,
               v[i].mileage,
               v[i].is_electric ? "ELECTRIC" : "FUEL");
    }

    return 0;
}
