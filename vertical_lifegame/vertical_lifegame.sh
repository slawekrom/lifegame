#!/bin/bash

#SBATCH -n 4
#SBATCH -e vertical_lifegame.err
#SBATCH -o vertical_lifegame.out

mpiexec ./vertical_lifegame $1 $2 $3 $4
