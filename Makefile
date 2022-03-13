FLAGS := -Wall -pedantic-errors -pthread -g
CLIENT := participant
SERV := coordinator

all:
	$(CLIENT) $(SERV)

%: %.cpp
	g++ $(FLAGS) $^ -o $@	

clean:
	rm -f $(CLIENT) $(SERV)

serv:
	./$(SERV)

part: 
	./$(CLIENT)
