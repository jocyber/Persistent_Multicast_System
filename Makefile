FLAGS := -Wall -pedantic-errors -pthread -g
CLIENT := participant
SERV := coordinator

all: $(CLIENT) $(SERV)

$(CLIENT): $(CLIENT)/$(CLIENT).cpp $(CLIENT)/part_functions.cpp
	g++ $(FLAGS) $^ -o part	

$(SERV): $(SERV).cpp
	g++ $(FLAGS) $^ -o coor	

clean:
	rm -f part coor

serv:
	./coor coor_config.txt

part: 
	./part part_config.txt
