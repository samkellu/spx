
/**
 * comp2017 - assignment 3
 * Sam Kelly
 * SKEL4720
 */

#include "spx_exchange.h"

int read_trader = -1;
int init_flag = 0;
int disconnect_trader = -1;
int exit_flag = 0;
long total_fees = 0;

// Signal handler for SIGUSR1 (read pipe), SIGUSR2 (invalid binary) and SIGCHLD (trader disconnected)
void read_sig(int signo, siginfo_t *si, void *uc) {
	if (signo == SIGUSR1) {
		read_trader = si->si_pid;
	} else if (signo == SIGUSR2) {
		exit_flag = 1;
	} else if (signo == SIGCHLD) {
		disconnect_trader = si->si_pid;
	}
}

// Writes a message to a given pipe file descriptor
int write_pipe(int fd, char* message) {
	// Writes message only if it is of valid size, and the file descriptor is valid
	if (strlen(message) < MAX_INPUT) {
		if (fd == -1) {
			return -1;
		}
		write(fd, message, strlen(message));
		return 1;
	}
	return -1;
}

// Creates an order as specified by a trader connected to the exchange
struct order** create_order(int type, char** products, struct trader* trader, int order_id, char product[PRODUCT_LENGTH], int qty, int price, struct order** (*operation)(struct order*, \
															struct order**, int), struct order** orders, struct trader** traders, int time) {
 // Initialises a new order
	struct order* new_order = malloc(sizeof(struct order));
	new_order->type = type;
	new_order->order_id = order_id;
	new_order->qty = qty;
	new_order->price = price;
	new_order->trader = trader;
	new_order->product = malloc(PRODUCT_LENGTH);
	new_order->time = time;

	if (product != NULL) {
		memcpy(new_order->product, product, PRODUCT_LENGTH);
	}

	// Alters contents of AMEND and CANCEL orders, as originally they require data from another order to function correctly
	if (type == AMEND || type == CANCEL) {
		// Locates the order referenced by the order and copies its information into the new order
		int cursor = 0;
		while (orders[cursor] != NULL) {

			if (orders[cursor]->trader == new_order->trader && orders[cursor]->order_id == new_order->order_id) {
				new_order->type = orders[cursor]->type;
				memcpy(new_order->product, orders[cursor]->product, PRODUCT_LENGTH);
				fflush(stdout);
				break;
			}
			cursor++;
		}
	}
	// Retrieves the index of the referenced product in the trade position array
	int pos_index = 0;
	for (int index = 1; index <= strtol(products[0], NULL, 10); index++) {
		if (strcmp(products[index], new_order->product) == 0) {
			pos_index = index - 1;
			break;
		}
	}

	// Converts string type values into integers for processing
	char* type_str;
	switch (new_order->type) {
		case 0:
			type_str = "BUY";
			break;
		case 1:
			type_str = "SELL";
			break;
	}

	// Preparing messages to send to traders regarding the new order
	char* market_msg = malloc(MAX_INPUT);
	sprintf(market_msg, "MARKET %s %s %d %d;", type_str, new_order->product, new_order->qty, new_order->price);
	// Writes messages to all traders other than the initiating trader
	int index = 0;
	while (traders[index] != NULL) {
		if (traders[index] != new_order->trader && traders[index]->active) {
			write_pipe(traders[index]->exchange_fd, market_msg);
			kill(traders[index]->pid, SIGUSR1);
		}
		index++;
	}
	free(market_msg);

	// Completes the order with its provided function
	orders = operation(new_order, orders, pos_index);

	return orders;
}

// Deletes an order from the exchange
struct order** delete_order(struct order* del_order, struct order** orders) {

	// Gets the index of the order in the array
	int index = 0;
	while (orders[index] != del_order) {
		index++;
	}
	// Reshuffles the array to delete the desired order
	while (orders[index] != NULL) {
		orders[index] = orders[index + 1];
		index++;
	}
	// Reallocates memory for the order array
	orders = realloc(orders, sizeof(struct order*) * index);
	free(del_order->product);
	free(del_order);
	return orders;
}

// Cancels an order and removes it from the exchange
struct order** cancel_order(struct order* new_order, struct order** orders, int pos_index) {

	// Locates the order to be deleted
	int index = 0;
	while (orders[index] != NULL) {
		if (orders[index]->trader == new_order->trader && orders[index]->order_id == new_order->order_id) {
			break;
		}
		index++;
	}

	// Deletes the order from the exchange
	orders = delete_order(orders[index], orders);
	free(new_order->product);
	free(new_order);
	return orders;
}

// Processes a buy order on the exchange, and attempts to match the order with others using price-time priority
struct order** buy_order(struct order* new_order, struct order** orders, int pos_index) {

	while (1) {
		// Initialises a trader struct to store the current cheapest valid sell order
		struct order* cheapest_sell = NULL;
		int current_order = 0;

		// Searches for the cheapest valid SELL order for the desired product
		while (orders[current_order] != NULL) {
			// Booleans to check if the current order is compatible with the new order
			int product_valid = (strcmp(orders[current_order]->product, new_order->product) == 0);
			int price_valid = (orders[current_order]->price <= new_order->price);

			if (product_valid && price_valid && orders[current_order]->type == SELL) {
				// Price time priority sorting
				if (cheapest_sell == NULL || (orders[current_order]->price == cheapest_sell->price && orders[current_order]->time < cheapest_sell->time) || orders[current_order]->price < cheapest_sell->price) {
					cheapest_sell = orders[current_order];
				}
			}
			current_order++;
		}
		current_order--;

		// Breaks in the case where there is no valid SELL order available
		if (cheapest_sell == NULL) {
			break;
		}

		// Calculates the qty associated with the trade
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

		// Calculates the cost and fee associated with the trade
		long cost = (long)qty * cheapest_sell->price;
		long fee = (long)roundl((long)cost * FEE_AMOUNT);
		total_fees += fee;

		// Prints the order matched message
		printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%ld, fee: $%ld.\n", LOG_PREFIX, cheapest_sell->order_id,\
		cheapest_sell->trader->id, new_order->order_id, new_order->trader->id, cost, fee);

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

		// Deletes the matched order if it is now empty
		if (cheapest_sell->qty == 0) {
			orders = delete_order(cheapest_sell, orders);
		} else {
			break;
		}
	}

	// Adds the new order to the order array if it is not empty
	if (new_order->qty != 0) {
		int cursor = 0;
		while (orders[cursor] != NULL) {
			cursor++;
		}

		// Reallocates memory for the order array
		orders = realloc(orders, sizeof(struct order) * (cursor + 2));
		orders[cursor] = new_order;
		orders[cursor + 1] = NULL;
		return orders;
	}
	free(new_order->product);
	free(new_order);
	return orders;
}

// Processes a sell order on the exchange, and attempts to match the order with others using price-time priority
struct order** sell_order(struct order* new_order, struct order** orders, int pos_index) {

	while (1) {
		// Initialises a trader struct to store the most expensive valid buy order
		struct order* highest_buy = NULL;
		int current_order = 0;

		// Locates the most expensive valid buy order on the exchange
		while (orders[current_order] != NULL) {
			// Booleans to check if the current order is compatible with the new order
			int product_valid = (strcmp(orders[current_order]->product, new_order->product) == 0);
			int price_valid = (orders[current_order]->price >= new_order->price);

			if (product_valid && price_valid && orders[current_order]->type == BUY) {
				// Price time priority sorting
				if (highest_buy == NULL || (orders[current_order]->price == highest_buy->price && orders[current_order]->time < highest_buy->time) || orders[current_order]->price > highest_buy->price) {
					highest_buy = orders[current_order];
				}
			}
			current_order++;
		}

		// Breaks if there are no valid buy orders available
		if (highest_buy == NULL) {
			break;
		}
		// Calculates the quantity associated with the trade
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

		// Calculates the cost and fee associated with the trade
		long cost = (long)qty * highest_buy->price;
		long fee = (long)roundl((long)cost * FEE_AMOUNT);
		total_fees += fee;

		// Prints the matched order message
		printf("%s Match: Order %d [T%d], New Order %d [T%d], value: $%ld, fee: $%ld.\n", LOG_PREFIX, highest_buy->order_id,\
		highest_buy->trader->id, new_order->order_id, new_order->trader->id, cost, fee);

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
		// Deletes the order if it is now empty
		if (highest_buy->qty == 0) {
			orders = delete_order(highest_buy, orders);
		} else {
			break;
		}
	}

	// Adds the new order to the order array if it is not empty
	if (new_order->qty != 0) {
		int cursor = 0;
		while (orders[cursor] != NULL) {
			cursor++;
		}
		// Reallocating memory for the orders array
		orders = realloc(orders, sizeof(struct order) * (cursor + 2));
		orders[cursor] = new_order;
		orders[cursor + 1] = NULL;
		return orders;
	}
	free(new_order->product);
	free(new_order);
	return orders;
}

// Changes the quantity and price values of an order already in the exchange
struct order** amend_order(struct order* new_order, struct order** orders, int pos_index) {

	// Locates the required order
	int cursor = 0;
	while (orders[cursor] != NULL) {
		if (orders[cursor]->order_id == new_order->order_id && orders[cursor]->trader == new_order->trader) {
			break;
		}
		cursor++;
	}

	// Creates a new order with the desired specifications
	struct order* amended_order = malloc(sizeof(struct order));
	amended_order->type = orders[cursor]->type;
	amended_order->order_id = orders[cursor]->order_id;
	amended_order->qty = new_order->qty;
	amended_order->price = new_order->price;
	amended_order->trader = orders[cursor]->trader;
	amended_order->product = malloc(PRODUCT_LENGTH);
	amended_order->time = new_order->time;
	memcpy(amended_order->product, orders[cursor]->product, PRODUCT_LENGTH);

	// Deletes the old order
	orders = delete_order(orders[cursor], orders);

	free(new_order->product);
	free(new_order);
	// Reprocesses the new amended order, and attempts to match with other orders
	if (amended_order->type == SELL) {
		return sell_order(amended_order, orders, pos_index);
	}
	return buy_order(amended_order, orders, pos_index);
}

// Reads the products file and returns an array of the product's names
char** read_products_file(char* fp) {

	FILE* file;
	if ((file = fopen(fp, "r")) == NULL) {
		return NULL;
	}
	// Sets the first element of the array to be the number of products being traded
	char* length_str = malloc(PRODUCT_LENGTH);
	fgets(length_str, PRODUCT_LENGTH, file);
	int file_length = strtol(length_str, NULL, 10) + 1;
	char** products = (char**) malloc(sizeof(char**) * file_length);
	products[0] = length_str;

	// Reads in as many lines as specified by the file's 1st line
	int index = 1;
	while ((file_length - index) > 0) {

		char* product = (char*) malloc(sizeof(char) * PRODUCT_LENGTH);
		product = fgets(product, PRODUCT_LENGTH, file);
		// Checks for null or empty lines
		if (product[0] == '\n' || product[0] == '\0') {
			free(product);
		} else {
			products[index++] = strtok(product, "\n");
		}
	}
	fclose(file);

	// Prints all products being traded
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

// Reads data from the desired fifo, returns an array of input arguments
char** take_input(int fd) {

	char** args = malloc(sizeof(char**));
	args[0] = malloc(PRODUCT_LENGTH);

	int char_counter = 0;
	int arg_counter = 0;
	int total_counter = 0;

	// Reads data one character at a time until the pipe is empty, or the input is of a sufficient size
	while (total_counter < MAX_INPUT && char_counter < PRODUCT_LENGTH - 1) {

		int result = read(fd, &args[arg_counter][char_counter], 1);

		// Returns if read() returns an error
		if (result == -1) {
			for (int cursor = 0; cursor <= arg_counter; cursor++) {
				free(args[cursor]);
			}
			free(args);
			return (char**)NULL;
		}
		total_counter++;

		// Checks validity of arguments, then adds them to an array of strings
		if (args[arg_counter][char_counter] == ' '  || args[arg_counter][char_counter] == ';') {

			args = realloc(args, sizeof(char**) * (arg_counter + 2));
			if (args[arg_counter][char_counter] == ';') {
				args[arg_counter][char_counter] = '\0';
				args[arg_counter + 1] = NULL;
				return args;
			}
			args[arg_counter][char_counter] = '\0';
			arg_counter++;

			args[arg_counter] = malloc(PRODUCT_LENGTH);
			char_counter = 0;
			continue;
		}
		char_counter++;
	}
	// Cleans up memory in the event that the input was too large
	for (int cursor = 0; cursor <= arg_counter; cursor++) {
		free(args[cursor]);
	}
	free(args);
	return (char**)NULL;
}

// Starts a trader on the exchange
struct trader* initialise_trader(char* path, int index, int num_products) {

	// Initialises a trader struct for the new trader
	struct trader* new_trader = malloc(sizeof(struct trader));
	new_trader->id = index;
	new_trader->position_qty = calloc(sizeof(int), sizeof(int) * num_products);
	new_trader->position_cost = calloc(sizeof(int), sizeof(int) * num_products);
	new_trader->active = 1;
	new_trader->current_order_id = 0;

	// Forks to create a new child process
	new_trader->pid = fork();
	if (new_trader->pid == -1) {
		printf("%s Fork failed\n", LOG_PREFIX);
		return NULL;
	}

	// If the process is the parent, prints the start message, waits for a signal in case the binary couldnt
	// start correctly, then returns the new trader struct
	if (new_trader->pid > 0) {

		printf("%s Starting trader %d (%s)\n", LOG_PREFIX, index, path);
		struct timespec tim, tim2;
		tim.tv_sec = 0;
		tim.tv_nsec = 300000;
		nanosleep(&tim , &tim2);

		return new_trader;
	}

	// If the process is the child, formats command line arguments and execs to start the new process
	char trader_id[MAX_TRADERS_BYTES];
	sprintf(trader_id, "%d", index);
	if (execl(path, path, trader_id, NULL) == -1) {
		// Sends a fail signal to the parent and exits if the binary failed to start
		kill(getppid(), SIGUSR2);
		kill(getpid(), 9);
		return NULL;
	}
	return new_trader;
}

// Creates a named pipe at the desired location
int create_fifo(char* path) {

	// Removes named pipe if it already exists
	unlink(path);
	// Creates the new fifo
	if (mkfifo(path, FIFO_PERMS) == -1) {
		printf("%s Error: Could not create FIFO\n", LOG_PREFIX);
		return -1;
	}
	printf("%s Created FIFO %s\n", LOG_PREFIX, path);
	return 1;
}

// Helper function to generate level data for the orderbook
struct level* orderbook_helper(struct order* current_order, int* num_levels, int* num_type, struct level* levels) {

	int valid = 1;
	// Checks if the given order fits in a given level and adds its data to that level if it does
	for (int level_cursor = 0; level_cursor < *num_levels; level_cursor++) {
		if (current_order->price == levels[level_cursor].price && current_order->type == levels[level_cursor].type) {
			valid = 0;
			levels[level_cursor].num++;
			levels[level_cursor].qty += current_order->qty;
			break;
		}
	}

	// Creates a new level for the order
	if (valid) {
		*num_levels = *num_levels + 1;
		*num_type = *num_type + 1;
		levels = realloc(levels, sizeof(struct level) * *num_levels);
		struct level new_level = {current_order->price, 1, current_order->qty, current_order->type};
		levels[*num_levels - 1] = new_level;
	}
	return levels;
}

// Creates the orderbook to display the current state of the exchange
void generate_orderbook(int num_products, char** products, struct order** orders, struct trader** traders) {

	printf("%s	--ORDERBOOK--\n", LOG_PREFIX);

	// Generates the product message for each product
	for (int product = 1; product <= num_products; product++) {

		int num_levels = 0;
		int num_sell_levels = 0;
		int num_buy_levels = 0;
		int cursor = 0;
		struct level* levels = malloc(0);

		// Retrieves current level information for each product
		while (orders[cursor] != NULL) {
			if (strcmp(orders[cursor]->product, products[product]) == 0) {
				if (orders[cursor]->type == SELL) {
					levels = orderbook_helper(orders[cursor], &num_levels, &num_sell_levels, levels);
				}
				if (orders[cursor]->type == BUY) {
					levels = orderbook_helper(orders[cursor], &num_levels, &num_buy_levels, levels);
				}
			}
			cursor++;
		}

		// Prints the level information for the current product
		printf("%s\tProduct: %s; Buy levels: %d; Sell levels: %d\n", LOG_PREFIX, products[product], num_buy_levels, num_sell_levels);

		// Prints all levels for the current product, from the most expensive to the least
		while (num_levels > 0) {

			int max = 0;
			int max_index;

			// Finds the current most expensive level
			for (int level = 0; level < num_levels; level++) {
				if (levels[level].price > max) {
					max_index = level;
					max = levels[level].price;
				}
			}

			// Creates a string representing the level's type
			char* type_str;
			if (levels[max_index].type) {
				type_str = "SELL";
			} else {
				type_str = "BUY";
			}

			// Spelling depending on number of orders in the level
			char* order_str;
			if (levels[max_index].num > 1) {
				order_str = "orders";
			} else {
				order_str = "order";
			}

			// Prints the current level
			printf("%s\t\t%s %d @ $%d (%d %s)\n", LOG_PREFIX, type_str, levels[max_index].qty, levels[max_index].price, \
						levels[max_index].num, order_str);

			// Deletes the level from the array of levels for the current product and moves on to the next
			num_levels--;
			for (int level = max_index; level < num_levels; level++) {
				levels[level] = levels[level + 1];
			}

			levels = realloc(levels, sizeof(struct level) * num_levels);
		}
		free(levels);
	}

	// Displays the current position of all traders
	printf("%s	--POSITIONS--\n", LOG_PREFIX);

	int cursor = 0;
	while (traders[cursor] != NULL) {
		printf("%s	Trader %d: ", LOG_PREFIX, traders[cursor]->id);

		// Prints position data for each product for the current trader
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

// Disconnects a given trader from the exchange
int disconnect(struct trader** traders, struct order** orders, char** products, int argc) {

	int cursor = 0;
	int count_active = 0;

	// Locates the trader that has disconnected
	while (traders[cursor] != NULL) {
		if (disconnect_trader == traders[cursor]->pid) {
			// Displays d/c message and set's the trader to be inactive
			printf("%s Trader %d disconnected\n", LOG_PREFIX, traders[cursor]->id);
			traders[cursor]->active = 0;
		}
		// Counts the number of active traders on the exchange
		if (traders[cursor]->active) {
			count_active++;
		}
		cursor++;
	}
	// Resetting global flags
	disconnect_trader = -1;

	// Ends the exchange if there are no traders active
	if (count_active == 0) {
		printf("%s Trading completed\n", LOG_PREFIX);
		printf("%s Exchange fees collected: $%ld\n", LOG_PREFIX, total_fees);

		// Deallocates all memory allocated to orders
		cursor = 0;
		while (orders[cursor] != NULL) {
			free(orders[cursor]->product);
			free(orders[cursor++]);
		}
		free(orders);

		// unlinks all named pipes
		cursor = 0;
		while (cursor < argc - 2) {
			char path[PATH_LENGTH];
			snprintf(path, PATH_LENGTH, EXCHANGE_PATH, cursor);
			unlink(path);
			snprintf(path, PATH_LENGTH, TRADER_PATH, cursor);
			unlink(path);
			cursor++;
		}

		// Deallocates all memory allocated to traders
		cursor = 0;
		while (traders[cursor] != NULL) {
			free(traders[cursor]->position_qty);
			free(traders[cursor]->position_cost);
			free(traders[cursor++]);
		}
		free(traders);

		// Deallocates all memory allocated to products
		int limit = strtol(products[0], NULL, 10);
		for (int index = 0; index <= limit; index++) {
			free(products[index]);
		}
		free(products);

		return 1;
	}
	return 0;
}

// Runs the exchange
int main(int argc, char **argv) {

	// Validating command line arguments
	if (argc < 3) {
		printf("%s Invalid command line arguments\n", LOG_PREFIX);
		return -1;
	}

	printf("%s Starting\n", LOG_PREFIX);

	// Gets the products being traded
	char** products = read_products_file(argv[1]);
	if (products == NULL) {
		printf("%s Error: Products file invalid", LOG_PREFIX);
		return -1;
	}


	// Sets up signal handlers
	struct sigaction sig_act;

	sig_act.sa_handler = (void *)read_sig;
	sigemptyset(&sig_act.sa_mask);
	sig_act.sa_flags = SA_RESTART | SA_SIGINFO;

	sigaction(SIGCLD, &sig_act, NULL);
	sigaction(SIGUSR1, &sig_act, NULL);
	sigaction(SIGUSR2, &sig_act, NULL);

	// Creates array to store all traders
	struct trader** traders = malloc(sizeof(struct trader) * (argc - 1));
	traders[argc - 2] = NULL;
	// Initialises all traders
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

		// Gracefully exists in the event that a trader could not be started
		if (exit_flag == 1 || traders[trader-2] == NULL) {
			printf("%s Error: Given trader binary is invalid\n", LOG_PREFIX);
			int num_products = strtol(products[0], NULL, 10);
			for (int cursor = 0; cursor <= num_products; cursor++) {
				free(products[cursor]);
			}
			free(products);

			int cursor = 0;
			while (traders[cursor] != NULL) {
				free(traders[cursor]->position_qty);
				free(traders[cursor]->position_cost);
				free(traders[cursor++]);
			}
			free(traders);
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

	int time = 0;

	// Main event loop
	while (1) {

		// Waits for signals from traders before checking flags to minimise CPU usage
		pause();

		char** arg_array;
		// Locates the trader that wrote to the exchange via PID
		if (read_trader != -1) {

			int cursor = 0;
			while (traders[cursor] != NULL) {
				if (traders[cursor]->pid == read_trader && traders[cursor]->active) {
					// Get input arguments from the trader's named pipe
					arg_array = take_input(traders[cursor]->trader_fd);
					break;
				}
				cursor++;
			}
			// Reset global flag
			read_trader = -1;

			// Breaks if the returned arguments are invalid, or the trader doesnt exist
			if (arg_array == NULL || traders[cursor] == NULL) {
				continue;
			}

			// Formats and prints the returned arguments
			printf("%s [T%d] Parsing command: <", LOG_PREFIX, traders[cursor]->id);

			int arg_cursor = 0;
			while (arg_array[arg_cursor] != NULL) {

				printf("%s", arg_array[arg_cursor]);
				if (arg_array[arg_cursor + 1] != NULL) {
					printf(" ");
				}
				arg_cursor++;
			}
			printf(">\n");

			// Checks if the number of given arguments is valid for the type of order
			int valid_num_args = 0;
			if (strcmp(arg_array[0], "SELL") == 0 || strcmp(arg_array[0], "BUY") == 0) {
				valid_num_args = 5;
			} else if (strcmp(arg_array[0], "AMEND") == 0) {
				valid_num_args = 4;
			} else if (strcmp(arg_array[0], "CANCEL") == 0) {
				valid_num_args = 2;
			}

			// For the case where the order's format was invalid
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

			char* msg = malloc(MAX_INPUT);
			order_id = strtol(arg_array[1], NULL, 10);

			// Processes args depending on the number of arguments given
			if (valid_num_args == 5) { // BUY/SELL case: <BUY/SELL> <order_id> <product> <qty> <price>

				qty = strtol(arg_array[3], NULL, 10);
				price = strtol(arg_array[4], NULL, 10);
				sprintf(msg, "ACCEPTED %s;", arg_array[1]);

				// Checks validity of the given product and id
				for (int product = 1; product <= strtol(products[0], NULL, 10); product++) {
					if (strcmp(products[product], arg_array[2]) == 0) {
						product_valid = 1;
						id_valid = (order_id == traders[cursor]->current_order_id);
						break;
					}
				}
			} else if (valid_num_args == 4 || valid_num_args == 2) { // AMEND/CANCEL case

				product_valid = 1;

				int index = 0;
				// Checks the validity of the given id
				while (orders[index] != NULL) {
					if (orders[index]->trader == traders[cursor] && orders[index]->order_id == order_id) {
						id_valid = 1;
						break;
					}
					index++;
				}

				if (valid_num_args == 4) {  // AMEND case: <AMEND> <order_id> <qty> <price>

					qty = strtol(arg_array[2], NULL, 10);
					price = strtol(arg_array[3], NULL, 10);
					sprintf(msg, "AMENDED %s;", arg_array[1]);

				} else if (valid_num_args == 2) { // CANCEL case: <CANCEL> <order_id>

					qty_valid = 1;
					price_valid = 1;
					sprintf(msg, "CANCELLED %s;", arg_array[1]);
				}
			}

			// Validates the quantity and prices of relevant orders
			if (valid_num_args == 5 || valid_num_args == 4) {
				qty_valid = (qty > 0 && qty < 1000000);
				price_valid = (price > 0 && price < 1000000);
			}

			if (id_valid && product_valid && qty_valid && price_valid) {
				// Inform the trader that their order was accepted
				if (traders[cursor]->active) {
					write_pipe(traders[cursor]->exchange_fd, msg);
					kill(traders[cursor]->pid, SIGUSR1);
					free(msg);
				}

				// Increments a given trader's id counter for relevant orders
				if (strcmp(arg_array[0], "SELL") == 0 || strcmp(arg_array[0], "BUY") == 0) {
					traders[cursor]->current_order_id++;
				}

				// Processes orders dependent on their type
				if (strcmp(arg_array[0], "BUY") == 0) {
					orders = create_order(BUY, products, traders[cursor], order_id, arg_array[2], qty, price, &buy_order, orders, traders, time++);

				} else if (strcmp(arg_array[0], "SELL") == 0) {
					orders = create_order(SELL, products, traders[cursor], order_id, arg_array[2], qty, price, &sell_order, orders, traders, time++);

				} else if (strcmp(arg_array[0], "AMEND") == 0) {
					orders = create_order(AMEND, products, traders[cursor], order_id, NULL, qty, price, &amend_order, orders, traders, time++);

				} else if (strcmp(arg_array[0], "CANCEL") == 0) {
					orders = create_order(CANCEL, products, traders[cursor], order_id, NULL, 0, 0, &cancel_order, orders, traders, time);
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
			// Deallocates memory allocated to the argument array
			while (arg_array[cursor] != NULL) {
				free(arg_array[cursor++]);
			}
			free(arg_array);
		}

		// Checks if any traders have disconnected
		if (disconnect_trader != -1) {
			if (disconnect(traders, orders, products, argc)) {
				return 0;
			}
			continue;
		}
	}
}
