translator: translator.cpp translator.h turing_machine.cpp turing_machine.h symbol_set.cpp symbol_set.h
	g++ -Wall -Wextra $(filter %.cpp,$^) -g -o $@

tm_interpreter: tm_interpreter.cpp turing_machine.cpp turing_machine.h
	g++ -Wall -Wshadow $(filter %.cpp,$^) -o $@

clean:
	rm -rf tm_interpreter *~
