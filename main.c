#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "mpi.h"

// Define master rank as constant for easier use
#define MASTER_RANK 0

// Define number of particles to be processed from input file
#define PARTICLE_COUNT 5
#define ITERATION_COUNT 25

// For simplicity of calculations keep G = 1 instead of actual value
#define GRAVITY_CONSTANT 1 // Dummy value, can be modified to change viz-scale
#define TIME_QUANTUM 0.01

// Input file stays the same for all combinations
// Only specified number of particles are picked
#define INPUT_FILE "input.txt"

// Sim Output file shows the final state of simulation
// Sim and viz output will change based on no of particles and cycles
#define OUTPUT_FILE "sim_output_5_25.txt"
#define VIZ_FILE "viz_data_5_25.txt"

// Flags for easier controlling of some blocks of execution
#define DEBUG 0 // DEBUG level to print more prints during each step of execution
#define VISUALIZATION 1 // Enable/disable visualization data

int rank, proc_count;
double * velocities = NULL; // Global velocities array to be handled my MASTER

MPI_Datatype particleInfo;

void calculate_forces(double masses[], double positions[], double current_forces[], int rank, int n_by_p);
void update_particle_status(double positions[], double current_forces[], double current_velocities[], int rank, int n_by_p);

void calculate_forces(double masses[], double positions[], double current_forces[], int rank, int n_by_p)
{
    // Calculate boundaries for each processor
    int start_point = rank * n_by_p;
    int end_point = start_point + n_by_p - 1;

    // Check for boundary conditions
    if (start_point >= PARTICLE_COUNT)
    {
        return;
    }
    else if(end_point >= PARTICLE_COUNT - 1)
    {
        end_point = PARTICLE_COUNT - 1;
    }

    int counter = start_point;
    for(counter = start_point; counter <= end_point; counter++)
    {
        double force_x = 0, force_y = 0;
        int ic = 0; //internal counter

        for(ic = 0; ic < PARTICLE_COUNT; ic++)
        {
            if(counter == ic)
            {
                continue;
            }

            // Euler distance = sqrt((x1 - x2)^2 + (y1 - y2)^2)
            double x_diff = positions[2 * ic] - positions[2 * counter];
            double y_diff = positions[2 * ic + 1] - positions[2 * counter + 1];
            double distance = sqrt((pow(x_diff, 2)) + (pow(y_diff, 2)));

            // Magnitude of force and individual components
            double net_force = ((GRAVITY_CONSTANT * masses[ic]) / distance);
            force_x += (GRAVITY_CONSTANT * masses[ic] * (x_diff / (pow(distance, 3))));
            force_y += (GRAVITY_CONSTANT * masses[ic] * (y_diff / (pow(distance, 3))));
        }

        current_forces[2 * (counter - start_point)] = force_x;
        current_forces[2 * (counter - start_point) + 1] = force_y;

        if(DEBUG > 0)
        {
            printf("\nForce on particle %d = %.3f %.3f\n", counter, force_x, force_y);
        }
    }
}

void update_particle_status(double positions[], double current_forces[], double current_velocities[], int rank, int n_by_p)
{
    // Calculate boundaries for each processor
    int start_point = rank * n_by_p;
    int end_point = start_point + n_by_p - 1;

    // Check for boundary conditions
    if (start_point >= PARTICLE_COUNT)
    {
        return;
    }
    else if(end_point >= PARTICLE_COUNT - 1)
    {
        end_point = PARTICLE_COUNT - 1;
    }

    int counter = start_point;
    for(counter = start_point; counter <= end_point; counter++)
    {
        // Update position of particle based on current velocity, force on it and current position
        positions[2 * counter] += current_velocities[2 * (counter - start_point)] * TIME_QUANTUM + (current_forces[2 * (counter - start_point)] * (pow(TIME_QUANTUM, 2)) / 2);
        positions[2 * counter] = fmod(positions[2 * counter], 1.00);
        positions[2 * counter + 1] += current_velocities[2 * (counter - start_point) + 1] * TIME_QUANTUM + (current_forces[2 * (counter - start_point) + 1] * (pow(TIME_QUANTUM, 2)) / 2);
        positions[2 * counter + 1] = fmod(positions[2 * counter + 1], 1.00);

        // Update velocity of particle based on current velocity and force
        current_velocities[2 * (counter - start_point)] += current_forces[2 * (counter - start_point)] * TIME_QUANTUM;
        current_velocities[2 * (counter - start_point) + 1] += current_forces[2 * (counter - start_point) + 1] * TIME_QUANTUM;

        if(DEBUG > 0)
        {
            printf("\nPosition of particle %d = %.3f %.3f\n", counter, positions[2 * counter], positions[2 * counter + 1]);
        }
    }
}

// Driver & simulation code
int main(int argc, char * argv[])
{

    // Arrays to maintain mass, position, velocity and force for each particle
    double * masses, * positions, * current_velocities, * current_forces;

    // MPI communicator initializations
    MPI_Init(& argc, & argv);
    MPI_Comm_size(MPI_COMM_WORLD, & proc_count);
    MPI_Comm_rank(MPI_COMM_WORLD, & rank);
    FILE * viz_fp; // File for capturing visualization data at each step

    // Miscellaneous declarations
    int n_by_p = (PARTICLE_COUNT + proc_count - 1) / proc_count; // Evenly distribute particles across processors

    // Plan the data distribution memory management
    if (rank == MASTER_RANK)
    {
        // Each processor needs previous and current velocities of all
        velocities = malloc(2 * proc_count * n_by_p * sizeof(double));
    }

    masses = malloc(PARTICLE_COUNT * sizeof(double));
    positions = calloc(2 * proc_count * n_by_p, sizeof(double));
    current_velocities = malloc(2 * n_by_p * sizeof(double));
    current_forces = malloc(2 * n_by_p * sizeof(double));

    MPI_Type_contiguous(2 * n_by_p, MPI_DOUBLE, & particleInfo);
    MPI_Type_commit(& particleInfo);

    // Read input from file
    if(rank == MASTER_RANK)
    {
        FILE * fp = fopen(INPUT_FILE, "r");

        if(!fp)
        {
            printf("Error opening input file\n");
            exit(1);
        }

        int mass_counter = 0, position_counter = 0, velocity_counter = 0, line = 0;

        for(line = 0; line < PARTICLE_COUNT; line++)
        {
            fscanf(fp, "%lf %lf %lf %lf %lf", &masses[mass_counter],
                &positions[position_counter], &positions[position_counter + 1],
                &velocities[velocity_counter], &velocities[velocity_counter + 1]);

            if(DEBUG > 0)
            {
                printf("%lf %lf %lf %lf %lf\n", masses[mass_counter],
                    positions[position_counter], positions[position_counter + 1],
                    velocities[velocity_counter], velocities[velocity_counter + 1]);
            }
            mass_counter += 1;
            position_counter += 2;
            velocity_counter += 2;
        }

        fclose(fp);
    } // Input from file complete

    MPI_Bcast(masses, PARTICLE_COUNT, MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);
    MPI_Bcast(positions, 2 * proc_count * n_by_p, MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);
    MPI_Scatter(velocities, 2 * n_by_p, MPI_DOUBLE, current_velocities, 2 * n_by_p, MPI_DOUBLE, MASTER_RANK, MPI_COMM_WORLD);

    double start_time = MPI_Wtime(); // Start time for perf measurements

    if(rank == MASTER_RANK && VISUALIZATION == 1)
    {
        viz_fp = fopen(VIZ_FILE, "w+");
        if(!viz_fp)
        {
            printf("Error creating viz output file\n");
            exit(1);
        }
    }

    // Main n-body simulation computations
    int iteration_counter = 1;
    for (iteration_counter = 1; iteration_counter <= ITERATION_COUNT; iteration_counter++)
    {
        if(rank == MASTER_RANK && DEBUG > 0)
        {
            printf("\n\n----- Starting Iteration No %d -----", iteration_counter);
        }

        calculate_forces(masses, positions, current_forces, rank, n_by_p);
        update_particle_status(positions, current_forces, current_velocities, rank, n_by_p);
        MPI_Allgather(MPI_IN_PLACE, 1, particleInfo, positions, 1, particleInfo, MPI_COMM_WORLD);
        MPI_Barrier(MPI_COMM_WORLD);
        if(rank == MASTER_RANK && DEBUG > 0)
        {
            printf("\n\n----- Finishing Iteration No %d -----\n\n", iteration_counter);
        }

        // Visualizatoin block,if-enabled
        if(rank == MASTER_RANK && VISUALIZATION == 1)
        {
            int temp_counter = 0;
            for (temp_counter = 0; temp_counter < PARTICLE_COUNT; temp_counter++)
            {
                fprintf(viz_fp, "%d %lf %lf %lf\n", iteration_counter, masses[temp_counter], positions[2 * temp_counter], positions[2 * temp_counter + 1]);
            }
        }
    }

    MPI_Gather(current_velocities, 1, particleInfo, velocities, 1, particleInfo, MASTER_RANK, MPI_COMM_WORLD);

    double end_time = MPI_Wtime();
    if(rank == MASTER_RANK)
    {
        FILE * final_fp = fopen(OUTPUT_FILE, "w+");
        if(!final_fp)
        {
            printf("Error creating output file\n");
            exit(1);
        }

        int output_counter = 0;
        for (output_counter = 0; output_counter < PARTICLE_COUNT; output_counter++)
        {
            fprintf(final_fp, "%lf %lf %lf %lf %lf\n", masses[output_counter],
                positions[2 * output_counter], positions[2 * output_counter + 1],
                velocities[2 * output_counter], velocities[2 * output_counter + 1]);
        }

        fclose(final_fp);
    }

    if (rank == MASTER_RANK)
    {
        printf("\np=%d, particles=%d, iterations=%d\n", proc_count, PARTICLE_COUNT, ITERATION_COUNT);
        printf("\n\nTime taken = %.8lf seconds", end_time - start_time);
        free(velocities);

        if(VISUALIZATION == 1)
        {
            fclose(viz_fp);
        }
    }

    MPI_Type_free(& particleInfo);
    free(masses);
    free(current_velocities);
    free(current_forces);

    MPI_Finalize();
    return 0;
}
