
/**
 * comp2017 - assignment 3
 * Sam Kelly
 * SKEL4720
 */

#include "spx_exchange.h"


int read_trader = -1;
int init_flag = 0;
int disconnect_trader = -1;
int total_fees = 0;


void read_sig(int signo, siginfo_t *si, void *uc) {
	if (signo == SIGUSR1) {
		read_trader = si->si_pid;
	} else if (signo == SIGUSR2) {
		printf("%s Error: Given trader binary doesn't exist\n", LOG_PREFIX);
		exit(0); // +++ replace with a graceful exit function
	} else if (signo == SIGCHLD) {
		disconnect_trader = si->si_pid;
		return;
	} else if (signo == SIGINT) {
		printf("Exitting...");
		exit(0);
	}
}

int write_pipe(int fd, char* message) {
	if (strlen(message) < MAX_INPUT) {
		if (fd == -1) {
			return -1;
		}
		write(fd, message, strlen(message));
		return 1;
	}
	return -1;
}

struct order** delete_order(struct order** orders, int index) {
	int cursor = index;
	free(orders[cursor]);
	while (orders[cursor + 1] != NULL) {
		orders[cursor] = orders[cursor + 1];
		cursor++;
	}
	orders = realloc(orders, sizeof(struct order) * cursor);
	return orders;
}


struct order** create_order(int type, int trader_id, int order_id, char product[PRODUCT_LENGTH], int qty, int price, struct order** (*operation)(struct order*, struct order**), struct order** orders) {
 // Adding order to the exchange's array of orders/ memory management stuff
	struct order* new_order = malloc(sizeof(struct order));
	new_order->type = type;
	new_order->order_id = order_id;
	memcpy(new_order->product, product, PRODUCT_LENGTH);
	new_order->qty = qty;
	new_order->price = price;
	new_order->trader_id = trader_id;

	orders = operation(new_order, orders);

	return orders;
}

struct order** buy_order(struct order* new_order, struct order** orders) {
	int matching = 1;

	while (matching) {

		struct order* cheapest_sell = NULL;
		int cheapest_index = 0;
		int current_order = 0; // orders is null terminated, so 0 always exists

		while (orders[current_order] != NULL) {
			// Booleans to check if the current order is compatible with the new order
			int product_valid = (strcmp(orders[current_order]->product, new_order->product) == 0);
			int price_valid = (orders[current_order]->price <= new_order->price); // +++ check for equality in the spec
			int trader_valid = (orders[current_order]->trader_id != new_order->trader_id);

			if (trader_valid && product_valid && price_valid && orders[current_order]->type == SELL) {
				if (cheapest_sell == NULL || orders[current_order]->price < cheapest_sell->price) {
					cheapest_sell = orders[current_order];
					cheapest_index = current_order;
				}
			}
			current_order++;
		}

		if (cheapest_sell == NULL) {
			break;
		}

		int qty = 0;
		if (cheapest_sell->qty <= new_order->qty) {
			qty = cheapest_sell->qty;
			new_order->qty -= cheapest_sell->qty;
		} else {
			cheapest_sell->qty -= new_order->qty;
			qty = new_order->qty;
		}

		int cost = qty * cheapest_sell->price;
		int fee = (int)round(0.01 * cost);
		total_fees += fee;

		printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%d, fee: $%d.", LOG_PREFIX, orders[current_order]->order_id,\
		orders[current_order]->trader_id, new_order->order_id, new_order->trader_id, cost,\
		fee);

		if (cheapest_sell->qty == 0) {
			char path[PATH_LENGTH];
			snprintf(path, PATH_LENGTH, EXCHANGE_PATH, cheapest_sell->order_id);
			int fd = open(path, O_WRONLY);

			char msg[MAX_INPUT];
			snprintf(msg, MAX_INPUT, "FILL %d %d", cheapest_sell->order_id, cheapest_sell->qty);
			orders = delete_order(orders, cheapest_index);

			write_pipe(fd, msg);
			close(fd);
		}

		if (new_order->qty != 0) {
			break;
		}
	}
	if (new_order->qty != 0) {
		int cursor = 0;
		while (orders[cursor] != NULL) {
			cursor++;
		}

		orders = realloc(orders, sizeof(struct order) * (cursor + 2));
		orders[cursor] = new_order;
		orders[cursor + 1] = NULL;
	}
	return orders;
}

// Manages the matching process the sell orders when they are received
struct order** sell_order(struct order* new_order, struct order** orders) {

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
				current_order--;
				// send signal to fill order to current_order.trader
			} else {
				new_order->qty -= orders[current_order]->qty;
			}
		}
		current_order++;
	}

	orders = realloc(orders, sizeof(struct order) * (current_order + 2));
	orders[current_order] = new_order;
	orders[current_order + 1] = NULL;
	return orders;
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
	char* length_str = malloc(PRODUCT_LENGTH); // +++ get actual length for this
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
	int result = read(fd, input, MAX_INPUT);
	if (result == -1) {
		return (char**)NULL;
	}
	int args_length = 1;
	char** arg_array = (char**) malloc(sizeof(char*));
	arg_array[0] = (char*)NULL;

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
		arg_array[args_length - 1] = arg;
		// Null terminated array of args
		arg_array[args_length] = (char*)NULL;

		// Traverse to next token
		token = strtok(NULL, " ");
		args_length++;
	}
	return arg_array;
}

struct trader* initialise_trader(char* path, int index, int num_products) {

	struct trader* new_trader = malloc(sizeof(struct trader));
	new_trader->pid = fork();
	new_trader->id = index;
	new_trader->position_qty = calloc(sizeof(int), sizeof(int) * num_products);
	new_trader->position_cost = calloc(sizeof(int), sizeof(int) * num_products);
	new_trader->current_order_id = 0;
	if (new_trader->pid == -1) {
		printf("%s Fork failed\n", LOG_PREFIX);
		return NULL;
	}

	if (new_trader->pid > 0) {
		printf("%s Starting trader %d (%s)\n", LOG_PREFIX, index, path);
		return new_trader;
	}

	char trader_id[MAX_TRADERS_BYTES];
	sprintf(trader_id, "%d", index);
	if (execl(path, path, trader_id, NULL) == -1) {
		kill(getppid(), SIGUSR2);
		kill(getpid(), 9);
		return NULL;
	}
	return new_trader;
}

int create_fifo(char* path) {
	unlink(path);//+++

	if (mkfifo(path, 0666) == -1) {//+++ check perms
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
		*num_levels = *num_levels + 1;
		*num_type = *num_type + 1;
		levels = realloc(levels, sizeof(struct level) * *num_levels);
		struct level new_level = {current_order->price, 1, current_order->qty, current_order->type};
		levels[*num_levels - 1] = new_level;
	}
	return levels;
}

void generate_orderbook(int num_products, char** products, struct order** orders, struct trader** traders) {

	printf("%s	--ORDERBOOK--\n", LOG_PREFIX);
	fflush(stdout);

	for (int product = 1; product <= num_products; product++) {
		int num_levels = 0;
		int num_sell_levels = 0;
		int num_buy_levels = 0;

		struct level* levels = malloc(0);
		int cursor = 0;
		while (orders[cursor] != NULL) {
			if (strcmp(orders[cursor]->product, products[product]) == 0) {
				if (orders[cursor]->type == SELL) {
					levels = orderbook_helper(orders[cursor], &num_levels, &num_sell_levels, levels);	// +++ Check that these pointers are actually updated lol
				}
				if (orders[cursor]->type == BUY) {
					levels = orderbook_helper(orders[cursor], &num_levels, &num_buy_levels, levels);
				}
			}
			cursor++;
		}
		printf("%s\tProduct: %s; Buy levels: %d; Sell levels: %d\n", LOG_PREFIX, products[product], num_buy_levels, num_sell_levels);
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

			printf("%s\t\t%s %d @ $%d (%d %s)\n", LOG_PREFIX, type_str, levels[max_index].qty, levels[max_index].price, \
						levels[max_index].num, order_str);

			levels[sort_cursor] = levels[max_index];
			for (int level = max_index; level > sort_cursor + 1; level--) {
				levels[level] = levels[level - 1];
			}
			sort_cursor++;
		}
		free(levels);
	}

	printf("%s	--POSITIONS--\n", LOG_PREFIX);

	int cursor = 0;
	while (traders[cursor] != NULL) {
		printf("%s	Trader %d: ", LOG_PREFIX, traders[cursor]->id);

		for (int product_num = 0; product_num < num_products; product_num++) {
			printf("%s %d ($%d)", products[product_num + 1], traders[cursor]->position_qty[product_num], traders[cursor]->position_cost[product_num]);
			if (product_num != num_products - 1) {
				printf(", ");
			} else {
				printf("\n");
			}
		}
		cursor++;
	}
}

struct trader** disconnect(struct trader** traders, struct order** orders, char** products, int argc) {
	int valid = 0;
	int cursor = 0;

	while (traders[cursor] != NULL) {
		if (disconnect_trader == traders[cursor]->pid) {
			printf("%s Trader %d disconnected\n", LOG_PREFIX, traders[cursor]->id);
			free(traders[cursor]->position_qty);
			free(traders[cursor]->position_cost);
			free(traders[cursor]);
			valid = 1;
		}
		if (valid) {
			traders[cursor] = traders[cursor + 1];
		}
		cursor++;
	}

	if (valid) {
		// printf("%d", cursor);
		// fflush(stdout);
		traders = realloc(traders, cursor * sizeof(struct trader));
		traders[cursor] = NULL;
		// check the validitiy of this pls
	}

	disconnect_trader = -1;

	if (cursor == 1) {
		printf("%s Trading completed\n", LOG_PREFIX);
		printf("%s Exchange fees collected: $%d\n", LOG_PREFIX, total_fees);
		int cursor = 0;
		while (orders[cursor] != NULL) {
			free(orders[cursor++]);
		}
		free(orders);
		cursor = 0;
		while (cursor < argc - 2) {
			char path[PATH_LENGTH];
			snprintf(path, PATH_LENGTH, EXCHANGE_PATH, cursor);
			unlink(path);
			snprintf(path, PATH_LENGTH, TRADER_PATH, cursor);
			unlink(path);
			cursor++;
		}
		cursor = 0;
		while (traders[cursor] != NULL) {
			free(traders[cursor]->position_qty);
			free(traders[cursor]->position_cost);
			free(traders[cursor]);
			cursor++;
		}
		free(traders);
		int limit = strtol(products[0], NULL, 10);
		for (int index = 0; index <= limit; index++) {
			free(products[index]);
		}
		free(products);
		return NULL;
	}
	return traders;
}

int main(int argc, char **argv) {
	if (argc > 1) {
		printf("%s Starting\n", LOG_PREFIX);
		char** products = read_products_file(argv[1]);

		if (products == NULL) {
			printf("%s Error: Products file does not exist", LOG_PREFIX);
			return -1;
		}

		struct trader** traders = malloc(sizeof(struct trader) * (argc - 1));
		traders[argc - 2] = NULL;

		struct sigaction sig_act;

		sig_act.sa_handler = (void *)read_sig;
		sigemptyset(&sig_act.sa_mask);
		sig_act.sa_flags = SA_RESTART | SA_SIGINFO;

		sigaction(SIGCLD, &sig_act, NULL);
		sigaction(SIGUSR1, &sig_act, NULL);
		sigaction(SIGUSR2, &sig_act, NULL);
		sigaction(SIGINT, &sig_act, NULL);


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
			traders[trader-2] = initialise_trader(argv[trader], trader-2, strtol(products[0], NULL, 10));
			if (traders[trader-2] == NULL) {
				return -1;
			}

			// Connects to each named pipe
			traders[trader-2]->exchange_fd = open(exchange_path, O_WRONLY);
			printf("%s Connected to %s\n", LOG_PREFIX, exchange_path);

			traders[trader-2]->trader_fd = open(trader_path, O_RDONLY);
			printf("%s Connected to %s\n", LOG_PREFIX, trader_path);
		}
		// Sending MARKET OPEN message to all exchange pipes
		int cursor = 0;
		while (traders[cursor] != NULL) {
			write_pipe(traders[cursor++]->exchange_fd, "MARKET OPEN;");
		}

		cursor = 0;
		while (traders[cursor] != NULL) {
			kill(traders[cursor++]->pid, SIGUSR1);
		}


		// Creates a null terminated array of orders
		struct order** orders = malloc(sizeof(struct order));
		orders[0] = NULL;

		int running = 1;
		// int counter = 0;
		while (running) {
			// Need a list of current traders to update when traders dc+++
			if (disconnect_trader != -1) {
				traders = disconnect(traders, orders, products, argc);
				if (traders == NULL) {
					return 0;
				}
			}

			char** arg_array = NULL;
			int trader_number = -1;
			// use select here to monitor pipe +++
			if (read_trader != -1) {

				read_trader = -1;
				int cursor = 0;
				while (traders[cursor] != NULL) {
					if (traders[cursor]->pid == read_trader) {

						arg_array = take_input(traders[cursor]->trader_fd);
						break;
					}
					cursor++;
				}

				if (arg_array == NULL) {
					continue;
				}

				printf("%s [T%d] Parsing command: <", LOG_PREFIX, traders[cursor]->id);

				int arg_cursor = 0;
				while (arg_array[arg_cursor] != NULL) {

						for (int cursor = 0; cursor < strlen(arg_array[arg_cursor]); cursor++) {
							if (arg_array[arg_cursor][cursor] == ';' || arg_array[arg_cursor][cursor] == '\n') {
								arg_array[arg_cursor][cursor] = '\0';
							}
						}

					printf("%s", arg_array[arg_cursor]);


					if (arg_array[arg_cursor + 1] != NULL) {
						printf(" ");
					}
					arg_cursor++;
				}

				printf(">\n");

				if (arg_cursor != 5) {

					arg_cursor = 0;
					while (arg_array[arg_cursor] != NULL) {
						free(arg_array[arg_cursor++]);
					}

					free(arg_array);
					continue;
				}


				int amount = strtol(arg_array[3], NULL, 10);
				int price = strtol(arg_array[4], NULL, 10);
				int order_id = strtol(arg_array[1], NULL, 10);

				int id_valid = (order_id == traders[cursor]->current_order_id);
				int product_valid = 0;
				int amount_valid = (amount > 0 && amount < 1000000);
				int price_valid = (price > 0 && price < 1000000);

				for (int product = 1; product <= strtol(products[0], NULL, 10); product++) {
					if (strcmp(products[product], arg_array[2]) == 0) {
						product_valid = 1;
						break;
					}
				}

				char* msg = malloc(MAX_INPUT);
				if (id_valid && product_valid && amount_valid && price_valid) {

					if (strcmp(arg_array[0], "BUY") == 0) {
						orders = create_order(BUY, trader_number, order_id, arg_array[2], amount, price, &buy_order, orders);

					} else if (strcmp(arg_array[0], "SELL") == 0) {
						orders = create_order(SELL, 12, order_id, arg_array[2], amount, price, &sell_order, orders);

					} else if (strcmp(arg_array[0], "AMEND") == 0) {
						printf("amend");

					} else if (strcmp(arg_array[0], "DEL") == 0) {
						printf("del");
					}
					sprintf(msg, "ACCEPTED %s;", arg_array[1]);
					traders[cursor]->current_order_id++;

					// Generating and displaying the orderbook for the exchange
					generate_orderbook(strtol(products[0], NULL, 10), products, orders, traders);

					char* market_msg = malloc(MAX_INPUT);
					int index = 0;
					sprintf(market_msg, "MARKET %s %s %d %d;", arg_array[0], arg_array[2], amount, price);

					while (traders[index] != NULL) {
						if (index != cursor) {
							write_pipe(traders[index]->exchange_fd, market_msg);
							kill(traders[index]->pid, SIGUSR1);
						}
						index++;
					}
					free(market_msg);

				} else {
					sprintf(msg, "INVALID;");
				}

				// Inform the trader that their order was accepted
				write_pipe(traders[cursor]->exchange_fd, msg);
				kill(traders[cursor]->pid, SIGUSR1);
				free(msg);

				cursor = 0;
				while (arg_array[cursor] != NULL) {
					free(arg_array[cursor++]);
				}

				free(arg_array);
			}

			// if (counter++ == 1000) {
			// 	running = 0;
			// }
			// sleep(1); // Check for responsiveness, or add blocking io if necessary +++
		}

	} else {

		printf("Not enough arguments"); //+++ check  messaging and arg lengths
		return 1;
	}
}
