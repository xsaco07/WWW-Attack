SERVER = server.c
CCLIENT = httpclient.c
PYCLIENT = httpclient.py
STRESS = Stress.py
PORT = 8088
RES_PATH = localhost:$(PORT)/a.html

server: build_server
	clear ; ./server -n 2 -w www -p $(PORT)

build_server:
	gcc -o server ../lib/utils.c $(SERVER)

cclient: build_cclient
	clear ; ./httpclient -u $(RES_PATH)

build_cclient:
	gcc $(CCLIENT) -lcurl -o httpclient

pyclient:
	python $(PYCLIENT) -u $(RES_PATH)

stress:
	python $(STRESS) -n 100 $(PYCLIENT) -u $(RES_PATH)
