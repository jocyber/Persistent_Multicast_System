FLAGS := -Wall -pedantic-errors -pthread -g
CLIENT := participant
SERV := coordinator

all:
	participant/$(CLIENT) $(SERV)

%: %.cpp $(CLIENT)/part_functions.cpp
	g++ $(FLAGS) $^ -o $@	

clean:
	rm -f $(CLIENT) $(SERV)

serv:
	./$(SERV)

part: 
	./$(CLIENT)
