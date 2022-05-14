1. Describe how your exchange works.

  As per the spec, the exchange works (from an abstract view), by receiving orders via named pipes from traders (started by the exchange as its children), validating them, processing/matching and then storing them if required. All messages are sent and received using a read-on-signal system, where pipes are only read from if a signal has been sent from the sender indicating that they have sent a message.

  Order matching is done using price time priority, which matches the most economically appropriate orders together. e.g. a buy order will be matched with the cheapest sell order, and if multiple valid sell orders exist at that price level, the oldest of those orders.

2. Describe your design decisions for the trader and how it's fault-tolerant.

  In order to make my auto trader fault tolerant, I first noted that the main fault that could occur would be loss of signals, as error checking of messages to traders is already trivially correct, as proved by the testing of spx_exchange below.

  In order to fix this problem, where signals sent from the trader to the exchange in busy conditions could be lost, I implemented an exponential back-off algorithm, which sends signals to the exchange at exponentially increasing intervals until the desired APPROVED message is received.

  In terms of other design decisions, my trader will send a message, then wait for it to be approved before sending the next (if it exists yet), rather than queuing up orders that  have been sent and not accepted yet as this would lead to an extra layer of complexity that isn't required for this task.

3. Describe your tests and how to run them.

  I have two sets of tests; end to end and unit-tests. These tests aim to achieve three goals, code coverage, edge case checking and correctness. In each set of tests, functions are passed invalid data, out of bounds data as well as valid data in order to check as many possible outputs as currently possible.

  In order to run my tests, use my bash script run_tests.sh.

  ```bash
  bash run_tests.sh
  ```

  This will first run all of my e2e tests, which use auto traders to simulate the real use of this exchange, covering almost all code in the program (combined coverage ~97%). Each test prints it's own code coverage as it is completed.
  Then the script runs my cmocka unit tests, which also creates a coverage report for these specific tests, with coverage of ~95%.

  Also, note that only write_pipe, read_products_file, and order processing functions are included in the unit tests, as order book functions produce only stdout output and cannot be tested in this way. Similarly, functions such as disconnect, and many error checking components (price/qty/order_id/product validity), residing in main cannot be included, and are excluded from my unit tests, instead being tested primarily in my e2e testcases.

  Also note, for some reason gcov doesnt register the code coverage for my e2e tests on ed, however this is not an issue when running locally. 
