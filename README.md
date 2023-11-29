# Distributed Transaction Scheduler
In this project, our goal is to implement a Scheduling Algorithm to simulate the behavior of efficiently allocating worker thread resources to incoming requests based on transaction type and priority levels. Implemented **Multithreading** using **pthread API**.

### Project Overview

* The platform operates on a **distributed system** consisting of multiple servers, each running specific services for different types of transactions.
* Services are comprised of worker threads responsible for processing incoming requests. Worker threads have priority levels and allocated resources, impacting their processing capacity.
* Requests are queued and directed to the appropriate service and worker thread based on transaction type and priority.
* Our challenge is to design a **scheduling algorithm** that efficiently allocates worker threads for processing requests.

### Algorithm Factors

* Prioritize worker threads based on their priority levels.
* Allocate requests considering worker thread availability.
* Account for the varying resource requirements of different transaction types.

### Input Format
  Our program will read input from the standard input:
  * Number of services (n) in the system.
  * Number of worker threads (m) for each service.
  * Priority level and resources assigned to each worker thread in the format: priority_level resources.
  * Transaction type and required resources for each request in the format: transaction_type resources_required.
  Our program efficiently processes incoming requests through user input.
  * Number of Requests (r) to simulate.
  * Transaction Type and Resource Requirements for each request.

### Output Format
  Our program will output the following information:
  * Order of processed requests.
  * Average waiting time for requests.
  * Average turnaround time for requests.
  * Number of rejected requests due to insufficient resources.
  Additionally, during high-traffic periods, the program will output:

  * The number of requests waiting due to resource shortages.  
  * The number of requests blocked due to unavailable worker threads.

### Execution Steps
  * Access the Terminal
  * Navigate to the Project Directory
  * Compile the Program: In the terminal, type **make** command to use the make file and compile the program:
  * Run the Program: After the compilation is successful, run the program using the generated executable file
  * Provide Input Data
  * Program Execution: The program will process the provided input data and display the desired output
