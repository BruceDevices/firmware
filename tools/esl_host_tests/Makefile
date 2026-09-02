CC ?= cc
CFLAGS := -std=c11 -Wall -Wextra -Werror -O0 -g
VENDOR_CFLAGS := -std=c11 -Wall -Wextra -O0 -g

ESL := ../../src/modules/ir/esl
INC := -I$(ESL) -I.

VENDOR_SRC := $(ESL)/protocol/tagtinker_proto.c
PURE_SRC := $(wildcard $(ESL)/esl_*.c)
TEST_SRC := $(wildcard test_*.c)
TESTS := $(TEST_SRC:.c=)

.PHONY: test clean
test: $(TESTS)
	@fail=0; \
	for t in $(TESTS); do printf '\n--- %s ---\n' "$$t"; ./$$t || fail=1; done; \
	if [ $$fail -ne 0 ]; then printf '\nSOME TESTS FAILED\n'; exit 1; fi; \
	printf '\nALL TESTS PASSED\n'

vendor.o: $(VENDOR_SRC)
	$(CC) $(VENDOR_CFLAGS) $(INC) -c $< -o $@

$(TESTS): %: %.c vendor.o $(PURE_SRC)
	$(CC) $(CFLAGS) $(INC) -o $@ $< $(PURE_SRC) vendor.o

clean:
	rm -f $(TESTS) vendor.o
