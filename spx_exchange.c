/**
 * comp2017 - assignment 3
 * Sam Kelly
 * SKEL4720
 */

#include "spx_exchange.h"

int read_flag = 0;

void handle_invalid_bin(int errno) {
		printf("%s Error: Given trader binary doesn't exist\n", LOG_PREFIX);
		exit(0); // +++ replace with a graceful exit function
}

void read_sig(int errno) {
		printf("%s Reading in exchange\n", LOG_PREFIX);
		read_flag = 1;
}


struct order* create_order(int type, int trader_id, int order_id, char product[PRODUCT_LENGTH], int qty, int price, struct order* (*operation)(struct order*, struct order**), struct order** orders) {
 // Adding order to the exchange's array of orders/ memory management stuff
	struct order* new_order = malloc(sizeof(struct order));
	new_order->type = type;
	new_order->order_id = order_id;
	memcpy(new_order->product, product, PRODUCT_LENGTH);
	new_order->qty = qty;
	new_order->price = price;
	new_order->trader_id = trader_id;

	new_order = operation(new_order, orders);
	return new_order;
}
//
// struct order* buy_order(int order_id, char product[PRODUCT_LENGTH], int qty, int price) {
// 	// struct order checking for sells??
// 	struct order* new_order = create_order(BUY, order_id, product, qty, price, &buy_order);
// 	return new_order;
// }
//
struct order* sell_order(struct order* new_order, struct order** orders) {

	int current_order = 0; // orders is null terminated, so 0 always exists
	while (orders[current_order] != NULL) {
		// Booleans to check if the current order is compatible with the new order
		int product_valid = (strcmp(orders[current_order]->product, new_order->product) == 0);
		int price_valid = (orders[current_order]->price >= new_order->price);
		if (product_valid && price_valid) {
			if (orders[current_order]->qty <= new_order->qty) {
				// send signal to fill order to current_order.trader
			}
		}
		current_order++;
	}
	return new_order;
}
//
// struct order* amend_order(int order_id, int qty, int price) {
// 	struct order* new_order = create_order(AMEND, order_id, NULL, qty, price, &amend_order);
//
// 	// del old struct order and replace
// 	return new_order;
// }
//
// struct order cancel_order(int struct order_id) {
// 	// del struct order
// 	// struct order checking for sells??
// 	return NULL; // ?
// }

// Reads the products file and returns a list of the product's names
char** read_products_file(char* fp) {
	FILE* file;
	if ((file = fopen(fp, "r")) == NULL) {
		return NULL;
	}
	// Number of products in product file
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
	fclose(file);

	printf("%s Trading %d products: ", LOG_PREFIX, file_length - 1);

	for (int index = 1; index < file_length; index++) {
		printf("%s", products[index]);
		if (index != file_length - 1) {
			printf(" ");
		} else {
			printf("\n");
		}
	}
	return products;
}

char** take_input() {
	char input[MAX_INPUT], *token;
	read(3, input, MAX_INPUT); // will be a pipe +++
	char** arg_array = (char**) malloc(0);
	int args_length = 0;

	token = strtok(input, " ");
	while (token != NULL) {
		// Removing newline characters from the token
		int char_index = 0;
		while (token[char_index] != '\0') {
			if (token[char_index] == '\n') {
				token[char_index] = '\0';
			}
			char_index++;
		}

		// Allocating memory to store the token
		arg_array = realloc(arg_array, (args_length + 1) * sizeof(char**));
		char* arg = malloc(PRODUCT_LENGTH);
		memcpy(arg, token, PRODUCT_LENGTH);
		arg_array[args_length] = arg;

		// Traverse to next token
		token = strtok(NULL, " ");
		args_length++;
	}
	return arg_array;
}

int write_pipe(int fd, char* message, pid_t pid) {
	if (strlen(message) + 1 < MAX_INPUT) {
		if (fd == -1) {
			return -1;
		}
		write(fd, message, MAX_INPUT);
		kill(pid, SIGUSR1);
		return 1;
	}
	return -1;
}

int initialise_trader(char* path, int* pid_array, int index) {
	pid_array[index] = fork();
	if (pid_array[index] == -1) {
		printf("%s Fork failed\n", LOG_PREFIX);
		return -1;
	}
	if (pid_array[index] == 0) {
		char ppid[MAX_PID];
		char trader_id[MAX_TRADERS_BYTES];
		sprintf(ppid, "%d", getppid());
		sprintf(trader_id, "%d", index);
		printf("%s Starting trader %s (%s)\n", LOG_PREFIX, trader_id, path);
		if (execl(path, trader_id, ppid, (char*)NULL) == -1) {
			kill(getppid(), SIGUSR2);
			kill(getpid(), 9);
		}
	}
	sleep(0.2);
	return 1;
}

int create_fifo(int* fds, char* path, int index) {
	// +++ CHECK if there can be multiple exchanges???
	unlink(path);//+++

	if (mkfifo(path, 0777) == -1) {//+++ check perms
		printf("%s Error: Could not create FIFO\n", LOG_PREFIX);
		return -1;
	}
	fds[index] = open(path, O_RDWR | O_NONBLOCK);
	printf("%s Created FIFO %s\n", LOG_PREFIX, path);
	return 1;
}

int main(int argc, char **argv) {
	if (argc > 1) {
		printf("%s Starting\n", LOG_PREFIX);
		char** products = read_products_file(argv[1]);

		if (products == NULL) {
			printf("%s Error: Products file does not exist", LOG_PREFIX);
			return -1;
		}

		int* fds = malloc(sizeof(int) * 2 * (argc - 2));
		int fd_cursor = 0;
		int* pid_array = malloc((argc - 1) * sizeof(int));

		signal(SIGUSR2, handle_invalid_bin);

		for (int trader = 2; trader < argc; trader++) {
			char path[PATH_LENGTH];
			snprintf(path, PATH_LENGTH, TRADER_PATH, trader-2);
			if (create_fifo(fds, path, fd_cursor++) == -1) {
				return -1;
			}
			snprintf(path, PATH_LENGTH, EXCHANGE_PATH, trader-2);
			if (create_fifo(fds, path, fd_cursor++) == -1) {
				return -1;
			}
			// Starts trader processes specified by command line arguments
			if (initialise_trader(argv[trader], pid_array, trader-2) == -1) {
				return -1;
			}
			// +++ check connectivity if required
			printf("%s Connected to %s\n", LOG_PREFIX, "/tmp/spx_exchange_0");
			printf("%s Connected to %s\n", LOG_PREFIX, path);
		}

		int index = 1;
		while (fds[index] >= 0) {
			write_pipe(fds[index], "MARKET OPEN;", pid_array[index-1]);
			index++;
		}

		signal(SIGUSR1, read_sig);

		// Creates a null terminated array of orders
		struct order** orders = malloc(sizeof(struct order));
		orders[0] = (struct order*) NULL;

		int running = 1;
		while (running) {
			// use select here to monitor pipe +++
			if (read_flag) {
				printf("%s ", LOG_PREFIX);
				char** arg_array = take_input();

				if (strcmp(arg_array[0], "BUY") == 0) {
					printf("buy");
					// create_order(BUY, int order_id, char product[PRODUCT_LENGTH], int qty, int price, &buy_order)

				} else if (strcmp(arg_array[0], "SELL") == 0) {
					printf("%s", arg_array[0]);
					//Arg 2, consider how to get the trader id????? +++
					struct order* new_order = create_order(SELL, 12, strtol(arg_array[1], NULL, 10), arg_array[2], strtol(arg_array[3], NULL, 10), strtol(arg_array[4], NULL, 10), &sell_order, orders);
					printf("%s", new_order->product);
					fflush(stdout);

				} else if (strcmp(arg_array[0], "AMEND") == 0) {
					printf("amend");

				} else if (strcmp(arg_array[0], "DEL") == 0) {
					printf("del");
				}
				read_flag = 0;
			}

			sleep(1); // Check for responsiveness, or add blocking io if necessary +++
		}
		// Free all mem
	} else {

		printf("Not enough arguments"); //+++ check  messaging and arg lengths
		return 1;
	}
}
