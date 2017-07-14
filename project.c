#include<stdio.h>
#include<conio.h>
#include<stdlib.h>
#include<stdbool.h>
#include<string.h>
#define max(a,b)  (((a)>(b))?(a):(b))

struct jobqueue{
	int job_id;
	int num_of_procs;
	int execution_time;
	int pending_time;
	int shadow_time;
	int status;
	struct jobqueue *ptr;
}*temp,*f_jobq=NULL,*r_jobq=NULL,*f_fgq=NULL,*r_fgq=NULL,*f_bgq=NULL,*r_bgq=NULL,*joblist=NULL,*temp1,*reserved_job=NULL,*unionlist=NULL,*f_union1,*f_union2,*temp2;


int job_count=0,jobq_count=0, bgq_count=0,fgq_count=0,union_count=0,pid_fg,pid_bg;
char s,J,p;
int vm[5][3];
int i=0,j=0,k=0,fgidle=5,bgidle=5,extra_fg=0,future_fg=0,t=0,f=0;
char name[FILENAME_MAX];
FILE *fptr;

void enqueue_jobq(int j_id,int no_procs,int ex_time);
void delete_jobq(int j_id);
void display_jobq();
void sort_jobq();

void addnode_joblist(int j_id,int no_procs,int ex_time);
void display_joblist();

void enqueue_bgq(int j_id,int no_procs,int ex_time,int pend_time);
void delete_bgq(int j_id);
void display_bgq();
void sort_bgq();

void enqueue_fgq(int j_id,int no_procs,int ex_time,int pend_time);
void delete_fgq(int j_id);
void display_fgq();
void sort_fgq();

void union_list();
bool isPresent (int j_id);
void delete_unionlist(int j_id);
void union_sort(char s);
void display_unionlist();

bool dispatch_fg(int j_id,int no_procs);
void delete_fg(int j_id,int no_procs);
void update_fg();

bool dispatch_bg(int j_id);
void delete_bg(int j_id);
void update_bg();

bool insert_rsrv(int j_id,int no_procs);
void delete_rsrv(int j_id);
void display_vm();
int deploy(int j_id,int p_id);
int job_pid_bg(int j_id);

bool status();

void main(){                 
	int j_id,no_procs,ex_time,n,a,sum_of_extime=0;
	FILE *ptr;
	ptr=fopen("jobs.txt","r+");
	do{
		fscanf(ptr,"%d\t%d\t%d",&j_id,&no_procs,&ex_time);
		enqueue_jobq(j_id,no_procs,ex_time);
		addnode_joblist(j_id,no_procs,ex_time);
	}while(!feof(ptr));
	fclose(ptr);
	for( i=0;i<=5;i++){
		for(j=0;j<3;j++){
			vm[i][j]=0;
		}
	}
	vm[4][0]=-1                      ;
	do{
		/*insertion at time 0 */
		if(t==0){	
			fptr=fopen("time-0.txt","w");
			union_list();
			union_sort('f');
			temp=unionlist;
			while((temp!=NULL)&&(fgidle>0)&&(bgidle>0)){
				while(fgidle>0){				/*inserting into foreground */
					if(temp->num_of_procs <= fgidle){
						if(dispatch_fg(temp->job_id,temp->num_of_procs)){
							enqueue_fgq(temp->job_id,temp->num_of_procs,temp->execution_time,temp->pending_time);
							delete_jobq(temp->job_id);
							delete_unionlist(temp->job_id);
							temp=temp=temp->ptr;
						}else{
							printf("\n job is not successfully inserted into foreground\n");
						}
					}else{
						temp=temp->ptr;
					}		
				}
				/*inserting into background */
				union_sort('b');
				temp=unionlist;
				for(i=0;i<5;i++){
					if(vm[i][0]==-1)
						bgidle--;
				}
				while(bgidle>0){
					if(temp->num_of_procs==1){
						if(dispatch_bg(temp->job_id)){
							enqueue_bgq(temp->job_id,temp->num_of_procs,temp->execution_time,temp->pending_time);
							delete_jobq(temp->job_id);
							delete_unionlist(temp->job_id);
							temp=temp->ptr;
						}else{
							printf("\n job is not successfully inserted into background\n");
						}	
					}else{
					temp=temp->ptr;
					}
				}
				/*reservation */
				temp=unionlist;				
				future_fg=fgidle;
				temp1=f_fgq;
				while(temp1!=NULL){
					future_fg=future_fg+temp1->num_of_procs;
					if((future_fg>=temp->num_of_procs)&&(temp->num_of_procs>3)){
						insert_rsrv(temp->job_id,temp->num_of_procs);
						temp->shadow_time=max(temp->shadow_time,temp1->execution_time);
						extra_fg=future_fg - temp->num_of_procs;
						reserved_job=temp;break;
					}
					temp1=temp1->ptr;
				}
			}
		}
		/* insertion after time 0  */
		if(t!=0){	 
			snprintf(name, sizeof(name), "time_%d.txt", t);
   			fopen_s(&fptr, name, "w");
			if(((t<=5)&&(t%2==0))||(t>=6)) {
				update_bg();
			}
			update_fg();
			if((f_jobq!=NULL)&&(fgidle>0)){
				/*deploying a job from background to foreground */
				unionlist=NULL;
				union_list();
				union_sort('f');
				temp=unionlist;
				while((temp!=NULL)&&(fgidle>0)){	
				if(temp->num_of_procs==1){
					pid_bg=job_pid_bg(temp->job_id);
					pid_fg=deploy(temp->job_id,pid_bg);
					if(pid_fg!=pid_bg){
						temp->pending_time=temp->execution_time;
						//resetting pending time in job list
						temp1=joblist;
						while(temp1!=NULL){
							if(temp->job_id==temp1->job_id){
								temp1->pending_time=temp1->execution_time;
							}
							temp1=temp1->ptr;
						}
					}
					enqueue_fgq(temp->job_id,temp->num_of_procs,temp->execution_time,temp->pending_time);
					delete_bg(temp->job_id);
					bgidle++;
					delete_bgq(temp->job_id);
					delete_unionlist(temp->job_id);
					temp=temp->ptr;
				}else{
					temp=temp->ptr;
				}
				}
				if(fgidle>=reserved_job->num_of_procs){/*insertion of reserved job into foreground */
						dispatch_fg(reserved_job->job_id,reserved_job->num_of_procs);
						enqueue_fgq(reserved_job->job_id,reserved_job->num_of_procs,reserved_job->execution_time,reserved_job->pending_time);
						delete_jobq(reserved_job->job_id);
						delete_unionlist(reserved_job->job_id);
						delete_rsrv(temp->job_id);
						reserved_job=NULL;
						
				}else{
					/* backfilling runnable jobs into foreground */
					if((f_jobq!=NULL)&&(fgidle>0)){		
					union_sort('f');
					temp=unionlist;
					while(temp!=NULL){
						if(temp->num_of_procs<fgidle){
							if((temp->execution_time)<reserved_job->shadow_time){
								if(dispatch_fg(temp->job_id,temp->num_of_procs)){
									delete_jobq(temp->job_id);
									delete_unionlist(temp->job_id);
									extra_fg=extra_fg - temp->num_of_procs;
								}
							}
						}
						temp=temp->ptr;
					}
				}
			}
			}
			/*reservation for another job if reserved job is inserted into foreground */
			if((reserved_job==NULL)&&(f_jobq!=NULL)){ 
				temp=unionlist;
				future_fg=fgidle;
				temp1=f_fgq;
				while(temp1!=NULL){
					future_fg=future_fg+temp1->num_of_procs;
					if((future_fg>=temp->num_of_procs)&&(temp->num_of_procs>3)){
						insert_rsrv(temp->job_id,temp->num_of_procs);
						temp->shadow_time=max(temp->shadow_time,temp1->execution_time);
						extra_fg=future_fg - temp->num_of_procs;
						reserved_job=temp;
						break;
					}
					temp1=temp1->ptr;
				}
			}
			/*inserting jobs into background */
			if((f_jobq!=NULL)&&(bgidle>0)){	
				sort_jobq();
				temp=f_jobq;
				i=0;
				while((temp!=NULL)&&(bgidle>0)&&(i<job_count)){
					if(temp->num_of_procs==1){
						if(dispatch_bg(temp->job_id)){
							enqueue_bgq(temp->job_id,temp->num_of_procs,temp->execution_time,temp->pending_time);
							delete_jobq(temp->job_id);
							delete_unionlist(temp->job_id);
							temp=temp->ptr;
						}else{
							printf("\n job is not successfully inserted into background\n");
						}	
					}else{
						temp=temp->ptr;
					}
					i++;
				}
			}	
		}
		display_vm();
		fprintf(fptr,"\n the status of Jobs in Job list");
		display_joblist();
		t++;
		fclose(fptr);
	}while(t!=38);
	printf("\n program completed");
	return;
}

void enqueue_jobq(int j_id,int no_procs,int ex_time){	
    if (r_jobq == NULL){
		r_jobq=(struct jobqueue *)malloc(sizeof(struct jobqueue));
    	r_jobq->job_id = j_id;
    	r_jobq->num_of_procs = no_procs;
    	r_jobq->execution_time = ex_time;
    	r_jobq->pending_time = ex_time;
    	r_jobq->shadow_time = 0;
    	r_jobq->status = 0;
    	r_jobq->ptr = NULL;
     	f_jobq = r_jobq;
   	}
	else
	{
		temp=(struct jobqueue *)malloc(sizeof(struct jobqueue));
    	temp->job_id = j_id;
    	temp->num_of_procs = no_procs;
    	temp->execution_time = ex_time;
    	temp->pending_time = ex_time;
    	temp->shadow_time = 0;
    	temp->status = 0;
    	temp->ptr = NULL;	
        r_jobq->ptr = temp;
        r_jobq=temp;
    }
    jobq_count++;
    return;
}

void addnode_joblist(int j_id,int no_procs,int ex_time){	
	temp=(struct jobqueue *)malloc(sizeof(struct jobqueue));
    temp->job_id = j_id;
    temp->num_of_procs = no_procs;
    temp->execution_time = ex_time; 
    temp->pending_time = ex_time;
    temp->shadow_time = 0;
    temp->status = 0;
    temp->ptr = NULL;
	if (joblist == NULL){
     	joblist=temp;
   	 }else{
		temp1=joblist;
		while(temp1->ptr !=NULL) {
			temp1=temp1->ptr;
		}	       
		temp1->ptr=temp;
    }
    job_count++;
    return;
}
void enqueue_fgq(int j_id,int no_procs,int ex_time,int pend_time){	
    if (f_fgq==NULL){
		temp=(struct jobqueue *)malloc(sizeof(struct jobqueue));
    	temp->job_id = j_id;
    	temp->num_of_procs = no_procs;
    	temp->execution_time = ex_time;
    	temp->pending_time = pend_time;
    	temp->shadow_time = 0;
    	temp->status = 0;
    	temp->ptr = NULL;
     	f_fgq = temp;
     	r_fgq=temp;
   	}
	else
	{
		temp=(struct jobqueue *)malloc(sizeof(struct jobqueue));
    	temp->job_id = j_id;
    	temp->num_of_procs = no_procs;
    	temp->execution_time = ex_time;
    	temp->pending_time = pend_time;
    	temp->shadow_time = 0;
    	temp->status = 0;
    	temp->ptr = NULL;	
        r_fgq->ptr=temp;
        r_fgq=temp;
    }
    fgq_count++;
    return;
}
void enqueue_bgq(int j_id,int no_procs,int ex_time,int pend_time){	
    if (f_bgq==NULL){
		temp=(struct jobqueue *)malloc(sizeof(struct jobqueue));
    	temp->job_id = j_id;
    	temp->num_of_procs = no_procs;
    	temp->execution_time = ex_time;
    	temp->pending_time = pend_time;
    	temp->shadow_time = 0;
    	temp->status = 0;
    	temp->ptr = NULL;
     	f_bgq = temp;
     	r_bgq = temp;
   	}
	else
	{
		temp=(struct jobqueue *)malloc(sizeof(struct jobqueue));
    	temp->job_id = j_id;
    	temp->num_of_procs = no_procs;
    	temp->execution_time = ex_time;
    	temp->pending_time = pend_time;
    	temp->shadow_time = 0;
    	temp->status = 0;
    	temp->ptr = NULL;	
        r_bgq->ptr = temp;
        r_bgq=temp;
    }
    bgq_count++;
    return;
}

void delete_jobq(int j_id)
{
	temp = f_jobq;
	if ((f_jobq == NULL) && (r_jobq == NULL))
    {
        printf("Queue is empty");
        return;
    }else{
    	if((f_jobq->job_id==j_id) ){
			temp1=f_jobq;
			f_jobq=f_jobq->ptr;
			if(f_jobq==NULL){
		 		r_fgq=NULL;
			}
		}else{
     		while((temp!=NULL)&&(temp->job_id != j_id)){
     			temp1=temp;
     			temp=temp->ptr;
		 	}
		 	if((temp->job_id == j_id)){
		 		temp1->ptr=temp->ptr;
		 	}
		 	if(temp1->ptr==NULL){
		 		r_fgq=temp1;
			}
		}
		job_count--;
	}	
	return;
}

void delete_bgq(int j_id)
{
	temp = f_bgq;
	if ((f_bgq == NULL) && (r_bgq == NULL))
    {
        printf("Queue is empty");
        return;
    }else{
    	if((f_bgq->job_id==j_id)){
			temp1=f_bgq;
			f_bgq=f_bgq->ptr;
			if(f_bgq==NULL){
		 		r_bgq=NULL;
			}
		}else{
     		while((temp!=NULL)&&(temp->job_id != j_id)){
     			temp1=temp;
     			temp=temp->ptr;
		 	}
		 	if((temp->job_id == j_id)){
		 		temp1->ptr=temp->ptr;
		 	}
		 	if(temp1->ptr==NULL){
		 		r_fgq=temp1;
			}
		}
	}	
	bgq_count--;
}

void delete_fgq(int j_id)
{
	temp = f_fgq;
	if ((f_fgq == NULL) && (r_fgq == NULL))
    {
        printf("Queue is empty");
        return;
    }else{
    	if((f_fgq->job_id==j_id)){
			temp1=f_fgq;
			f_fgq=f_fgq->ptr;
			if(f_fgq==NULL){
		 		r_fgq=NULL;
			}
		}else{
     		while((temp!=NULL)&&(temp->job_id != j_id)){
     			temp1=temp;
     			temp=temp->ptr;
		 	}
		 	if((temp->job_id == j_id)){
				temp1->ptr=temp->ptr;
		 	}
		 	if(temp1->ptr==NULL){
		 		r_fgq=temp1;
			}
		}
	}	
	fgq_count--;
}
	
void display_jobq(){
	temp = f_jobq;
    if ((f_jobq == NULL) && (r_jobq == NULL))
    {
        printf("\nQueue is empty");
        return;
    }
    while(temp!=NULL)
    {
        printf("\n%d\t%d\t%d\n",temp->job_id,temp->num_of_procs,temp->execution_time );
        temp = temp->ptr;
    }
}
void display_joblist(){
	char str1[14]="incomplete";
	char str2[14]="complete";
	char str3[14];
	temp = joblist;
 	fprintf(fptr,"\nJob ID\tNo. of Processes\tExe. Time\tPend. Time\tStatus");
    if (joblist == NULL)
    {
        fprintf(fptr,"\nQueue is empty");
        return;
    }
    while(temp != NULL)
    {
    	if(temp->status==0){
    		strcpy(str3,str1);
		}else{
			strcpy(str3,str2);
		}
        fprintf(fptr,"\n%d\t\t%d\t\t%d\t\t%d\t\t%s\n",temp->job_id,temp->num_of_procs,temp->execution_time,temp->pending_time,str3 );
        temp = temp->ptr;
    }
}
void display_fgq(){
	temp = f_fgq;
    if ((f_fgq == NULL) && (r_fgq == NULL))
    {
        printf("\nQueue is empty");
        return;
    }
    while(temp!=NULL)
    {
        printf("\n%d\t%d\t%d\t%d\n",temp->job_id,temp->num_of_procs,temp->execution_time,temp->pending_time );
        temp = temp->ptr;
    }
}
void display_bgq(){
	temp = f_bgq;
    if ((f_bgq == NULL) && (r_bgq == NULL))
    {
        printf("\nQueue is empty");
        return;
    }
    while (temp != NULL)
    {
    	printf("\n%d\t%d\t%d\t%d\n",temp->job_id,temp->num_of_procs,temp->execution_time,temp->pending_time );
        temp = temp->ptr;
	}
}
void union_list(){
	f_union1 = f_jobq;
	f_union2 = f_bgq;
	while(f_union1!= NULL || f_union2!=NULL)
	{
		while(f_union1!=NULL){
			if(unionlist==NULL){
				temp=(struct jobqueue *)malloc(sizeof(struct jobqueue));
				temp->job_id = f_union1->job_id;
    			temp->num_of_procs = f_union1->num_of_procs;
    			temp->execution_time = f_union1->execution_time;
    			temp->pending_time = f_union1->pending_time;
    			temp->shadow_time = 0;
   				temp->status = 0;
   				temp->ptr = NULL;
   				unionlist=temp;
   				f_union1=f_union1->ptr;
   				union_count++;
			}
			else
			{
				temp1=unionlist;
				while(temp1->ptr!=NULL)
					temp1=temp1->ptr;
				temp=(struct jobqueue *)malloc(sizeof(struct jobqueue));
				temp->job_id = f_union1->job_id;
    			temp->num_of_procs = f_union1->num_of_procs;
    			temp->execution_time = f_union1->execution_time;
    			temp->pending_time = f_union1->pending_time;
    			temp->shadow_time = 0;
   				temp->status = 0;
   				temp->ptr = NULL;
   				temp1->ptr=temp;
   				f_union1=f_union1->ptr;
   				union_count++;

   			}
		}
		while(f_union2!=NULL){
			if(unionlist==NULL){
				temp=(struct jobqueue *)malloc(sizeof(struct jobqueue));
				temp->job_id = f_union2->job_id;
    			temp->num_of_procs = f_union2->num_of_procs;
    			temp->execution_time = f_union2->execution_time;
    			temp->pending_time = f_union2->pending_time;
    			temp->shadow_time = 0;
   				temp->status = 0;
   				temp->ptr = NULL;
   				unionlist=temp;
   				f_union2=f_union2->ptr;
   				union_count++;
			}
			else
			{
				if(isPresent(f_union2->job_id)){
					temp1=unionlist;
					while(temp1->ptr!=NULL)
						temp1=temp1->ptr;
					temp=(struct jobqueue *)malloc(sizeof(struct jobqueue));
					temp->job_id = f_union2->job_id;
    				temp->num_of_procs = f_union2->num_of_procs;
    				temp->execution_time = f_union2->execution_time;
    				temp->pending_time = f_union2->pending_time;
    				temp->shadow_time = 0;
   					temp->status = 0;
   					temp->ptr = NULL;
   					temp1->ptr=temp;
   					f_union2=f_union2->ptr;
   					union_count++;
   				}else{
   					f_union2=f_union2->ptr;
   				}
   			}
		}
	}
}
bool isPresent (int j_id)
{
    struct jobqueue *t = unionlist;
    while (t != NULL)
    {
        if (t->job_id == j_id)
            return false;
        t = t->ptr;
    }
    return true;
}
void display_unionlist(){
	temp = unionlist;
    if (unionlist == NULL)
    {
        printf("List is empty");
        return;
    }
    else{
    	while(temp != NULL){
        	printf("\n%d\t%d\t%d\n",temp->job_id,temp->num_of_procs,temp->execution_time );
        	temp = temp->ptr;
		}
	}
    return;
}
void union_sort(char s){
	if(s=='f'){
		if(unionlist->ptr==NULL){
			return;	
		}else{
			temp1=unionlist;
			while(temp1!=NULL){
				temp2=temp1->ptr;
				while(temp2!=NULL){
					if(temp1->job_id > temp2->job_id){
						int a=temp1->job_id;
						temp1->job_id=temp2->job_id;
						temp2->job_id=a;
						a=temp1->num_of_procs;
						temp1->num_of_procs=temp2->num_of_procs;
						temp2->num_of_procs=a;
						a=temp1->execution_time;
						temp1->execution_time=temp2->execution_time;
						temp2->execution_time=a;
						a=temp1->pending_time;
						temp1->pending_time=temp2->pending_time;
						temp2->pending_time=a;
						a=temp1->shadow_time;
						temp1->shadow_time=temp2->shadow_time;
						temp2->shadow_time=a;
						a=temp1->status;
						temp1->status=temp2->status;
						temp2->status=a;
					}
					temp2=temp2->ptr;
				}
				temp1=temp1->ptr;
			}
		}
	}else if(s=='b'){
		if(unionlist->ptr==NULL){
			return;	
		}else{
			temp1=unionlist;
			while(temp1!=NULL){
				temp2=temp1->ptr;
				while(temp2!=NULL){
					if((temp1->execution_time > temp2->execution_time)&&(temp1->num_of_procs==1)&&(temp2->num_of_procs==1)){
						int a=temp1->job_id;
						temp1->job_id=temp2->job_id;
						temp2->job_id=a;
						a=temp1->num_of_procs;
						temp1->num_of_procs=temp2->num_of_procs;
						temp2->num_of_procs=a;
						a=temp1->execution_time;
						temp1->execution_time=temp2->execution_time;
						temp2->execution_time=a;
						a=temp1->pending_time;
						temp1->pending_time=temp2->pending_time;
						temp2->pending_time=a;
						a=temp1->shadow_time;
						temp1->shadow_time=temp2->shadow_time;
						temp2->shadow_time=a;
						a=temp1->status;
						temp1->status=temp2->status;
						temp2->status=a;
					}
					temp2=temp2->ptr;
				}
				temp1=temp1->ptr;
			}
		}
	}
	return;
}
void delete_unionlist(int j_id)
{
	temp = unionlist;
	if (unionlist == NULL)
    {
        printf("Queue is empty");
        return;
    }else{
    	if((unionlist->job_id==j_id)){
			temp1=unionlist;
			unionlist=unionlist->ptr;
		}else{
     		while((temp!=NULL)&&(temp->job_id != j_id)){
     			temp1=temp;
     			temp=temp->ptr;
		 	}
		 	if((temp->job_id == j_id)){
		 		temp1->ptr=temp->ptr;
		 	}
		}
	}	
	return;
}
bool dispatch_fg(int j_id,int no_procs){
	for(i=0;i<5;i++){
		if((vm[i][1]==0)&&(no_procs>0)){
			vm[i][1]=j_id;
			no_procs--;	
			fgidle--;	
		}
	}
	return true;
}
bool dispatch_bg(int j_id){
	for(i=0;i<5;i++){
		if(vm[i][0]==0){
			vm[i][0]=j_id;	
			bgidle--;
			return true;	
		}
	}
	return false;
}
void sort_fgq(){
	if(f_fgq->ptr==NULL){
			return;	
	}else{
		temp1=f_fgq;
		while(temp1!=NULL){
			temp2=temp1->ptr;
			while(temp2!=NULL){
				if(temp1->execution_time > temp2->execution_time){
					int a=temp1->job_id;
					temp1->job_id=temp2->job_id;
					temp2->job_id=a;
					a=temp1->num_of_procs;
					temp1->num_of_procs=temp2->num_of_procs;
					temp2->num_of_procs=a;
					a=temp1->execution_time;
					temp1->execution_time=temp2->execution_time;
					temp2->execution_time=a;
					a=temp1->pending_time;
					temp1->pending_time=temp2->pending_time;
					temp2->pending_time=a;
					a=temp1->shadow_time;
					temp1->shadow_time=temp2->shadow_time;
					temp2->shadow_time=a;
					a=temp1->status;
					temp1->status=temp2->status;
					temp2->status=a;
				}
				temp2=temp2->ptr;
			}
			temp1=temp1->ptr;
		}
	}
	return;
}
bool insert_rsrv(int j_id,int no_procs){
	for(i=0;i<no_procs;i++){
		vm[i][2]=j_id;
	}
	return true;
}
void delete_fg(int j_id,int no_procs){
	for(i=0;i<5;i++){
		if((vm[i][1]==j_id)&&(no_procs>0)){
			vm[i][1]=0;
		}
    }
}
void delete_bg(int j_id){
	for(i=0;i<5;i++){
		if(vm[i][0]==j_id){
			vm[i][0]=0;
		}
	}
}
bool status(){
	int flag=0;
	for(i=0;i<5;i++){
		for(j=0;j<3;j++){
			if(vm[i][j]==0 || vm[i][j]==-1)
				flag++;
		}
	}
	if(flag==15)
		return false;
	else
		return true;
}
void update_fg(){
	/*deletion of completed jobs in foreground */
	temp=f_fgq;       
	while(temp!=NULL){
		temp->pending_time=temp->pending_time-1;
		//update the pending time in job list
		temp1=joblist;
		while(temp1!=NULL){
			if(temp->job_id==temp1->job_id)
			{
				temp1->pending_time=temp->pending_time;
			}
			temp1=temp1->ptr;
		}
		if(temp->pending_time==0){
			fprintf(fptr,"\n at time %d job j%d is completed",t,temp->job_id);
		//update the status in job list
			temp1=joblist;
			while(temp1!=NULL){
				if(temp->job_id==temp1->job_id)
				{
					temp1->status=1;
				}
				temp1=temp1->ptr;
			}
			
			delete_fgq(temp->job_id);
			delete_fg(temp->job_id,temp->num_of_procs);
			fgidle=fgidle+temp->num_of_procs;
		}
		temp=temp->ptr;
	}
}
void update_bg(){
	/*deletion of completed jobs in background */
	temp=f_bgq;
	while(temp!=NULL){
		temp->pending_time=temp->pending_time-1;
		//update the pending time in job list
		temp1=joblist;
		while(temp1!=NULL){
			if(temp1->job_id==temp->job_id)
			{
				temp1->pending_time=temp->pending_time;
			}
			temp1=temp1->ptr;
		}		
		if(temp->pending_time==0){
			fprintf(fptr,"\n at time %d job j%d is completed",t,temp->job_id);
			//update the status in job list
			temp1=joblist;
			while(temp1!=NULL){
				if(temp1->job_id==temp->job_id)
				{
					temp1->status=1;
				}
				temp1=temp1->ptr;
			}
			delete_bgq(temp->job_id);
			delete_bg(temp->job_id);
			bgidle=bgidle+temp->num_of_procs;
		}	
		temp=temp->ptr;
	}
}
int job_pid_bg(int j_id){
	for(i=0;i<5;i++){
		if(vm[i][0]==j_id){
			return i;
		}
	}
}
int deploy(int j_id,int p_id){
	if(vm[p_id][1]==0){
		vm[p_id][1]=j_id;
		fgidle--;
		return p_id;
	}else{
		for(i=0;i<5;i++){
			if(vm[i][1]==0){
				vm[i][1]=j_id;
				fgidle--;
				return i;
			}
		}
	}
}
void sort_bgq(){
	if(f_bgq->ptr==NULL){
			return;	
	}else{
		temp1=f_bgq;
		while(temp1!=NULL){
			temp2=temp1->ptr;
			while(temp2!=NULL){
				if(temp1->job_id > temp2->job_id){
					int a=temp1->job_id;
					temp1->job_id=temp2->job_id;
					temp2->job_id=a;
					a=temp1->num_of_procs;
					temp1->num_of_procs=temp2->num_of_procs;
					temp2->num_of_procs=a;
					a=temp1->execution_time;
					temp1->execution_time=temp2->execution_time;
					temp2->execution_time=a;
					a=temp1->pending_time;
					temp1->pending_time=temp2->pending_time;
					temp2->pending_time=a;
					a=temp1->shadow_time;
					temp1->shadow_time=temp2->shadow_time;
					temp2->shadow_time=a;
					a=temp1->status;
					temp1->status=temp2->status;
					temp2->status=a;
				}
				temp2=temp2->ptr;
			}
			temp1=temp1->ptr;
		}
	}
	return;
}
void display_vm(){
	k=0;
	fprintf(fptr,"\n at time-%d",t);
	fprintf(fptr,"\n\tbackground\tforeground\treservation");
	for(i=4;i>=0;i--){
		fprintf(fptr,"\n");
		fprintf(fptr,"P%d",(k+1));
		for(j=0;j<3;j++){
			if(vm[i][j]==-1){
				fprintf(fptr,"\tN\t");
			}else if(vm[i][j]==0){
					fprintf(fptr,"\t0\t");
				}else{
					fprintf(fptr,"\tJ%d\t",vm[i][j]);
			}
		}
		k++;
	}
}
void sort_jobq(){
	if(f_jobq->ptr==NULL){
			return;	
	}else{
		temp1=f_jobq;
		while(temp1!=NULL){
			temp2=temp1->ptr;
			while(temp2!=NULL){
				if((temp1->execution_time > temp2->execution_time)&&(temp1->num_of_procs==1)&&(temp2->num_of_procs==1)){
					int a=temp1->job_id;
					temp1->job_id=temp2->job_id;
					temp2->job_id=a;
					a=temp1->num_of_procs;
					temp1->num_of_procs=temp2->num_of_procs;
					temp2->num_of_procs=a;
					a=temp1->execution_time;
					temp1->execution_time=temp2->execution_time;
					temp2->execution_time=a;
					a=temp1->pending_time;
					temp1->pending_time=temp2->pending_time;
					temp2->pending_time=a;
					a=temp1->shadow_time;
					temp1->shadow_time=temp2->shadow_time;
					temp2->shadow_time=a;
					a=temp1->status;
					temp1->status=temp2->status;
					temp2->status=a;
				}
				temp2=temp2->ptr;
			}
			temp1=temp1->ptr;
		}
	}
	return;
}
void delete_rsrv(int j_id){
	for(i=0;i<5;i++){
		vm[i][2]=0;
	}
}
