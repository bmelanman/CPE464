# Makefile to extract the project and compile everything

all:
	mkdir -p lab07_submission
	tar -xf submission.tar -C submission
	make -C submission -f Makefile all
	cp ./submission/rcopy ./submission/server ./

new_tar:
	tar -cf submission.tar ../*.c ../*.h ../Makefile ../given_files

clean_project:
	make -C submission -f Makefile clean

clean:
	rm -rf submission rcopy server
