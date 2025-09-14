/* 
 * echoserveri.c - An iterative echo server 
 */ 
/* $begin echoserverimain */
#include "csapp.h"

typedef struct {
	int maxfd;
	fd_set read_set;
	fd_set ready_set;
	int nready;
	int maxi;
	int clientfd[FD_SETSIZE];
	rio_t clientrio[FD_SETSIZE];
} pool;
typedef struct item{
	int ID;
	int left_stock;
	int price;
	int readcnt;
	sem_t mutex;
	struct item* left_item;
	struct item* right_item;
} item;
item* root = NULL;
int check_connected = 0;
int connected_client = 0;
void echo(int connfd);
void init_pool(int listenfd, pool* p);
void add_client(int connfd, pool* p);
void check_clients(pool* p);
void insert(item* compare, int ID, int left_stock, int price);
item* makeInit(int ID, int left_stock, int price);
void traverse(item* node, char* buf);
void sell_check(item* node, int ID, int left_stock);
int buy_check(item* node, int ID, int left_stock);
void apply_result(item* node, FILE* fp);

int main(int argc, char **argv) 
{
    int listenfd, connfd, stockId, stockNum, stockPrice;
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;  /* Enough space for any address */  //line:netp:echoserveri:sockaddrstorage
    static pool pool;
    char client_hostname[MAXLINE], client_port[MAXLINE];
    FILE* fp;

    if (argc != 2) {
	fprintf(stderr, "usage: %s <port>\n", argv[0]);
	exit(0);
    }
   
    fp = Fopen("stock.txt", "r");
    while(fscanf(fp, "%d %d %d\n", &stockId, &stockNum, &stockPrice) != EOF){
    	if(root == NULL){
		root = makeInit(stockId, stockNum, stockPrice);
	}
	else{
		insert(root, stockId, stockNum, stockPrice);
	}
    }
    Fclose(fp);

    listenfd = Open_listenfd(argv[1]);
    init_pool(listenfd, &pool);
    while (1) {
	pool.ready_set = pool.read_set;
	pool.nready = Select(pool.maxfd + 1, &pool.ready_set, NULL, NULL, NULL);
	if(FD_ISSET(listenfd, &pool.ready_set)){
		clientlen = sizeof(struct sockaddr_storage); 
		connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
		add_client(connfd, &pool);
        	Getnameinfo((SA *) &clientaddr, clientlen, client_hostname, MAXLINE, 
                    client_port, MAXLINE, 0);
       		printf("Connected to (%s, %s)\n", client_hostname, client_port);
	}
	check_clients(&pool);
	if(check_connected){
		if(connected_client == 0){
			break;
		}
	}
    }
    fp = Fopen("stock.txt", "w");
    apply_result(root, fp);
    Fclose(fp);
    exit(0);
}

void init_pool(int listenfd, pool* p){
	for(int i = 0 ; i < FD_SETSIZE ; i++){
		p->clientfd[i] = -1;
	}
	FD_ZERO(&p->read_set);
	p->maxfd = listenfd;
	p->maxi = -1;
	FD_SET(listenfd, &p->read_set);
}

void add_client(int connfd, pool* p){
	int i;
	p->nready--;
	for(i = 0 ; i < FD_SETSIZE ; i++){
		if(p->clientfd[i] < 0){
			p->clientfd[i] = connfd;
			FD_SET(connfd, &p->read_set);
			if(p->maxfd < connfd){
				p->maxfd = connfd;
			}
			if(p->maxi < i){
				p->maxi = i;
			}
			Rio_readinitb(&p->clientrio[i] ,connfd);
			check_connected = 1;
			connected_client++;
			break;
		}
	}
	if(i == FD_SETSIZE){
		app_error("add_client error: Too many clients");
	}
}

void check_clients(pool *p){
	int i, connfd, n, stockNum, stockId;
	char* token, *stock_num, *stock_id;
	char buf[MAXLINE];
	rio_t rio;

	for(i = 0 ; (i <= p->maxi) && (p->nready > 0); i++){
		connfd = p->clientfd[i];
		rio = p->clientrio[i];
		
		if((connfd > 0) && FD_ISSET(connfd, &p->ready_set)){
			if((n = Rio_readlineb(&rio, buf, MAXLINE)) != 0){
				printf("Server received %d bytes on fd %d\n", n,connfd);
				buf[n-1] = '\0';
				token = strtok(buf," ");
				if(!strcmp(token, "show")){
					buf[0] = '\0';
					traverse(root, buf);
					strcat(buf, "\0");
				}
				else if(!strcmp(token, "buy")){
					token = strtok(NULL, " ");
                                        stock_id = token;
                                        token = strtok(NULL, " ");
                                        stock_num = token;
                                        stockId = atoi(stock_id);
                                        stockNum = atoi(stock_num);
					if(buy_check(root, stockId, stockNum)){
						strcpy(buf, "[buy] success\n");
					}
					else{
						strcpy(buf, "Not enough left stock\n");
					}
				}
				else if(!strcmp(token, "sell")){
					token = strtok(NULL, " ");
					stock_id = token;
					token = strtok(NULL, " ");
					stock_num = token;
					stockId = atoi(stock_id);
					stockNum = atoi(stock_num);
					sell_check(root, stockId, stockNum);
					strcpy(buf, "[sell] success\n");
				}
				
				if(!strcmp(token, "exit")){
					Close(connfd);
					FD_CLR(connfd, &p->read_set);
					p->clientfd[i] = -1;
					connected_client--;
					if(p->maxi == i){
						for(int j = p->maxi ; j >= 0 ; j--){
							if(p->clientfd[j] != -1){
								p->maxi = j;
								break;
							}
						}
					}
					if(p->maxfd == connfd){
						p->maxfd = 3;
						for(int j = p->maxi ; j >= 0 ; j--){
							if(p->maxfd < p->clientfd[j]){
								p->maxfd = p->clientfd[j];
							}
						}
					}
				}
				else{
					Rio_writen(connfd, buf, MAXLINE);
				}
				buf[0] = '\0';
			}
			else{
				Close(connfd);
				FD_CLR(connfd, &p->read_set);
				p->clientfd[i] = -1;
				connected_client--;
				 if(p->maxi == i){
                                        for(int j = p->maxi ; j >= 0 ; j--){
                                                if(p->clientfd[j] != -1){
                                                        p->maxi = j;
                                                        break;
                                                }
                                        }
                                 }
                                 if(p->maxfd == connfd){
                                        p->maxfd = 3;
                                        for(int j = p->maxi ; j >= 0 ; j--){
                                                if(p->maxfd < p->clientfd[j]){
                                                        p->maxfd = p->clientfd[j];
                                                }
                                        }
                                 }
			}
			p->nready--;
		}
	}
}

void insert(item* compare, int ID, int left_stock, int price){
	item* temp;
	
	if(compare->ID > ID){
		if(compare->left_item == NULL){
			temp = makeInit(ID, left_stock, price);
			compare->left_item = temp;
		}
		else{
			insert(compare->left_item, ID, left_stock, price);
		}
	}
	else{
		if(compare->right_item == NULL){
			temp = makeInit(ID, left_stock, price);
			compare->right_item = temp;
		}
		else{
			insert(compare->right_item, ID, left_stock, price);
		}
	}
}

item* makeInit(int ID, int left_stock, int price){
	item* temp = (item*)malloc(sizeof(item));
	
	temp->ID = ID;
	temp->left_stock = left_stock;
	temp->price = price;
	temp->readcnt = 0;
	temp->left_item = NULL;
	temp->right_item = NULL;

	return temp;
}

void traverse(item* node, char* buf){
	if(node == NULL){
		return;
	}
	char temp[10];
	
	sprintf(temp, "%d", node->ID);
	strcat(buf,temp);
	strcat(buf, " ");

	sprintf(temp, "%d", node->left_stock);
	strcat(buf, temp);
	strcat(buf, " ");

	sprintf(temp, "%d", node->price);
	strcat(buf, temp);
	strcat(buf, "\n");
	traverse(node->left_item, buf);
	traverse(node->right_item, buf);
}

void sell_check(item* node, int ID, int left_stock){
	if(node == NULL){
		return;
	}
	if(node->ID == ID){
		node->left_stock += left_stock;
		return;
	}
	
	if(ID < node->ID){
		sell_check(node->left_item, ID, left_stock);
	}
	else{
		sell_check(node->right_item, ID, left_stock);
	}
}

int buy_check(item* node, int ID, int left_stock){
	int n1 = 0, n2 = 0;

	if(node == NULL){
		return 0;
	}

	if(node->ID == ID){
		if(node->left_stock >= left_stock){
			node->left_stock -= left_stock;
			return 1;
		}
		else{
			return 0;
		}
	}

	if(ID < node->ID){
		n1 = buy_check(node->left_item, ID, left_stock);
	}
	else{
		n2 = buy_check(node->right_item, ID, left_stock);
	}

	if(n1 | n2){
		return 1;
	}
	else{
		return 0;
	}
}

void apply_result(item* node, FILE* fp){
	if(node == NULL){
		return;
	}
	fprintf(fp, "%d %d %d\n", node->ID, node->left_stock, node->price);
	apply_result(node->left_item, fp);
	apply_result(node->right_item, fp);
}
/* $end echoserverimain */
