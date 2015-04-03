#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
//#include "csapp.h"
#include "memlib.c"

#define WSIZE 4
#define DSIZE 8
#define CHUNKSIZE (1<<12)

#define MAX(x,y) ((x)>(y)?(x):(y))

#define PACK(size,alloc) ((size) | (alloc))

#define GET(p) (*(unsigned int*)(p))
#define PUT(p,val) (*(unsigned int*)(p)=(val))

#define GET_SIZE(p) (GET(p) & (~7))
#define GET_ALLOC(p) (GET(p) & 1)

#define HDRP(bp) ((char *)(bp) -WSIZE)
#define FTRP(bp) ((char *)(bp)+GET_SIZE(HDRP(bp))-DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp)-DSIZE))

static char *heap_listp=0;

int mm_init(void);
static void *extend_heap(size_t words);
void mm_free(void *bp);
static void *coalesce(void *bp);
void *mm_malloc(size_t size);
static void *find_fit(size_t asize);
static void place(void *bp,size_t asize);

int mm_init(void){
	if((heap_listp=mem_sbrk(4*WSIZE)) == (void*)-1)
		return -1;
	
	PUT(heap_listp,0);
	PUT(heap_listp + WSIZE , PACK(DSIZE,1));
	PUT(heap_listp + 2*WSIZE, PACK(DSIZE,1));
	PUT(heap_listp + 3*WSIZE, PACK(0,1));
	heap_listp+=(2*WSIZE);

	if(extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;
	return 0;
}


static void *extend_heap(size_t words){
	char *bp;
	size_t size;
	size=(words%2) ? (words+1)*WSIZE : words*WSIZE;
	if((long)(bp=mem_sbrk(size))==-1)
		return NULL;

	PUT(HDRP(bp),PACK(size,0));
	PUT(FTRP(bp),PACK(size,0));
	PUT(HDRP(NEXT_BLKP(bp)),PACK(0,1));

	return coalesce(bp);
}

void mm_free(void *bp){
	size_t size=GET_SIZE(HDRP(bp));

	PUT(HDRP(bp),PACK(size,0));
	PUT(FTRP(bp),PACK(size,0));
	coalesce(bp);
}

static void *coalesce(void *bp){
	size_t prev_alloc=GET_ALLOC(FTRP( PREV_BLKP(bp)));
	size_t next_alloc=GET_ALLOC(HDRP( NEXT_BLKP(bp)));
	size_t size=GET_SIZE(HDRP(bp));

	if(prev_alloc && next_alloc)
		return bp;
	else if(prev_alloc && !next_alloc){
		size+=GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp),PACK(size,0));
		PUT(FTRP(bp),PACK(size,0));
	}
	else if(!prev_alloc && next_alloc){
		size+=GET_SIZE(FTRP(PREV_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
		PUT(FTRP(bp),PACK(size,0));
		bp=PREV_BLKP(bp);
	}
	else {
		size+=GET_SIZE(HDRP(NEXT_BLKP(bp)));
		size+=GET_SIZE(FTRP(PREV_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
		PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));
		bp=PREV_BLKP(bp);
	}
	return bp;
}

void *mm_malloc(size_t size){
	size_t asize;
	size_t extendsize;
	char *bp;

	if(size==0)return NULL;

	if(size<=DSIZE)
		asize=2*DSIZE;
	else 
		asize=DSIZE*((size+(DSIZE)+(DSIZE-1))/DSIZE);

	if((bp=find_fit(asize))!=NULL){
		place(bp,asize);
		return bp;
	}

	extendsize=MAX(asize,CHUNKSIZE);
	if((bp=extend_heap(extendsize/WSIZE))==NULL)
		return NULL;
	place(bp,asize);
	return bp;
}

static void *find_fit(size_t asize){
	void *bp;
	for(bp=heap_listp; GET_SIZE(HDRP(bp))>0 ; bp=NEXT_BLKP(bp)){
		if(!GET_ALLOC(HDRP(bp)) && GET_SIZE(HDRP(bp))>=asize)
			return bp;
	}
	return NULL;
}

static void place(void *bp,size_t asize){
	size_t totsize=GET_SIZE(HDRP(bp));
	if(totsize-asize<2*DSIZE){
		PUT(HDRP(bp),PACK(totsize,1));
		PUT(FTRP(bp),PACK(totsize,1));
	}
	else {
		PUT(HDRP(bp),PACK(asize,1));
		PUT(FTRP(bp),PACK(asize,1));
		
		PUT(HDRP(NEXT_BLKP(bp)),PACK(totsize-asize,0));
		PUT(FTRP(NEXT_BLKP(bp)),PACK(totsize-asize,0));
	}
}


void printmemlist(){
	void *bp;
	
	printf("heap_listp:ox%x\n",heap_listp);
	for(bp=heap_listp;GET_SIZE(HDRP(bp))>0; bp=NEXT_BLKP(bp)){
		printf("bp:ox%x\nstart:%x\tlength:%d\ttag:%d\n\n",bp,HDRP(bp),GET_SIZE(HDRP(bp)),GET_ALLOC(HDRP(bp)));
	}
}

int main()
{
	int *p;
	int n;
	char s[10];
	mem_init();
	if(mm_init()<0){
		printf("init error\n");
		exit(0);
	}
	while(scanf("%s",s)==1){
		if('m'==s[0]){
			printf("input memory size you need \n");
			scanf("%d",&n);
			p=(int *)mm_malloc(n);
			printmemlist();
		}
		else if('f'==s[0]){
			printf("input which memory block you want free\n");
			scanf("%x",&p);
			mm_free(p);
			printmemlist();
		}
		else if('p'==s[0]){
			printmemlist();
		}
		else if('e'==s[0])break;
	}

	return 0;
}

