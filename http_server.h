/*
	Developer: Yana Brushtein
	Date: 18/02/2023
	Reviewer: Aviv
	Status: Approved
*/

#ifndef __CR6_HTTP_SERVER_H__
#define __CR6_HTTP_SERVER_H__

typedef struct server_handle server_handle_t;

server_handle_t *Init(int port);
int RunServer(server_handle_t *srv_params);
void Destroy(server_handle_t *srv_params);

#endif /* __CR6_HTTP_SERVER_H__ */
