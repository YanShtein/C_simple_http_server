#include <stdio.h>          /*for printing*/
#include "http_server.h"    /*http_server functions*/

#define PORT 8000

int main(void)
{
    server_handle_t *my_server = NULL;
    my_server = Init(PORT);

    if (my_server)
    {
        printf("Socket created and bounded to address\n");
    }

    RunServer(my_server);

    Destroy(my_server);

    return 0;
}