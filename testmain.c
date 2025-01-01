#include <stdio.h>

void parse_log_file(const char *filepath, float *sum_buyingValue, float *sum_price) {
    FILE *file = fopen(filepath, "r");
    if (!file) {
        perror("Error opening file");
        return;
    }

    char line[100];

    while (fgets(line, sizeof(line), file)) {
        float buyingValue, price;

        // Parsing each line assuming the format is:
        // Item,ThreadID,Store,Category,File,CategoryPID,buyingValue,price(float)
        if (sscanf(line, "%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%f,%f", &buyingValue, &price) == 2) {
            *sum_buyingValue += buyingValue;
            *sum_price += price;

            // Debugging to check each parsed price
            printf("price = %f\n", price);
        } else {
            printf("Failed to parse line: %s", line);
        }
    }

    fclose(file);
}

int main(int argc, char const *argv[]) {
    float sum_price = 0.0;
    float sum_buyingValue = 0.0;

    parse_log_file("./Dataset/Store1/user1_0.log", &sum_buyingValue, &sum_price);

    printf("\nTotal Price: %f\n", sum_price);
    printf("Total Buying Value: %f\n", sum_buyingValue);

    return 0;
}
