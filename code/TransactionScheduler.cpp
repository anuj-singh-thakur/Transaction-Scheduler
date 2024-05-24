/*
* Implementation: In this project, our goal is to manage transactions across
*	          multiple servers effectively while optimizing resource utilization to ensure 
*		  smooth customer interactions.
*
* 
*/


#include<bits/stdc++.h>
#define endl "\n"
#define BURST_TIME 3
using namespace std;

/*  Request Node:
* 
*   int req_id --> unique id for each request
*   int transaction_type --> service type required to execute each request
*   int resource_required --> no of resource required
*   int worker_id;  --> to know which worker executed this request
*   time_t start_time,end_time,burst_time --> start, end and burst time of a request  
*
*/
typedef struct{
	int req_id;
	int transaction_type;
	int resource_required;
	int worker_id;
	time_t start_time,end_time,burst_time;
}Request; 

/*  Worker Node:
*
*       int worker_id;  --> unique worker id
*	int priority_level --> priority level of worker node
*	int total_resource; --> total resource assigned to each worker node
*	int available_resource; --> used during execution of worker node.
*	pthread_mutex_t mtx;  -->  mutex to access worker node synchronously 
*
*/

typedef struct{
	int worker_id;
	int priority_level;
	int total_resource;
	int available_resource;
	pthread_mutex_t mtx;
}WorkerNode;

/*  Service Node:
*
*	int service_id;  --> unique server id
*	vector<WorkerNode> WorkerNodeVector; --> vector of worker node for each service
*	queue<Request> service_queue; --> queue for each service which stores the incoming request for this service
*	int max_resource; --> max resource among all the worker node used for rejecting the request if it has more resource requirement.
*	pthread_mutex_t mtx;  --> mutex to access service node synchronously 
*	bool stop_service_flag;  --> flag to stop service thread at the end of all the request processed by main thread.
*
*/
typedef struct{
	int service_id;
	vector<WorkerNode> WorkerNodeVector;
	queue<Request> service_queue;
	int max_resource;
	pthread_mutex_t mtx;
	bool stop_service_flag;
}Service;

/*  Execution Node: --> Store Global Information 
 *	
 *      vector<Request*> executed_request ---> used to store request in which order it's processed.
 *	pthread_mutex_t mtx; ---> mutex to access Execution node synchronously 
 *	int request_rejected;  --> used to store rejected count 
 *	int request_forced_wait; --> used to store forced request count 
 *	int request_blocked;     --> used to store blocked request count 
 *
 */
typedef struct{
	vector<Request*> executed_request;
	pthread_mutex_t mtx;
	int request_rejected;
	int request_forced_wait;
	int request_blocked;
}Execution;

/*--------Global Variable which accessible in all service thread, worker thread and main thread --------*/
Execution execution;


bool comparator(const WorkerNode& wt1, const WorkerNode& wt2) {
    // Custom comparison function to compare worker threads based on priority level
    return wt1.priority_level > wt2.priority_level;
}

/*
*  worker function: sleep this function based on the burst time of a request and then release the resource acquired by given request;
*			       and update the request in the global execution node.
*
*  @param: WorkerNode *workerNode  ---> worker node pointer 
*  @param: Request *request		  ---> Request node pointer
*
*/

void workerFunction(WorkerNode *workerNode,Request *request){
	
	sleep(request->burst_time);
	request->end_time = time(0);
	pthread_mutex_lock(&workerNode->mtx);
	request->worker_id = workerNode->worker_id;
	workerNode->available_resource += request->resource_required;	
	pthread_mutex_unlock(&workerNode->mtx);
	//printf("\nReq_id : %d\n",request->req_id);
	//printf("\nst : %ld bt : %ld et: %ld\n",request->start_time,request->burst_time,request->end_time);
	
	pthread_mutex_lock(&execution.mtx);
	execution.executed_request.push_back(request);	
	pthread_mutex_unlock(&execution.mtx);
}


/*
*  service function: handle all the requests available in the queue and accordingly assign the available worker node based on the priority of 
*					worker node. ( once we select a worker node based on the resource requirement then we execute another thread which processed this request.)	
*
*  @param: Service *service ---> service pointer which contains all the necessary information.
*
*/

void serviceFunction(Service *service){
	int max_resource = 0;
	int request_rejected=0;
	unordered_set<int> request_forced_wait;
	unordered_set<int> request_blocked;	
	
	pthread_mutex_lock(&service->mtx);
	max_resource = service->max_resource;
	//int id = service->service_id;
	vector<WorkerNode> &WorkerNodeVector = service->WorkerNodeVector;		
	pthread_mutex_unlock(&service->mtx);
	
	vector<thread> threads;
	
	while(true){
		/*
		* Reading all the flag variables from the service node. 
		*
		*/
		pthread_mutex_lock(&service->mtx);
		bool exit_flag= service->stop_service_flag;
		bool empty_flag = service->service_queue.empty();
		//int size = service->service_queue.size();
		
		Request request;
		if(!empty_flag){
			request = service->service_queue.front();
			service->service_queue.pop();
		}
		pthread_mutex_unlock(&service->mtx);
		
		if(!empty_flag && request.req_id!=0){
			if(request.resource_required> max_resource){
				request_rejected+=1;
				//printf("\nserviceId : %d Req: %d rejected\n",id,request.req_id);
		
			}else{
				bool request_handled = false;
				bool isWorkerThreadAvailable = false;
				for(auto& workerNode : WorkerNodeVector){
					pthread_mutex_lock(&workerNode.mtx);
					if(workerNode.available_resource == workerNode.total_resource){
						isWorkerThreadAvailable = true;
					}
					/*
					* checking worker thread one by one to full-fill current request. 
					*/
					if(workerNode.available_resource >= request.resource_required){
						//printf("\nResource satisfied... %d  %d\n",request.req_id,request.resource_required);
						workerNode.available_resource -= request.resource_required;
						request_handled = true;
					}
					pthread_mutex_unlock(&workerNode.mtx);
					if(request_handled){
						Request *req_ptr = (Request*)malloc(sizeof(Request));
						if(req_ptr!=NULL){
							req_ptr->req_id = request.req_id;
							req_ptr->transaction_type = request.transaction_type;
							req_ptr->resource_required = request.resource_required;
							req_ptr->start_time = request.start_time;
							req_ptr->burst_time = request.burst_time;
							req_ptr->end_time = request.end_time;
						}
						/*
						*  Running worker thread to fulfill the current request. 
						* 
 						*/
						thread t(workerFunction,&workerNode,req_ptr);
						
						threads.push_back(move(t));
						break;
					}
				}
				if(!request_handled){
					if(isWorkerThreadAvailable){
						request_forced_wait.insert(request.req_id);
					}else{
						request_blocked.insert(request.req_id);
					}
					
					/*
					*  blocked request again pushing back into the queue.
					*/
					pthread_mutex_lock(&service->mtx);
					service->service_queue.push(request);
					pthread_mutex_unlock(&service->mtx);
				}	
			}
		}
		/*
		* Exit the current thread when exit_flag is on and the queue is empty.
		*   
		*/
		if(exit_flag && empty_flag){
			
			break;
		}
	}
	/*
	*  wait until all the thread started by this service finishes their execution.
	*/
	for(auto& t: threads){
		t.join();
	}
	/* 
	*  updating all the local variable counts in the global variable.
	*
	*/
	pthread_mutex_lock(&execution.mtx);
	execution.request_rejected += request_rejected;
	execution.request_forced_wait += request_forced_wait.size();
	execution.request_blocked += request_blocked.size();	
	pthread_mutex_unlock(&execution.mtx);
	
}

int main(){
	int n=0;
	cout<<"Enter no of services: ";
	cin>>n;
	Service service[n];
	
	for(int i=0;i<n;i++){
		int m=0;
		cout<<"Enter no of worker node: ";
		cin>>m;
		cout<<"Enter priority_level and resource\n";
		
		/*
		* Initializing each Service based on input value m.
		*/
		service[i].service_id = i+1;
		service[i].WorkerNodeVector.resize(m);
		pthread_mutex_init(&service[i].mtx, NULL);
		service[i].stop_service_flag=false;
		int max_resource=0;
		for(int j=0;j<m;j++){
			int priority_level=0,resource=0;
			cin>>priority_level>>resource;
			max_resource = max(max_resource,resource);

			/*
			*  Initializing worker Node with the given input.
			*/
			service[i].WorkerNodeVector[j].worker_id = j+1;
			service[i].WorkerNodeVector[j].priority_level = priority_level;
			service[i].WorkerNodeVector[j].total_resource = resource;
			service[i].WorkerNodeVector[j].available_resource = resource;
			
			pthread_mutex_init(&service[i].WorkerNodeVector[j].mtx,NULL);
		}
		service[i].max_resource = max_resource;
		sort(service[i].WorkerNodeVector.begin(),service[i].WorkerNodeVector.end(),comparator);
		
		/*for(auto x:service[i].WorkerNodeVector){
			cout<<x.priority_level<<" "<<x.total_resource<<endl;
		}*/
		
	}
	
	queue<Request> requests;
	int transaction_type,resource_required;
	int req_id=0;
	int no_of_requests=0;
	cout<<"Enter no of requests: ";
	cin>>no_of_requests;
	cout<<"Enter transaction_type and resource_required\n";
	for(int i=0;i<no_of_requests;i++){
		/* 
		*  Reading Request requirements and creating request nodes.	
		*/	
		cin>>transaction_type>>resource_required;
		Request request;
		request.req_id = ++req_id;
		request.transaction_type = transaction_type;
		request.resource_required = resource_required;
		request.start_time=request.end_time=request.burst_time = 0;
		request.worker_id = -1;
		requests.push(request);	
	}
	/*
	* Initializing Execution node which contains a summary of request execution.
	*/	

	pthread_mutex_init(&execution.mtx, NULL);
	execution.request_rejected=0;
	execution.request_forced_wait=0;
	execution.request_blocked=0;
	
	/*
	* starting all the services and assigning their respective service node.
	*/	
	vector<thread> threads;
	
	for(int i=0;i<n;i++){
		thread t(serviceFunction,&service[i]);
		threads.push_back(move(t));
	}
	/*
	* Reading each request and assigning it to the appropriate Service node.
 	*/
	while(!requests.empty()){
		Request r = requests.front();
		requests.pop();
		if(r.transaction_type>=n){
			cout<<"Invalid Transaction Type"<<endl;
			continue;
		}	
		pthread_mutex_lock(&service[r.transaction_type].mtx);
		r.start_time = time(0);
		//r.burst_time = rand()%BURST_TIME+1;
		r.burst_time = BURST_TIME;
		service[r.transaction_type].service_queue.push(r);
		pthread_mutex_unlock(&service[r.transaction_type].mtx);
	}
	/*
	*  Making stop_service_flag to true so all the services can start exiting their
	*  execution.
	*/
	for(int i=0;i<n;i++){
		pthread_mutex_lock(&service[i].mtx);
		service[i].stop_service_flag = true;	
		pthread_mutex_unlock(&service[i].mtx);
	}
	/*
	* waiting until all the threads join the main thread.
	*/
	for(auto& t: threads){
		t.join();
	}	
	
	/*
	*  showing the summary of the execution.
	*/


	cout<<"\n\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~: Request Processed :~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n"<<endl;
	pthread_mutex_lock(&execution.mtx);
	
	printf("\tReq_id\tstart_time\tend_time\tburst_time\twating_time\tturn_around_time\n");
	cout<<"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"<<endl;
	
	double total_turn_around_time=0.0;
	double total_waiting_time = 0.0;
	int total_request_processed=0;
	for(Request *r:execution.executed_request){
		long turn_around_time = r->end_time-r->start_time;
		long waiting_time = turn_around_time - r->burst_time;
		total_turn_around_time += turn_around_time;
		total_waiting_time += waiting_time;
		total_request_processed+=1;
		
		printf("\n\t%d\t%ld\t%ld\t%ld\t\t%ld\t\t%ld",r->req_id,r->start_time,r->end_time,r->burst_time,waiting_time,turn_around_time);
		
	}
	double  avg_turn_around_time = total_turn_around_time/(total_request_processed?total_request_processed:1);
	double  avg_waiting_time= total_waiting_time/(total_request_processed?total_request_processed:1);
	
	pthread_mutex_unlock(&execution.mtx);
	
	cout<<"\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n"<<endl;
	
	printf("\t\tAverage Waiting Time: %.3lf\n",avg_waiting_time);
	printf("\t\tAverage Turn Around Time: %.3lf\n\n",avg_turn_around_time);
	
	printf("\t\tRequest Rejected: %d\n",execution.request_rejected);	
	printf("\t\tRequest force wait: %d\n",execution.request_forced_wait);
	printf("\t\tRequest Blocked: %d\n",execution.request_blocked);
	cout<<"\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n"<<endl;
	
	
	return 0;
}
