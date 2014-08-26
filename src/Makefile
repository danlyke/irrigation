CC=g++
CFLAGS= -std=c++11 -Wall -Wextra -Weffc++ -g -fPIC -I /usr/include/postgresql
LDFLAGS= -lmicrohttpd

# -lsqlite3 -lpq -lboost_filesystem -lboost_system -lgdbm -lpcre -lboost_iostreams  -l boost_program_options

SOURCES=server.cpp

all : ../obj ../bin/server 


vpath %.cpp .
$(OBJECTS): | obj
OBJECTS=$(patsubst %.cpp,../obj/%.o,$(SOURCES))

../obj/%.o : %.cpp

../obj :
	@mkdir -p $@

../obj/%.o : %.cpp
	$(CC) -c $(CFLAGS) $< -o $@
	$(CC) -MM $(CFLAGS) $< > ../obj/../obj/$*.d
	@mv -f ../obj/$*.d ../obj/$*.d.tmp
	@sed -e 's|.*:|$*.o:|' < ../obj/$*.d.tmp > ../obj/$*.d
	@sed -e 's/.*://' -e 's/\\$$//' < ../obj/$*.d.tmp | fmt -1 | \
	  sed -e 's/^ *//' -e 's/$$/:/' >> ../obj/$*.d
	@rm -f ../obj/$*.d.tmp

clean:
	-rm -f $(OBJECTS)


../bin/server : ../obj/server.o
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS) 

