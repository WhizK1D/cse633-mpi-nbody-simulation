#!/bin/bash

###### select partition (check CCR documentation)
#SBATCH --partition=general-compute --qos=general-compute
#SBATCH --account=cse633s20

####### set memory that nodes provide (check CCR documentation, e.g., 32GB)
#SBATCH --mem=48000

####### make sure no other jobs are assigned to your nodes
#SBATCH --exclusive

####### further customizations
#SBATCH --job-name="N-body-CSE-633"
#SBATCH --output=job_1_1_100_100_%j.stdout
#SBATCH --error=job_1_1_100_100_%j.stderr
#SBATCH --nodes=1
#SBATCH --ntasks-per-node=1
#SBATCH --time=00:10:00
#SBATCH --constraint=IB
##SBATCH --mail-type=END
##SBATCH --mail-user=amourya@buffalo.edu

export I_MPI_DEBUG=4
export I_MPI_PMI_LIBRARY=/usr/lib64/libpmi.so
module load intel-mpi/2018.3

srun echo "Running for 1_1_100_100"
srun -n 1 ./nbody_100_100
