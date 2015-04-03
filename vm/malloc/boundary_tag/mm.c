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
#define GET_TAG(p) (GET(p) & 2)//if pre block if empty tag==0 ; else tag==1
#define SET_TAG(p,t) (GET(p)=(t==0 ? (GET(p)&(~2)) : (GET(p)|2))) 

#define HDRP(bp) ((char *)(bp) -WSIZE)
#define FTRP(bp) ((char *)(bp)+GET_SIZE(HDRP(bp))-DSIZE)

#define NEXT_BLKP(bp) ((char *)(bp) + GET_SIZE(HDRP(bp)))
#define PREV_BLKP(bp) ((char *)(bp) - GET_SIZE((char *)(bp)-DSIZE))

static char *heap_listp=0;
static char last_block_alloc=1;

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
	heap_listp+=(WSIZE)*2;

	if(extend_heap(CHUNKSIZE/WSIZE) == NULL)
		return -1;
	return 0;
}


static void *extend_heap(size_t words){//////////////
	char *bp;
	size_t size;
	size=(words%2) ? (words+1)*WSIZE : words*WSIZE;
	if((long)(bp=mem_sbrk(size))==-1)
		return NULL;

	PUT(HDRP(bp),PACK(size,0));
	PUT(FTRP(bp),PACK(size,0));
	SET_TAG(HDRP(bp),last_block_alloc);//////////
	
	PUT(HDRP(NEXT_BLKP(bp)),PACK(0,1));
	SET_TAG(HDRP(NEXT_BLKP(bp)),0);//////////

	last_block_alloc=0;
	return coalesce(bp);
}

void mm_free(void *bp){
	size_t size=GET_SIZE(HDRP(bp));
	void *nextp=HDRP(NEXT_BLKP(bp));

	int tag=GET_TAG(HDRP(bp));
	PUT(HDRP(bp),PACK(size,0+tag*2));
	PUT(FTRP(bp),PACK(size,0+tag*2));
	SET_TAG(HDRP(bp),tag);

	if(GET_SIZE(nextp)==0)last_block_alloc=0;

	//GET(nextp)=GET(nextp) &(~2);
	SET_TAG((nextp),0);
	coalesce(bp);
}

static void *coalesce(void *bp){
	size_t prev_alloc=GET_TAG(HDRP(bp));
	size_t next_alloc=GET_ALLOC(HDRP( NEXT_BLKP(bp)));
	size_t size=GET_SIZE(HDRP(bp));
	int tag;

	if(prev_alloc && next_alloc)
		return bp;
	else if(prev_alloc && !next_alloc){
		tag=GET_TAG(HDRP(bp));
		size+=GET_SIZE(HDRP(NEXT_BLKP(bp)));
		PUT(HDRP(bp),PACK(size,0));
		PUT(FTRP(bp),PACK(size,0));
	}
	else if(!prev_alloc && next_alloc){
		size+=GET_SIZE(FTRP(PREV_BLKP(bp)));
		tag=GET_TAG(HDRP(PREV_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
		PUT(FTRP(bp),PACK(size,0));
		bp=PREV_BLKP(bp);
	}
	else {
		tag=GET_TAG(HDRP(PREV_BLKP(bp)));
		size+=GET_SIZE(HDRP(NEXT_BLKP(bp)));
		size+=GET_SIZE(FTRP(PREV_BLKP(bp)));
		PUT(HDRP(PREV_BLKP(bp)),PACK(size,0));
		PUT(FTRP(NEXT_BLKP(bp)),PACK(size,0));
		bp=PREV_BLKP(bp);
	}
	SET_TAG(HDRP(bp),tag);
	return bp;
}

void *mm_malloc(size_t size){
	size_t asize;
	size_t extendsize;
	char *bp;

	if(size==0)return NULL;

	if(size<=WSIZE)
		asize=DSIZE;
	else 
		asize=DSIZE*((size-WSIZE+(DSIZE-1))/DSIZE+1);

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
	int tag=GET_TAG(HDRP(bp));
	if(totsize-asize<DSIZE){
		if(GET_SIZE(HDRP(NEXT_BLKP(bp)))==0)
			last_block_alloc=1;
		PUT(HDRP(bp),PACK(totsize,1));
		SET_TAG(HDRP(NEXT_BLKP(bp)),1);///////////////
	}
	else {
		PUT(HDRP(bp),PACK(asize,1));
		PUT(FTRP(bp),PACK(asize,1));
		
		PUT(HDRP(NEXT_BLKP(bp)),PACK(totsize-asize,0));
		PUT(FTRP(NEXT_BLKP(bp)),PACK(totsize-asize,0));

		SET_TAG(HDRP(NEXT_BLKP(bp)),1);/////////
	}
	SET_TAG(HDRP(bp),tag);
}


void printmemlist(){
	void *bp;
	
	printf("heap_listp:ox%x\n",heap_listp);
	for(bp=heap_listp;GET_SIZE(HDRP(bp))>0; bp=NEXT_BLKP(bp)){
		printf("bp:ox%x\nstart:%x\tlength:%d\talloc:%d\ttag:%d\n\n",bp,HDRP(bp),GET_SIZE(HDRP(bp)),GET_TAG(HDRP(bp)),GET_ALLOC(HDRP(bp)));
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

