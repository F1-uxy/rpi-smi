TARGET := smi_irq
TEST_TARGET := smi_test_poll

KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
BUILD_DIR := build

SOURCES := smi_irq.c
TEST_SOURCES := test_poll.c

obj-m := $(TARGET).o

all:
	@mkdir -p $(BUILD_DIR)
	$(MAKE) -C $(KDIR) M=$(PWD) modules
	mv $(TARGET).ko $(BUILD_DIR)/

test:
	gcc $(TEST_SOURCES) -o $(BUILD_DIR)/$(TEST_TARGET)

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	@rm -rf $(BUILD_DIR)/*