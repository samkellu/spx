/**
 * comp2017 - assignment 3
 * Sam Kelly
 * SKEL4720
 */

#include "spx_exchange.h"


int read_flag = 0;
int init_flag = 0;

void handle_invalid_bin(int errno) {
		printf("%s Error: Given trader binary doesn't exist\n", LOG_PREFIX);
		exit(0); // +++ replace with a graceful exit function
}

void read_sig(int errno) {
		printf("%s Reading in exchange\n", LOG_PREFIX);
		read_flag = 1;
}

struct order** delete_order(struct order** orders, int index) {
	int cursor = index;
	while (orders[cursor + 1] != NULL) {
		orders[cursor] = orders[cursor + 1];
		cursor++;
	}
	orders = realloc(orders, sizeof(struct order) * cursor);
	return orders;
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
		if (product_valid && price_valid && orders[current_order]->type == BUY) {
			if (orders[current_order]->qty <= new_order->qty) {
				int cost = orders[current_order]->qty * new_order->price;
				printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%d, fee: $%d.", LOG_PREFIX, orders[current_order]->order_id,\
								orders[current_order]->trader_id, new_order->order_id, new_order->trader_id, cost,\
								(int)round(0.01 * cost));
				orders = delete_order(orders, current_order);
				// send signal to fill order to current_order.trader
			} else {
				new_order->qty -= orders[current_order]->qty;
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

char** take_input(int fd) {
	char input[MAX_INPUT], *token;
	int result = read(fd, input, MAX_INPUT); // will be a pipe +++
	if (result == -1) {
		return (char**)NULL;
	}
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

int write_pipe(int fd, char* message) {
	if (strlen(message) < MAX_INPUT) {
		if (fd == -1) {
			return -1;
		}
		write(fd, message, strlen(message) + 1);
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

	if (pid_array[index] > 0) {
		printf("%s Starting trader %d (%s)\n", LOG_PREFIX, index, path);
		return 1;
	}

	char trader_id[MAX_TRADERS_BYTES];
	sprintf(trader_id, "%d", index);
	if (execl(path, trader_id, (char*)NULL) == -1) {
		kill(getppid(), SIGUSR2);
		kill(getpid(), 9);
		return -1;
	}
	return 1;
}

int create_fifo(char* path) {
	// +++ CHECK if there can be multiple exchanges???
	unlink(path);//+++

	if (mkfifo(path, 0777) == -1) {//+++ check perms
		printf("%s Error: Could not create FIFO\n", LOG_PREFIX);
		return -1;
	}
	printf("%s Created FIFO %s\n", LOG_PREFIX, path);
	return 1;
}

struct level* orderbook_helper(struct order* current_order, int* num_levels, int* num_type, struct level* levels) {

	int valid = 1;

	for (int level_cursor = 0; level_cursor < *num_levels; level_cursor++) {
		if (current_order->price == levels[level_cursor].price && current_order->type == levels[level_cursor].type) {
			valid = 0;
			levels[level_cursor].num++;
			levels[level_cursor].qty += current_order->qty;
			break;
		}
	}

	if (valid) {
		num_levels++;
		num_type++;
		levels = realloc(levels, sizeof(struct level) * *num_levels);
		struct level new_level = {current_order->price, 1, current_order->qty, current_order->type};
		levels[*num_levels - 1] = new_level;
	}
	return levels;
}

void generate_orderbook(int num_products, char** products, struct order** orders) {

	printf("%s	--ORDERBOOK--\n", LOG_PREFIX);

	for (int product = 1; product <= num_products; product++) {

		struct order** current_orders = malloc(0);
		int num_levels = 0;
		int num_sell_levels = 0;
		int num_buy_levels = 0;
		struct level* levels = malloc(0);
		int num_orders = 0;
		int cursor = 0;

		while (orders[cursor] != NULL) {
			if (strcmp(orders[cursor]->product, products[product]) == 0) {
				if (orders[cursor]->type == SELL) {
					levels = orderbook_helper(orders[cursor], &num_levels, &num_sell_levels, levels);	// +++ Check that these pointers are actually updated lol
				}
				if (orders[cursor]->type == BUY) {
					levels = orderbook_helper(orders[cursor], &num_levels, &num_buy_levels, levels);
				}
				current_orders = realloc(current_orders, sizeof(struct order) * ++num_orders);
				current_orders[num_orders-1] = orders[cursor];
			}
		}
		printf("%s	Product: %s; Buy levels: %d; Sell levels: %d\n", LOG_PREFIX, products[product], num_buy_levels, num_sell_levels);

		int sort_cursor = 0;
		while (sort_cursor < num_levels) {

			int max = 0;
			int max_index;
			for (int level = 0; level < num_levels; level++) {
				if (levels[level].price > max) {
					max_index = level;
					max = levels[level].price;
				}
			}

			char* type_str;
			if (levels[max_index].type) {
				type_str = "SELL";
			} else {
				type_str = "BUY";
			}

			char* order_str;
			if (levels[max_index].num > 1) {
				order_str = "orders";
			} else {
				order_str = "order";
			}

			printf("%s			%s %d @ $%d (%d %s)\n", LOG_PREFIX, type_str, levels[max_index].qty, levels[max_index].price, \
						levels[max_index].num, order_str);


			levels[sort_cursor] = levels[max_index];
			for (int level = max_index; level > sort_cursor + 1; level--) {
				levels[level] = levels[level - 1];
			}
			sort_cursor++;
		}
		free(levels);
	}
}

int main(int argc, char **argv) {
	if (argc > 1) {
		printf("%s Starting\n", LOG_PREFIX);
		char** products = read_products_file(argv[1]);

		if (products == NULL) {
			printf("%s Error: Products file does not exist", LOG_PREFIX);
			return -1;
		}

		int* trader_fds = malloc(sizeof(int) * (argc - 2));
		int* exchange_fds = malloc(sizeof(int) * (argc - 2));
		int* pid_array = malloc((argc - 1) * sizeof(int));

		signal(SIGUSR2, handle_invalid_bin);

		for (int trader = 2; trader < argc; trader++) {
			// Creates named pipes for the exchange and traders
			char exchange_path[PATH_LENGTH];
			char trader_path[PATH_LENGTH];
			snprintf(exchange_path, PATH_LENGTH, EXCHANGE_PATH, trader-2);
			if (create_fifo(exchange_path) == -1) {
				return -1;
			}

			snprintf(trader_path, PATH_LENGTH, TRADER_PATH, trader-2);
			if (create_fifo(trader_path) == -1) {
				return -1;
			}

			// Starts trader processes specified by command line arguments
			if (initialise_trader(argv[trader], pid_array, trader-2) == -1) {
				return -1;
			}
			// Connects to each named pipe

			exchange_fds[trader - 2] = open(exchange_path, O_WRONLY);
			printf("%s Connected to %s\n", LOG_PREFIX, exchange_path);

			trader_fds[trader - 2] = open(trader_path, O_RDONLY);
			printf("%s Connected to %s\n", LOG_PREFIX, trader_path);
		}
		// Sending MARKET OPEN message to all exchange pipes
		for (int index = 0; index < argc - 2; index++) {
			write_pipe(exchange_fds[index], "MARKET OPEN;");
		}

		signal(SIGUSR1, read_sig);
		sleep(1);
		for (int index = 0; index < argc - 2; index++) {
			kill(pid_array[index], SIGUSR1);
		}


		// Creates a null terminated array of orders
		struct order** orders = malloc(sizeof(struct order));
		orders[0] = (struct order*) NULL;

		int running = 1;
		while (running) {
			char** arg_array;
			int trader_number;
			// use select here to monitor pipe +++
			if (read_flag) {
				printf("%s ", LOG_PREFIX);
				for (int pipe = 0; pipe < argc - 2; pipe++) {
						arg_array = take_input(trader_fds[pipe]);
						if (arg_array != NULL) {
							trader_number = pipe;
							break;
						}
					}

				printf("trader no: %d\n", trader_number);
				printf("%s [T%d] Parsing command: <%s>\n", LOG_PREFIX, trader_number, arg_array[0]);
				generate_orderbook(strtol(products[0], NULL, 10), products, orders);



				if (strcmp(arg_array[0], "BUY") == 0) {
					printf("buy");
					// create_order(BUY, int order_id, char product[PRODUCT_LENGTH], int qty, int price, &buy_order)

				} else if (strcmp(arg_array[0], "SELL") == 0) {
					printf("%s", arg_array[0]);

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
