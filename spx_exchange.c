
/**
 * comp2017 - assignment 3
 * Sam Kelly
 * SKEL4720
 */

#include "spx_exchange.h"


int read_trader = -1;
int init_flag = 0;
int disconnect_trader = -1;
long total_fees = 0;


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

struct order** delete_order(struct order* del_order, struct order** orders) {
	int index = 0;
	while (orders[index] != del_order) {
		index++;
	}
	while (orders[index] != NULL) {
		orders[index] = orders[index + 1];
		index++;
	}
	orders = realloc(orders, sizeof(struct order) * index);
	free(del_order);
	return orders;
}

struct order** cancel_order(struct order* new_order, struct order** orders, int pos_index) {

	int index = 0;
	while (orders[index]->trader != new_order->trader && orders[index]->order_id != new_order->order_id) {
		index++;
	}
	orders = delete_order(orders[index], orders);
	free(new_order);
	return orders;
}

struct order** create_order(int type, int pos_index, struct trader* trader, int order_id, char product[PRODUCT_LENGTH], int qty, int price, struct order** (*operation)(struct order*, struct order**, int), struct order** orders, struct trader** traders) {
 // Adding order to the exchange's array of orders/ memory management stuff
	struct order* new_order = malloc(sizeof(struct order));
	new_order->type = type;
	new_order->order_id = order_id;
	if (product != NULL) {
		memcpy(new_order->product, product, PRODUCT_LENGTH);
	}
	new_order->qty = qty;
	new_order->price = price;
	new_order->trader = trader;

	if (type == AMEND || type == CANCEL) {

		int cursor = 0;
		while (orders[cursor] != NULL) {
			printf("%d:%d  %d:%d\n", orders[cursor]->trader->id, new_order->trader->id, orders[cursor]->order_id, new_order->order_id);
			if (orders[cursor]->trader->id == new_order->trader->id && orders[cursor]->order_id == new_order->order_id) {
				new_order->type = orders[cursor]->type;
				printf("%s", orders[cursor]->product);
				printf("wow");
				memcpy(new_order->product, orders[cursor]->product, PRODUCT_LENGTH);
				break;
			}
			cursor++;
		}
	}
	char* type_str;
	switch (type) {
		case 0:
			type_str = "BUY";
			break;
		case 1:
			type_str = "SELL";
			break;
	}

	char* market_msg = malloc(MAX_INPUT);
	sprintf(market_msg, "MARKET %s %s %d %d;", type_str, new_order->product, new_order->qty, new_order->price);

	int index = 0;
	while (traders[index] != NULL) {
		if (traders[index] != new_order->trader && traders[index]->active) {
			write_pipe(traders[index]->exchange_fd, market_msg);
			kill(traders[index]->pid, SIGUSR1);
		}
		index++;
	}
	free(market_msg);

	orders = operation(new_order, orders, pos_index);

	return orders;
}

struct order** buy_order(struct order* new_order, struct order** orders, int pos_index) {
	int matching = 1;

	while (matching) {

		struct order* cheapest_sell = NULL;
		int current_order = 0; // orders is null terminated, so 0 always exists

		while (orders[current_order] != NULL) {
			// Booleans to check if the current order is compatible with the new order
			int product_valid = (strcmp(orders[current_order]->product, new_order->product) == 0);
			int price_valid = (orders[current_order]->price <= new_order->price); // +++ check for equality in the spec
			int trader_valid = (orders[current_order]->trader != new_order->trader);

			if (trader_valid && product_valid && price_valid && orders[current_order]->type == SELL) {
				if (cheapest_sell == NULL || orders[current_order]->price < cheapest_sell->price) {
					cheapest_sell = orders[current_order];
				}
			}
			current_order++;
		}
		current_order--;

		if (cheapest_sell == NULL) {
			break;
		}

		int qty = 0;
		if (cheapest_sell->qty <= new_order->qty) {
			qty = cheapest_sell->qty;
			new_order->qty -= cheapest_sell->qty;
			cheapest_sell->qty = 0;

		} else {

			cheapest_sell->qty -= new_order->qty;
			qty = new_order->qty;
			new_order->qty = 0;
		}

		long cost = (long)qty * cheapest_sell->price;
		long fee = (long)roundl((long)cost * FEE_AMOUNT);
		total_fees += fee;

		printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%ld, fee: $%ld.\n", LOG_PREFIX, cheapest_sell->order_id,\
		cheapest_sell->trader->id, new_order->order_id, new_order->trader->id, cost,\
		fee);

		char msg[MAX_INPUT];
		char path[PATH_LENGTH];
		int fd;

		if (cheapest_sell->trader->active) {
			// Inform trader that their order has been filled
			snprintf(path, PATH_LENGTH, EXCHANGE_PATH, cheapest_sell->trader->id);
			fd = open(path, O_WRONLY);
			snprintf(msg, MAX_INPUT, "FILL %d %d;", cheapest_sell->order_id, qty);
			write_pipe(fd, msg);
			kill(cheapest_sell->trader->pid, SIGUSR1);
			close(fd);
		}
		// Update position values
		cheapest_sell->trader->position_qty[pos_index] -= qty;
		cheapest_sell->trader->position_cost[pos_index] += cost;

		if (new_order->trader->active) {
			// Inform initiating trader that their order has been filled
			snprintf(path, PATH_LENGTH, EXCHANGE_PATH, new_order->trader->id);
			fd = open(path, O_WRONLY);
			snprintf(msg, MAX_INPUT, "FILL %d %d;", new_order->order_id, qty);
			write_pipe(fd, msg);
			kill(new_order->trader->pid, SIGUSR1);
			close(fd);
		}
		// Update position values
		new_order->trader->position_qty[pos_index] += qty;
		new_order->trader->position_cost[pos_index] -= (cost + fee);

		if (cheapest_sell->qty == 0) {
			orders = delete_order(cheapest_sell, orders);
		} else {
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
		return orders;
	}
	free(new_order);
	return orders;
}

// Manages the matching process the sell orders when they are received
struct order** sell_order(struct order* new_order, struct order** orders, int pos_index) {
	int matching = 1;

	while (matching) {

		struct order* highest_buy = NULL;
		int current_order = 0; // orders is null terminated, so 0 always exists

		while (orders[current_order] != NULL) {
			// Booleans to check if the current order is compatible with the new order
			int product_valid = (strcmp(orders[current_order]->product, new_order->product) == 0);
			int price_valid = (orders[current_order]->price >= new_order->price); // +++ check for equality in the spec
			int trader_valid = (orders[current_order]->trader != new_order->trader);

			if (trader_valid && product_valid && price_valid && orders[current_order]->type == BUY) {
				if (highest_buy == NULL || orders[current_order]->price > highest_buy->price) {
					highest_buy = orders[current_order];
				}
			}
			current_order++;
		}
		current_order--;

		if (highest_buy == NULL) {
			break;
		}

		int qty = 0;
		if (highest_buy->qty <= new_order->qty) {
			qty = highest_buy->qty;
			new_order->qty -= highest_buy->qty;
			highest_buy->qty = 0;

		} else {
			highest_buy->qty -= new_order->qty;
			qty = new_order->qty;
			new_order->qty = 0;

		}

		long cost = (long)qty * highest_buy->price;
		long fee = (long)roundl((long)cost * FEE_AMOUNT);
		total_fees += fee;

		printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%ld, fee: $%ld.\n", LOG_PREFIX, highest_buy->order_id,\
		highest_buy->trader->id, new_order->order_id, new_order->trader->id, cost,\
		fee);

		char msg[MAX_INPUT];
		char path[PATH_LENGTH];
		int fd;

		if (highest_buy->trader->active) {
			// Inform trader that their order has been filled
			snprintf(path, PATH_LENGTH, EXCHANGE_PATH, highest_buy->trader->id);
			fd = open(path, O_WRONLY);
			snprintf(msg, MAX_INPUT, "FILL %d %d;", highest_buy->order_id, qty);
			write_pipe(fd, msg);
			kill(highest_buy->trader->pid, SIGUSR1);
			close(fd);
		}
		// Update position values
		highest_buy->trader->position_qty[pos_index] += qty;
		highest_buy->trader->position_cost[pos_index] -= cost;

		if (new_order->trader->active) {
			// inform initiating trader that their order has been fulfilled
			snprintf(path, PATH_LENGTH, EXCHANGE_PATH, new_order->trader->id);
			fd = open(path, O_WRONLY);
			snprintf(msg, MAX_INPUT, "FILL %d %d;", new_order->order_id, qty);
			write_pipe(fd, msg);
			kill(new_order->trader->pid, SIGUSR1);
			close(fd);
		}
		// Update position values
		new_order->trader->position_qty[pos_index] -= qty;
		new_order->trader->position_cost[pos_index] += cost - fee;

		if (highest_buy->qty == 0) {
			orders = delete_order(highest_buy, orders);
		} else {
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
		return orders;
	}
	free(new_order);
	return orders;
}

struct order** amend_order(struct order* new_order, struct order** orders, int pos_index) {

	int cursor = 0;
	while (orders[cursor] != NULL) {

		if (orders[cursor]->order_id == new_order->order_id && orders[cursor]->trader == new_order->trader) {
			orders[cursor]->qty = new_order->qty;
			orders[cursor]->price = new_order->price;
			break;
		}
		cursor++;
	}
	free(new_order);
	return orders;
}

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
	new_trader->active = 1;
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
		while (num_levels > 0) {

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

			num_levels--;
			for (int level = max_index; level < num_levels; level++) {
				levels[level] = levels[level + 1];
			}

			levels = realloc(levels, sizeof(struct level) * num_levels);
		}
		free(levels);
	}

	printf("%s	--POSITIONS--\n", LOG_PREFIX);

	int cursor = 0;
	while (traders[cursor] != NULL) {
		printf("%s	Trader %d: ", LOG_PREFIX, traders[cursor]->id);

		for (int product_num = 0; product_num < num_products; product_num++) {
			printf("%s %ld ($%ld)", products[product_num + 1], traders[cursor]->position_qty[product_num], traders[cursor]->position_cost[product_num]);
			if (product_num != num_products - 1) {
				printf(", ");
			} else {
				printf("\n");
			}
		}
		cursor++;
	}
}

int disconnect(struct trader** traders, struct order** orders, char** products, int argc) {
	int cursor = 0;
	int count_active = 0;

	while (traders[cursor] != NULL) {
		if (disconnect_trader == traders[cursor]->pid) {
			printf("%s Trader %d disconnected\n", LOG_PREFIX, traders[cursor]->id);
			traders[cursor]->active = 0;
		}
		if (traders[cursor]->active) {
			count_active++;
		}
		cursor++;
	}
	disconnect_trader = -1;

	if (count_active == 0) {
		printf("%s Trading completed\n", LOG_PREFIX);
		printf("%s Exchange fees collected: $%ld\n", LOG_PREFIX, total_fees);

		cursor = 0;
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
			free(traders[cursor++]);
		}
		free(traders);

		int limit = strtol(products[0], NULL, 10);
		for (int index = 0; index <= limit; index++) {
			free(products[index]);
		}
		free(products);

		return 1;
	}
	return 0;
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
		while (running) {

			if (disconnect_trader != -1) {
				if (disconnect(traders, orders, products, argc)) {
					return 0;
				}
			}

			char** arg_array;
			// use select here to monitor pipe +++
			if (read_trader != -1) {

				int cursor = 0;
				while (traders[cursor] != NULL) {
					if (traders[cursor]->pid == read_trader && traders[cursor]->active) {
						arg_array = take_input(traders[cursor]->trader_fd);
						break;
					}
					cursor++;
				}
				read_trader = -1;

				if (traders[cursor] == NULL) {
					continue;
				}

				printf("%s [T%d] Parsing command: <", LOG_PREFIX, traders[cursor]->id);

				int arg_cursor = 0;
				while (arg_array[arg_cursor] != NULL) {

					if (arg_array[arg_cursor + 1] == NULL) {
						for (int cursor = 0; cursor < strlen(arg_array[arg_cursor]); cursor++) {
							if (arg_array[arg_cursor][cursor] == ';' || arg_array[arg_cursor][cursor] == '\n') {
								arg_array[arg_cursor][cursor] = '\0';
								break;
							}
						}
					}

					printf("%s", arg_array[arg_cursor]);

					if (arg_array[arg_cursor + 1] != NULL) {
						printf(" ");
					}
					arg_cursor++;
				}

				printf(">\n");

				int valid_num_args = 0;
				if (strcmp(arg_array[0], "SELL") == 0 || strcmp(arg_array[0], "BUY") == 0) {
					valid_num_args = 5;
				} else if (strcmp(arg_array[0], "AMEND") == 0) {
					valid_num_args = 4;
				} else if (strcmp(arg_array[0], "CANCEL") == 0) {
					valid_num_args = 2;
				}

				if (arg_cursor != valid_num_args) {
					arg_cursor = 0;

					while (arg_array[arg_cursor] != NULL) {
						free(arg_array[arg_cursor++]);
					}

					free(arg_array);
					if (traders[cursor]->active) {
						// Inform the trader that their order was invalid
						char* msg = malloc(MAX_INPUT);
						sprintf(msg, "INVALID;");
						write_pipe(traders[cursor]->exchange_fd, msg);
						kill(traders[cursor]->pid, SIGUSR1);
						free(msg);
					}
					continue;
				}

				int qty;
				int price;
				int order_id;

				int id_valid = 0;
				int product_valid = 0;
				int qty_valid = 0;
				int price_valid = 0;
				int product_index = 0;

				char* msg = malloc(MAX_INPUT);
				order_id = strtol(arg_array[1], NULL, 10);
				switch (valid_num_args) {
					case 5: // case for SELL and BUY orders
						qty = strtol(arg_array[3], NULL, 10);
						price = strtol(arg_array[4], NULL, 10);

						sprintf(msg, "ACCEPTED %s;", arg_array[1]);
						for (int product = 1; product <= strtol(products[0], NULL, 10); product++) {
							if (strcmp(products[product], arg_array[2]) == 0) {
								product_valid = 1;
								product_index = product - 1;
								id_valid = (order_id == traders[cursor]->current_order_id);
								break;
							}
						}
						break;

					case 4: // case for AMEND orders
						qty = strtol(arg_array[2], NULL, 10);
						price = strtol(arg_array[3], NULL, 10);
						sprintf(msg, "AMENDED %s;", arg_array[1]);

					case 2:
						qty_valid = 1;
						price_valid = 1;
						sprintf(msg, "CANCELLED %s;", arg_array[1]);

					case 4||2:
						product_valid = 1;
						int index = 0;
						while (orders[index] != NULL) {
							if (orders[index]->trader == traders[cursor] && orders[index]->order_id == order_id) {
								id_valid = 1;
								break;
							}
							index++;
						}
				}


				if (valid_num_args == 5 || valid_num_args == 4) {
					qty_valid = (qty > 0 && qty < 1000000);
					price_valid = (price > 0 && price < 1000000);
				}

				if (id_valid && product_valid && qty_valid && price_valid) {
					// Inform the trader that their order was accept
					if (traders[cursor]->active) {
						write_pipe(traders[cursor]->exchange_fd, msg);
						kill(traders[cursor]->pid, SIGUSR1);
						free(msg);
					}

					if (strcmp(arg_array[0], "SELL") == 0 || strcmp(arg_array[0], "BUY") == 0) {
						traders[cursor]->current_order_id++;
					}
					if (strcmp(arg_array[0], "BUY") == 0) {
						orders = create_order(BUY, product_index, traders[cursor], order_id, arg_array[2], qty, price, &buy_order, orders, traders);

					} else if (strcmp(arg_array[0], "SELL") == 0) {
						orders = create_order(SELL, product_index, traders[cursor], order_id, arg_array[2], qty, price, &sell_order, orders, traders);

					} else if (strcmp(arg_array[0], "AMEND") == 0) {
						orders = create_order(SELL, product_index, traders[cursor], order_id, NULL, qty, price, &amend_order, orders, traders);

					} else if (strcmp(arg_array[0], "CANCEL") == 0) {
						orders = create_order(CANCEL, product_index, traders[cursor], order_id, NULL, 0, 0, &cancel_order, orders, traders);
					}
					// Generating and displaying the orderbook for the exchange
					generate_orderbook(strtol(products[0], NULL, 10), products, orders, traders);

				} else {
					if (traders[cursor]->active) {
						// Inform the trader that their order was invalid
						sprintf(msg, "INVALID;");
						write_pipe(traders[cursor]->exchange_fd, msg);
						kill(traders[cursor]->pid, SIGUSR1);
						free(msg);
					}
				}

				cursor = 0;
				while (arg_array[cursor] != NULL) {
					free(arg_array[cursor++]);
				}
				free(arg_array);
			}
		}
	} else {
		printf("Not enough arguments"); //+++ check  messaging and arg lengths
		return 1;
	}
}
