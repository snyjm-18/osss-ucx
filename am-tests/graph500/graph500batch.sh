#!/bin/bash
#SBATCH --time=5
#SBATCH --nodes=20
#SBATCH --ntasks-per-node=10
#SBATCH -n 200
#SBATCH -t 05:00:00

export SMA_SYMMETRIC_SIZE=1G

oshrun ./graph500 28 16
echo 
oshrun ./graph500 30 16
echo
oshrun ./graph500 32 16
