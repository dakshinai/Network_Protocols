#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

struct queuenode {
	struct queuenode *prev, *next;
	int pageno;
	time_t timestamp;
	time_t expirystamp;
	char etag[50];
	char *contents;
	char modifystampstr[30];
};

struct queue {
	int count;
	int capacity;
	struct queuenode *front, *rear;
};

struct hash {
	int capacity;
	struct queuenode **array;
	char **urls;
	int urlcount;
};

struct queuenode* CreateNewQueueNode(int pagenumber, char *contents,
		time_t expirystamp, char *etag,char *modifystampstr) {
	struct queuenode* temp = (struct queuenode*) malloc(
			sizeof(struct queuenode));
	temp->pageno = pagenumber;
	temp->prev = temp->next = NULL;

	temp->contents = (char*) malloc(strlen(contents) + 1);
	strcpy(temp->contents, contents);

	time_t currenttime;
	time(&currenttime);
	temp->timestamp = currenttime;
	temp->expirystamp = expirystamp;
	strcpy(temp->modifystampstr,modifystampstr);

	strcpy(temp->etag,etag);
	return temp;
}

struct queue* CreateNewQueue(int capacity) {
	struct queue* tempqueue = (struct queue*) malloc(sizeof(struct queue));
	tempqueue->count = 0;
	tempqueue->capacity = capacity;
	tempqueue->front = tempqueue->rear = NULL;

	return tempqueue;
}

struct hash* CreateHash(int capacity) {
	struct hash *thash = (struct hash*) malloc(sizeof(struct hash));

	thash->capacity = capacity;
	thash->array = (struct queuenode**) malloc(
			thash->capacity * sizeof(struct queuenode*));
	thash->urls = (char**) malloc(capacity * sizeof(char*));
	thash->urlcount = 0;

	int i;
	for (i = 0; i < thash->capacity; i++) {
		thash->array[i] = NULL;
	}

	return thash;
}

int IsQueueFull(struct queue * queue) {
	return queue->count == queue->capacity;
}

int IsQueueEmpty(struct queue *queue) {
	return queue->rear == NULL;
}

void Dequeue(struct queue *queue) {
	if (IsQueueEmpty(queue)) {
		return;
	}

	struct queuenode *temp = queue->rear;

	if (queue->front == queue->rear)
		queue->front = NULL;

	queue->rear = queue->rear->prev;
	if (queue->rear)
		queue->rear->next = NULL;

	free(temp);

	queue->count--;
}

void Enqueue(struct queue * queue, struct hash *thash, int pageno,
		char *contents, time_t expirystamp, char *etag,char *modifystampstr) {
	if (IsQueueFull(queue)) {

		thash->array[queue->rear->pageno] = NULL;
		Dequeue(queue);
	}

	struct queuenode *newnode = CreateNewQueueNode(pageno, contents,
			expirystamp, etag,modifystampstr);

	if (IsQueueEmpty(queue)) {
		queue->front = queue->rear = newnode;

	} else {
		newnode->next = queue->front;
		queue->front->prev = newnode;
		queue->front = newnode;
	}

	thash->array[pageno] = newnode;

	queue->count++;

}

int IsPageInCache(struct queue * queue, struct hash* thash, char* fullurl) {

	int i;
	for (i = 0; i < thash->urlcount; i++) {
		if (strcmp(fullurl, thash->urls[i]) == 0)
			break;
	}

	if (i < thash->urlcount) {

		struct queuenode *page = thash->array[i + 1];
		if (page != NULL)
			return 1;
	}

	return 0;

}

void ReferencePage(struct queue * queue, struct hash* thash, char* fullurl,
		char *contents, int refreshContents, time_t expirystamp,char *etag,char *modifystampstr) {

	int i;
	int pageno;
	for (i = 0; i < thash->urlcount; i++) {
		if (strcmp(fullurl, thash->urls[i]) == 0)
			break;
	}

	if (i < thash->urlcount) {
		pageno = i + 1;
	} else {

		thash->urlcount++;
		if (thash->urlcount > thash->capacity) {
			perror("Cannot accomodate more new pages");
			exit(1);
		}
		thash->urls[thash->urlcount - 1] = (char*) malloc(strlen(fullurl) + 1);
		strcpy(thash->urls[thash->urlcount - 1], fullurl);
		pageno = thash->urlcount;
	}

	struct queuenode *page = thash->array[pageno];
	if (page == NULL)
		Enqueue(queue, thash, pageno, contents, expirystamp, etag,modifystampstr);
	else if (page != queue->front) {

		page->prev->next = page->next;
		if (page->next)
			page->next->prev = page->prev;

		if (page == queue->rear)
			queue->rear = page->prev;

		page->next = queue->front;
		queue->front->prev = page;
		page->prev = NULL;
		queue->front = page;

	}

	if (refreshContents) {
		free(page->contents);
		page->contents = (char*) malloc(strlen(contents) + 1);
		page->expirystamp = expirystamp;
		time_t currenttime;
		time(&currenttime);
		page->timestamp = currenttime;
		strcpy(page->contents, contents);
	}

}

/*int main() {

 struct queue *queue = CreateNewQueue(4);
 struct hash *hash = CreateHash(10);

 time_t currenttime;
 time(&currenttime);

 ReferencePage(queue, hash, "http://165.91.215.188/1.txt", "aabb", 0,
 currenttime);
 ReferencePage(queue, hash, "http://165.91.215.188/2.txt", "ccdd", 0,
 currenttime);
 ReferencePage(queue, hash, "http://165.91.215.188/3.txt", "eeff", 0,
 currenttime);
 ReferencePage(queue, hash, "http://165.91.215.188/1.txt", "xxyy", 1,
 currenttime);
 ReferencePage(queue, hash, "http://165.91.215.188/4.txt", "gghh", 0,
 currenttime);
 ReferencePage(queue, hash, "http://165.91.215.188/5.txt", "iijj", 0,
 currenttime);

 printf("http://165.91.215.188/1.txt:%d\n",
 IsPageInCache(queue, hash, "http://165.91.215.188/1.txt"));
 printf("http://165.91.215.188/2.txt:%d\n",
 IsPageInCache(queue, hash, "http://165.91.215.188/2.txt"));
 printf("http://165.91.215.188/3.txt:%d\n",
 IsPageInCache(queue, hash, "http://165.91.215.188/3.txt"));
 printf("http://165.91.215.188/4.txt:%d\n",
 IsPageInCache(queue, hash, "http://165.91.215.188/4.txt"));
 printf("http://165.91.215.188/5.txt:%d\n",
 IsPageInCache(queue, hash, "http://165.91.215.188/5.txt"));

 struct queuenode *temp = queue->front;

 struct tm timestruct;
 time_t time;

 while (temp != NULL) {

 printf("\n%s:%s is page %d with contents %s", ctime(&temp->timestamp),
 hash->urls[temp->pageno - 1], temp->pageno, temp->contents);

 strptime(ctime(&temp->timestamp), "%a %b %d %H:%M:%S %Y", &timestruct);
 time = mktime(&timestruct);

 printf("\nConverted time is %s\n", ctime(&time));

 temp = temp->next;
 }

 printf("\n");
 return 0;
 }*/
