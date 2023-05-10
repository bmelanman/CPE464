# Makefile to extract the project and compile everything

TOP_DIR = lab07_submission

all:
	mkdir -p lab07_submission
	tar -C $(TOP_DIR) -xf submission.tar
	make -C $(TOP_DIR) -f Makefile all
	cp $(TOP_DIR)/rcopy $(TOP_DIR)/server ./

new_tar:
	tar -cf submission.tar ../*.c ../*.h ../Makefile ../given_files

clean:
	rm -rf $(TOP_DIR) rcopy server
