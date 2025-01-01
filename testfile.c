#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_SIZE 256
#define MAX_DIRS 3
#define USRENAME_SIZE 64

typedef struct OrdersThread {
    char username[USRENAME_SIZE];
    int order_id;
    float total_price;
} OrdersThread;

// Function to parse a single log file and return the sum of buyingValue and price
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

// Function to find the file with the highest buyingValue and price sum
void ordersThreadTask(const OrdersThread *ot, const char *directories[MAX_DIRS]) {
    char filename[128];
    snprintf(filename, sizeof(filename), "%s_%d.log", ot->username, ot->order_id);

    float maxSumBuyingValue = 0.0;
    float maxSumPrice = 0.0;
    int maxIndex = -1;
    char maxFilePath[256] = {0};

    for (int i = 0; i < MAX_DIRS; i++) {
        char filepath[256];
        snprintf(filepath, sizeof(filepath), "%s/%s", directories[i], filename);

        float sum_buyingValue = 0.0;
        float sum_price = 0.0;

        parse_log_file(filepath, &sum_buyingValue, &sum_price);
        printf("Sum of buyingValue for file %s: %.2f, Sum of price: %.2f\n", filepath, sum_buyingValue, sum_price);

        if (sum_buyingValue > maxSumBuyingValue && sum_price > maxSumPrice) {
            maxSumBuyingValue = sum_buyingValue;
            maxSumPrice = sum_price;
            maxIndex = i;
            strncpy(maxFilePath, filepath, sizeof(maxFilePath) - 1);
        }
    }

    if (maxIndex != -1) {
        printf("File with the highest sum buyingValue and price: %s\nSum buyingValue: %.2f\nSum price: %.2f\n", maxFilePath, maxSumBuyingValue, maxSumPrice);
    } else {
        printf("No valid log files found.\n");
    }
}

// Main
int main() {
    OrdersThread ot = {"alireza", 20, 0.0};
    const char *directories[MAX_DIRS] = {
        "dir1",
        "dir2",
        "dir3"
    };

    ordersThreadTask(&ot, directories);

    return 0;
}

// Sum of buyingValue for file dir1/alireza_20.log: 150.00, Sum of price: 300.00
// Sum of buyingValue for file dir2/alireza_20.log: 250.00, Sum of price: 500.00
// Sum of buyingValue for file dir3/alireza_20.log: 180.00, Sum of price: 360.00

// File with the highest sum buyingValue and price: dir2/alireza_20.log
// Sum buyingValue: 250.00
// Sum price: 500.00
