/*
libactor - A C Actor Library
example.c
Copyright (C) 2009 Chris Moos

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include <stdio.h>

#if defined(WIN32)
#	include <windows.h>
void sleep(unsigned int seconds)
{
	Sleep(seconds*1000);
}
#else
#	include <sys/socket.h>
#	include <netinet/in.h>
#	include <arpa/inet.h>
#endif // defined(WIN32)

#include <libactor/actor.h>

enum {
	PING_MSG = 100,
	PONG_MSG,
	ECHO_CLIENT_INFO
};

/*
Trap exit example
*/
void *trap_exit_die(void *args) {
	sleep(5);
	return 0;
}

void *trap_exit(void *args) {
	actor_msg_t *msg;
	actor_id aid;
	
	actor_trap_exit(1);
	aid = spawn_actor(trap_exit_die, NULL);
	printf("Waiting for actor to die...\n");
	msg = actor_receive();
	if(msg->type == ACTOR_MSG_EXITED) {
		printf("An actor died! ID: %d\n", msg->sender);
	}
	arelease(msg);
	return 0;
}

/*
Echo Server Example
*/

void *echo_client(void *args) {
	actor_msg_t *msg;
	struct sockaddr_in *remote;
	char buf[32];
	int sock = (int)args;
	int ret;

	msg = actor_receive();
	if(msg->type == ECHO_CLIENT_INFO) {
		remote = (struct sockaddr_in*)msg->data;
		printf("Echo client connected: %s\n", inet_ntoa(remote->sin_addr));
		
		while((ret = recv(sock, buf, 32, 0)) > 0) {
			if(send(sock, buf, ret, 0) == -1) break;
		}
		printf("Echo client disconnected: %s\n", inet_ntoa(remote->sin_addr));
	}
	arelease(msg);
	return 0;
}

void *echo_server(void *args) {
	struct sockaddr_in local, remote;
	int sockfd, clientsock;
	int socklen = sizeof(struct sockaddr_in);
	int sockoption;
	actor_id aid;
	
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(9999);
	sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	
	/* Set SO_REUSEADDR */
	sockoption = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&sockoption, sizeof(sockoption));
	
	if(bind(sockfd, (struct sockaddr*)&local, sizeof(struct sockaddr_in)) == -1) {
		perror("bind");
		return 0;
	}
	listen(sockfd, 5);
	printf("Echo server listening on port: 9999\n");
	while(1) {
		if((clientsock = accept(sockfd, (struct sockaddr*)&remote, &socklen)) != -1) {
			aid = spawn_actor(echo_client, (void*)clientsock);
			actor_send_msg(aid, ECHO_CLIENT_INFO, (void*)&remote, sizeof(struct sockaddr_in));
		} else break;
	}
	printf("Echo server exiting...\n");
	return 0;
}

/* 
PING/PONG Example
*/

void *pong_func(void *args) {
	actor_msg_t *msg;
	
	while(1) {
		msg = actor_receive();
		if(msg->type == PING_MSG) {
			printf("PING! ");
			actor_reply_msg(msg, PONG_MSG, NULL, 0);
		}
		arelease(msg);
	}
	return 0;
}

void *ping_func(void *args) {
	actor_msg_t *msg;
	actor_id aid = spawn_actor(pong_func, NULL);
	while(1) {
		actor_send_msg(aid, PING_MSG, NULL, 0);
		msg = actor_receive();
		if(msg->type == PONG_MSG) printf("PONG!\n");
		arelease(msg);
		sleep(5);
	}
	return 0;
}


void *main_func(void *args) {
	struct actor_main *main = (struct actor_main*)args;
	int x;
	
	/* Accessing the arguments passed to the application */
	printf("Number of arguments: %d\n", main->argc);
	for(x = 0; x < main->argc; x++) printf("Argument: %s\n", main->argv[x]);
	
	/* PING/PONG example */
	spawn_actor(ping_func, NULL);
	
	/* Echo server */
	spawn_actor(echo_server, NULL);
	
	/* Trap exit example */
	spawn_actor(trap_exit, NULL);
}

DECLARE_ACTOR_MAIN(main_func)
