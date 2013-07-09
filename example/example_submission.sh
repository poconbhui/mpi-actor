#!/usr/bin/env bash
#$ -V
#$ -l h_rt=:20:
#$ -pe mpi 32
#$ -cwd

mpiexec -n 32 ./simulation
