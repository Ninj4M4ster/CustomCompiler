FLAGS = -std=c++17 -O3

all: kompilator

kompilator: lexer.o parser.o code_generator.o symbol_table.o compiler.o
	$(CXX) $^ -o $@
	strip $@

%.o: %.cpp
	$(CXX) $(FLAGS) -c $^

lexer.cpp: lexer.l parser.h
	flex -o $@ $<

parser.cpp parser.h: parser.y
	bison -Wall -d -o parser.cpp $^

clean:
	rm -f *.o parser.cpp parser.hpp lexer.cpp kompilator