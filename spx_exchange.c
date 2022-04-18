/**
 * comp2017 - assignment 3
 * Sam Kelly
 * SKEL4720
 */

#include "spx_exchange.h"

// void process_command((void)(*p)(), char** arguments) {}

void buy_order() {}

void sell_order() {}

void amend_order() {}

void cancel_order() {}

// Reads the products file and returns a list of the product's names
char** read_products_file(char* fp) {
	FILE* file;
	if ((file = fopen(fp, "r")) == NULL) {
		return NULL;
	}

	char length_str[PRODUCT_LENGTH]; // +++ get actual length for this
	fgets(length_str, PRODUCT_LENGTH, file);
	int file_length = strtol(length_str, NULL, 10) + 1;
	char** products = (char**) malloc(sizeof(char**) * file_length);
	products[0] = length_str;

	int index = 1;
	while ((file_length - index) > 0) {
		char* product = (char*) malloc(sizeof(char) * PRODUCT_LENGTH);
		product = fgets(product, PRODUCT_LENGTH, file);
		products[index++] = strtok(product, "\n");;
	}
	return products;
}

int main(int argc, char **argv) {
	if (argc > 2) {
		printf("%s Starting\n", LOG_PREFIX);
		char** products = read_products_file(argv[1]);

		if (products == NULL) {
			printf("%s Error: Products file does not exist", LOG_PREFIX);
			return 1;
		}
		int file_length = strtol(products[0], NULL, 10);
		printf("%s Trading %d products: ", LOG_PREFIX, file_length);
		for (int index = 1; index <= file_length; index++) {
			printf("%s", products[index]);
			if (index != file_length) {
				printf(" ");
			} else {
				printf("\n");
			}
		}
	} else {
		printf("Not enough arguments"); //+++ check  messaging and arg lengths
		return 1;
	}
}
