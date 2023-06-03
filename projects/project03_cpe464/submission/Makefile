# Makefile to extract the project and compile everything

TOP_DIR = proj3_submission

all:
	mkdir -p $(TOP_DIR)
	tar -C $(TOP_DIR) -xf submission.tar
	make -C $(TOP_DIR) -f Makefile all
	cp $(TOP_DIR)/rcopy $(TOP_DIR)/server ./

tar:
	make libcpe -C ../ -f Makefile
	tar -cf submission.tar ../*.c ../*.h ../Makefile ../README ../*.a
	make clean -C ../ -f Makefile

clean:
	rm -rf $(TOP_DIR) rcopy server
