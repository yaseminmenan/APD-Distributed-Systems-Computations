#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 3
#define MAX_COORDS 3
#define COORD_0 0
#define COORD_1 1
#define COORD_2 2

typedef struct Processes_s {
    int rank;
    int *workers;
    int nr_workers;
    int coordonator;
} Processes;

void read_cluster(char *filename, int rank, Processes *processes) {
    int worker_rank, size;

    // Open file
    FILE *file = fopen(filename, "r");

    char line[2] = {0};

    fgets(line, MAX_LINE_LENGTH, file);
    size = atoi(line);

    processes[rank].nr_workers = size;
    processes[rank].workers = (int *) malloc (size * sizeof(int));

    for (int i = 0; i < size; i++) {

        fgets(line, MAX_LINE_LENGTH, file);

        worker_rank = atoi(line);

        processes[rank].workers[i] = worker_rank;
        processes[worker_rank].coordonator = rank;
    }

    /* Close file */
    fclose(file);    
}

void inform_workers(int rank, int size, Processes *processes) {
    int i, j, worker;
    
    // Send the coordonator's rank to each worker
    for (i = 0; i < size; i++) {

        worker = processes[rank].workers[i];

        printf("M(%d,%d)\n", rank, worker);
        MPI_Send(&rank, 1, MPI_INT, worker, 0, MPI_COMM_WORLD);
    }
}

void send_data(int rank, int size, Processes *processes) {
    int pair[2];
    pair[0] = rank;
    pair[1] = size;
    
    // Send data to the coordonator's workers and then to the other
    // coordonators
    for (int i = 0; i < size; i++) {
       int current_worker = processes[rank].workers[i];

       // Send the coordonator's rank and worker array size
       printf("M(%d,%d)\n", rank, current_worker);
       MPI_Send(&pair, 2, MPI_INT, current_worker, 0, MPI_COMM_WORLD);

       // Send the worker array
       printf("M(%d,%d)\n", rank, current_worker);
       MPI_Send(processes[rank].workers, size, MPI_INT, current_worker, 0, MPI_COMM_WORLD);
    }

    for (int i = 0; i < MAX_COORDS; i++) {
        if (i != rank) {
            printf("M(%d,%d)\n", rank, i);
            MPI_Send(&pair, 2, MPI_INT, i, 0, MPI_COMM_WORLD);

            printf("M(%d,%d)\n", rank, i);
            MPI_Send(processes[rank].workers, size, MPI_INT, i, 0, MPI_COMM_WORLD);
        }
    }
}

void coord_receive_data(int rank, int size, Processes *processes) {
    int recv_size, recv_coord, recv_pair[2];
    
    // Coordonator receives data from the other coordonators
    for (int i = 0; i < MAX_COORDS - 1; i++) {
        // Receives coordinator rank and worker array size
        MPI_Recv(&recv_pair, 2, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        recv_coord = recv_pair[0];
        recv_size = recv_pair[1];

        processes[recv_coord].nr_workers = recv_size;
        processes[recv_coord].workers = (int *) malloc(recv_size * sizeof(int));
        
	// Receives worker array
        MPI_Recv(processes[recv_coord].workers, recv_size, MPI_INT, recv_coord, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Update topology
        for (int i = 0; i < recv_size; i++) {
            int worker_rank = processes[recv_coord].workers[i];
            processes[worker_rank].coordonator = recv_coord;
        }

        // Send data to workers
        for (int j = 0; j < size; j++) {
            int current_worker = processes[rank].workers[j];

            printf("M(%d,%d)\n", rank, current_worker);
            MPI_Send(&recv_pair, 2, MPI_INT, current_worker, 0, MPI_COMM_WORLD);

            printf("M(%d,%d)\n", rank, current_worker);
            MPI_Send(processes[recv_coord].workers, recv_size, MPI_INT, current_worker, 0, MPI_COMM_WORLD);
         }
    }
}

void worker_receive_data(int rank, int coordonator, Processes *processes) {
    int recv_pair[2], recv_coord, recv_size;
    
    // Worker receives data from its coordonator
    for (int i = 0; i < MAX_COORDS; i++){
        MPI_Recv(&recv_pair, 2, MPI_INT, coordonator, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        recv_coord = recv_pair[0];
        recv_size = recv_pair[1];

        processes[recv_coord].nr_workers = recv_size;
        processes[recv_coord].workers = (int *) malloc(recv_size * sizeof(int));

        MPI_Recv(processes[recv_coord].workers, recv_size, MPI_INT, coordonator, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        // Update topology
       for (int i = 0; i < recv_size; i++) {
            int worker_rank = processes[recv_coord].workers[i];
            processes[worker_rank].coordonator = recv_coord;
        }
    }
}

void send_worker_iterations(int rank, Processes *processes, int iterations, int start, int size, int *V, int N) {
    
    int end, pair[2], worker;
    
    // Compute start and end values for worker's
    // iterations and send them
    for (int i = 0; i < size; i++) {
        end = start + iterations;

        pair[0] = start;
        pair[1] = end;

        worker = processes[rank].workers[i];
	
        printf("M(%d,%d)\n", rank, worker);
        MPI_Send(&pair, 2, MPI_INT, worker, 0, MPI_COMM_WORLD);
	
	// Send the array
        printf("M(%d,%d)\n", rank, worker);
        MPI_Send(V, N, MPI_INT, worker, 0, MPI_COMM_WORLD);

        start = end;
    }
}

void print_topology(int rank, Processes *processes, int nProcesses) {
    printf("%d ->", rank);
    for (int i = 0; i < MAX_COORDS; i++) {
        printf(" %d:", i);
        for (int j = 0; j < processes[i].nr_workers; j++) {
            printf("%d", processes[i].workers[j]);
            if (j != processes[i].nr_workers - 1) {
                printf(",");
            }
        }
    }
    printf("\n");
}

int main (int argc, char *argv[])
{
    int  nProcesses, rank, last, coordonator, path_size, size, *V, *recv_V, N, total_workers;
    char *path;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nProcesses);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    MPI_Status status;

    total_workers = nProcesses - MAX_COORDS;

    Processes *processes = (Processes *) malloc(nProcesses * sizeof(Processes));
    for (int i = 0; i < nProcesses; i++) {
        processes[i].rank = i;
        processes[i].nr_workers = 0;
        processes[i].coordonator = -1;
    }

    path_size = strlen("cluster") + 2 + strlen(".txt");
    path = malloc(path_size * sizeof(char)); 

    N = atoi(argv[1]);
    V = (int *) malloc(N * sizeof(int));
    recv_V = (int *) malloc(N * sizeof(int));

    if (rank < MAX_COORDS) {
        snprintf(path, path_size, "cluster%d.txt", rank);

        // Read workers and assign their coordonator
        read_cluster(path, rank, processes);

        size = processes[rank].nr_workers;

        // Inform workers of coordonator
        inform_workers(rank, size, processes);

        // Send worker array to all other coordonators
        // and to coordonator's workers 
        send_data(rank, size, processes);

        // Receive data from other coordonators and send to workers
        coord_receive_data(rank, size, processes);
	
	// Print the complete topology
        print_topology(rank, processes, nProcesses);
    }
    else {

        // Wait to be assigned a coordonator
        if (processes[rank].coordonator == -1) {
            MPI_Recv(&coordonator, 1, MPI_INT, MPI_ANY_TAG, 0, MPI_COMM_WORLD,  MPI_STATUS_IGNORE);
            processes[rank].coordonator = coordonator;
        } 

        // Receive data from coordonator and updates topology
        worker_receive_data(rank, coordonator, processes);
	
	// Print the complete topology
        print_topology(rank, processes, nProcesses);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (rank == COORD_0) {
	// Generate array
        for (int i = 0; i < N; i++) {
            V[i] = i;
        }

        // Send array to other coordonators
        for (int i = 1; i < MAX_COORDS; i++ ){
            printf("M(%d,%d)\n", rank, i);
            MPI_Send(V, N, MPI_INT, i, 0, MPI_COMM_WORLD);
        }    

        // Send interations to workers
        int iter = N / total_workers;
        send_worker_iterations(rank, processes, iter, 0, size, V, N);

        int recv_pair[2], start, end;
	
	// Receive the new array from the other coordonators
        // and update the values
        for (int i = 1; i < MAX_COORDS; i++) {
            for (int j = 0; j < processes[i].nr_workers; j++) {
                MPI_Recv(&recv_pair, 2, MPI_INT, i , 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                start = recv_pair[0];
                end = recv_pair[1];

                MPI_Recv(recv_V, N, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                for (int i = start; i < end; i++) {
                    V[i] = recv_V[i];
                }
            }
        }

	// Receive the new array from the workers
        // and updates the values
        for (int i = 0; i < size; i++) {
            int worker = processes[rank].workers[i];
            MPI_Recv(&recv_pair, 2, MPI_INT, worker , 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            start = recv_pair[0];
            end = recv_pair[1];

            MPI_Recv(recv_V, N, MPI_INT, worker, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            for (int i = start; i < end; i++) {
                V[i] = recv_V[i];
            }
        }
	
	// Print the new array
        printf("Rezultat:");
        for (int i = 0; i < N;i++) {
            printf(" %d", V[i]);
        }
        printf("\n");
    } 
    else if (rank == COORD_1 || rank == COORD_2) {
	// Receive array from coordonator 0
        MPI_Recv(V, N, MPI_INT, COORD_0, 0, MPI_COMM_WORLD,  MPI_STATUS_IGNORE);

        int iter = N / total_workers;
        int start;
	
	// Compute start values for workers
        if (rank == COORD_1) {
            start = processes[COORD_0].nr_workers * iter;
        } else {
           start = (processes[COORD_0].nr_workers + processes[COORD_1].nr_workers) * iter;
        }

        // Send interations to workers
        send_worker_iterations(rank, processes, iter, start, size, V, N); 

        int recv_pair[2];
	// Receive the new array from workers
        for (int i = 0; i < size; i++) {
            int worker = processes[rank].workers[i];

            MPI_Recv(&recv_pair, 2, MPI_INT, worker, 0, MPI_COMM_WORLD,  MPI_STATUS_IGNORE);

            printf("M(%d,%d)\n", rank, COORD_0);
            MPI_Send(&recv_pair, 2, MPI_INT, COORD_0, 0, MPI_COMM_WORLD);

            MPI_Recv(recv_V, N, MPI_INT, worker, 0, MPI_COMM_WORLD,  MPI_STATUS_IGNORE);

            // Send the array to coordonator 0
            printf("M(%d,%d)\n", rank, COORD_0);
            MPI_Send(recv_V, N, MPI_INT, COORD_0, 0, MPI_COMM_WORLD);
        }
    } else {
        int recv_pair[2], start, end;
        // Receive interval for array calculations
        MPI_Recv(&recv_pair, 2, MPI_INT, coordonator, 0, MPI_COMM_WORLD,  MPI_STATUS_IGNORE);
        start = recv_pair[0];
        end = recv_pair[1];
	
	// Receive the array
        MPI_Recv(V, N, MPI_INT, coordonator, 0, MPI_COMM_WORLD,  MPI_STATUS_IGNORE);
	
	// Do calculations
        for (int i = start; i < end; i++) {
            V[i] *= 2;
        }
        	
	// Send the interval and updated array to coordonator
        printf("M(%d,%d)\n", rank, coordonator);
        MPI_Send(&recv_pair, 2, MPI_INT, coordonator, 0, MPI_COMM_WORLD);

        printf("M(%d,%d)\n", rank, coordonator);
        MPI_Send(V, N, MPI_INT, coordonator, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();

}

