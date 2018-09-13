# heatSim

This project consists on a program written in C for simulating heat propagation in a surface. The square surface is a matrix *i* x *j*, where 0 <= *i* <= *N* + 1, 0 <= *j* <= *N* + 1 and *h[i][j]* represents the temperature in that specific spot. The goal is to calculate the temperature in each spot after the heat has spread, given the temperature imposed in each point.

The project is divided in 4 parts:

  - Exercise 1 - Development of a parallel version of the problem using the message exchange paradigm, using a simple stop condition. In order to solve this exercise, the students used a library that implements the exchange of messages.

  - Exercise 2 - Study and improvement of the message exchange library given by the teachers.

  - Exercise 3 - Development of another parallel version of the problem, this time using the shared memory paradigm. It will extend each version with a dinamic stop condition.

  - Exercise 4 - Added the functionality of periodically saving the system state, allowing the program to recover from a suspension or flaw. Also implemented a server which allows for a more intuitive visualization of the matrix.

## Getting Started

### Prerequisites

You will need the gcc compiler installed in your machine.

### Compiling

Download the .zip and compile the files of each exercise using the following command:

```
$ make heatSim
```
which should create the executable file 'heatSim'.

Note: the name 'heatSim' used in the command and in the executable file will change depending on the number of the exercise. Open the 'Makefile' file in each folder to check for the right name.

## Running

The program should be executed in the following way:

```
$ make run
```

It will run the program with the predefined values. You can change them in the 'Makefile' file.
The values have the following scheme:

  - Exercise 1: N tEsq tSup tDir tInf iter trab csz;
  - Exercise 2: N tEsq tSup tDir tInf iter trab csz;
  - Exercise 3: N tEsq tSup tDir tInf iter trab csz;
  - Exercise 4: N tEsq tSup tDir tInf iter trab csz fichS periodoS.

These arguments have the following purpose:

  - N: size of the matrix;
  - tEsq, tSup, tDir, tInf: temperatures in each side of the matrix;
  - iter: integer > 0 which defines the number of iterations of the program;
  - trab: integer > 0 which defines the number of tasks of the simulation;
  - csz: integer > 0 which defines how many messages the queue can hold;
  - fichS: name of the file which contains the state of the matrix;
  - periodoS: integer >= 0 which defines the time between savings.

## Authors

* Francisco Matos
* Daniela Lopes

## Acknowledgments

The library used in the project was given by the teachers of the course.

## Evaluation

19/20
