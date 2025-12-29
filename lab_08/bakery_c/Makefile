CLIENT = bakery_client
SERVER = bakery_server

SOURCES_CLNT.c = 
SOURCES_CLNT.h = 
SOURCES_SVC.c = 
SOURCES_SVC.h = 
SOURCES.x = bakery.x

TARGETS_SVC.c = bakery_svc.c bakery_server.c bakery_xdr.c 
TARGETS_CLNT.c = bakery_clnt.c bakery_client.c bakery_xdr.c 
TARGETS = bakery.h bakery_xdr.c bakery_clnt.c bakery_svc.c bakery_client.c bakery_server.c

OBJECTS_CLNT = $(SOURCES_CLNT.c:%.c=%.o) $(TARGETS_CLNT.c:%.c=%.o)
OBJECTS_SVC = $(SOURCES_SVC.c:%.c=%.o) $(TARGETS_SVC.c:%.c=%.o)

# Флаги для Linux и libtirpc
CPPFLAGS += -D_REENTRANT
CFLAGS += -g -I/usr/include/tirpc
LDLIBS += -lnsl -lpthread -ltirpc

all : $(CLIENT) $(SERVER)

$(OBJECTS_CLNT) : $(SOURCES_CLNT.c) $(SOURCES_CLNT.h) $(TARGETS_CLNT.c) 

$(OBJECTS_SVC) : $(SOURCES_SVC.c) $(SOURCES_SVC.h) $(TARGETS_SVC.c) 

$(CLIENT) : $(OBJECTS_CLNT) 
	$(CC) -o $(CLIENT) $(OBJECTS_CLNT) $(LDLIBS) 

$(SERVER) : $(OBJECTS_SVC) 
	$(CC) -o $(SERVER) $(OBJECTS_SVC) $(LDLIBS)

clean:
	 $(RM) core $(OBJECTS_CLNT) $(OBJECTS_SVC) $(CLIENT) $(SERVER)