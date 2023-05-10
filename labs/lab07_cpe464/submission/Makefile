# Makefile to extract the project and compile everything

all:
	mkdir -p submission
	tar -xf submission.tar -C submission
	make -C submission -f Makefile all
	cp ./submission/rcopy ./submission/server ./

clean_project:
	make -C submission -f Makefile clean

clean:
	rm -rf submission rcopy server
