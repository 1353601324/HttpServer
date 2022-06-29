bin=main
cgi=test_cgi
cgisrc=cgi/test_cgi.cc
src=main.cc
cc=g++
FD=-std=c++11 -lpthread

.PHONY:all
all:$(bin) $(cgi)

$(cgi):$(cgisrc)
	$(cc) -o $@ $^ $(FD)
$(bin):$(src)
	$(cc) -o $@ $^ $(FD)

.PHONY:clean
clean:
	rm -f $(bin)
	rm -f $(cgi)
	rm -rf output

.PHONY:out
out:
	mkdir -p output
	cp $(bin) output
	cp $(cgi) wwwroot
	cp -rf wwwroot output
