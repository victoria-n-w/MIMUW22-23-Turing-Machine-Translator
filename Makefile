translator: translator.cpp turing_machine.cpp turing_machine.h 
	g++ -Wall -Wextra $(filter %.cpp,$^) -o $@

tm_interpreter: tm_interpreter.cpp turing_machine.cpp turing_machine.h
	g++ -Wall -Wshadow $(filter %.cpp,$^) -o $@

clean:
	rm -rf tm_interpreter *~